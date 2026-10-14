#!/usr/bin/env python3
"""
Find Shift-JIS (cp932) text in the game binaries, emit it as paste-ready C
literals, and lint our own sources for non-ASCII bytes that would break the
build.

Why: Konami's programmers left Japanese text in the modules -- debug printf
formats, asserts, menu labels, file names.  Those bytes are cp932, not UTF-8,
and every tool in the pipeline mishandles them differently: splat/spimdisasm
emit them as raw bytes inside `.asciz` that gas then rejects or re-encodes,
an editor that "helpfully" saves a .c file as UTF-8 silently changes every
literal, and an identifier containing them is not portable C at all.  The
project rule (docs/11-japanese-text.md) is therefore: identifiers are ASCII,
SJIS literals are written as `\\x` escapes, the translation goes in a comment.
This tool produces the escapes and checks the rule.

How (binary scan): the PRX is a plain ELF32-LE, so the section table is read
directly with struct (no readelf).  Modules link at base 0, so a section's
sh_addr IS its vram.  Every SHF_ALLOC, non-executable PROGBITS section is cut
at NUL bytes; each NUL-terminated run (or, when binary data runs straight into
a string, its longest valid tail) is decoded strictly as cp932 and kept only
if it
  - decodes with no errors and no control / private-use / unassigned chars
    (except \\t \\n \\r),
  - contains at least 2 real Japanese chars (kana, kanji, CJK punctuation,
    fullwidth forms) produced by DOUBLE-byte sequences,
  - is not dominated by halfwidth katakana (single bytes 0xA1-0xDF, which is
    what random binary data looks like when decoded as cp932).
Binary data is full of bytes that happen to be valid cp932, so these filters
are deliberately strict; a missed string is cheaper than 10,000 fake ones.

It also inspects the symbol-like data -- .lib.ent / .lib.stub library names,
.rodata.sceModuleInfo module name, .comment -- and says whether any of it is
non-ASCII (--symbols).

How (--check-src): a small C lexer walks src/**/*.c|h (and include/ if any)
and flags any byte >= 0x80 outside a comment, i.e. in an identifier, a string
or a char literal.  UTF-8 in comments is allowed, but a comment that is not
valid UTF-8 (a raw cp932 paste) is flagged too.  config/symbols/*.txt is
checked the same way: everything before `//` must be ASCII.

Usage:
    scripts/sjis_strings.py rel_debug_menu            # table
    scripts/sjis_strings.py rel_debug_menu --c        # /* 0xADDR: utf8 */ "\\x82.."
    scripts/sjis_strings.py --all --tsv > /tmp/x.tsv  # every module (+ EBOOT.elf if built)
    scripts/sjis_strings.py --all --summary           # counts per module
    scripts/sjis_strings.py --all --symbols           # Japanese in lib/module names?
    scripts/sjis_strings.py --check-src               # lint; exit 1 on any hit
"""
import argparse
import glob
import os
import struct
import sys
import unicodedata

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GMODULE = os.path.join(ROOT, "iso_extracted/PSP_GAME/USRDIR/gmodule")
EBOOT = os.path.join(ROOT, "build/EBOOT.elf")

SHF_ALLOC, SHF_EXECINSTR = 0x2, 0x4
SHT_PROGBITS = 1


# --------------------------------------------------------------------- ELF ---

def read_sections(data):
    """[(name, type, flags, addr, off, size)] from the ELF32-LE section table."""
    if data[:4] != b"\x7fELF":
        raise ValueError("not a plaintext ELF (encrypted ~PSP? decrypt it first)")
    e_shoff, = struct.unpack_from("<I", data, 0x20)
    e_shentsize, e_shnum, e_shstrndx = struct.unpack_from("<HHH", data, 0x2E)
    raw = [struct.unpack_from("<10I", data, e_shoff + i * e_shentsize)
           for i in range(e_shnum)]
    stroff = raw[e_shstrndx][4]
    out = []
    for name, typ, flags, addr, off, size, *_ in raw:
        end = data.index(b"\0", stroff + name)
        out.append((data[stroff + name:end].decode("ascii", "replace"),
                    typ, flags, addr, off, size))
    return out


def data_sections(secs):
    """Allocated, non-executable sections with file contents: where strings live.
    The sceNid/sceStub tables are hashes and code, never text, so skip them."""
    skip = (".rodata.sceNid", ".rodata.sceVstub", ".sceStub.text")
    return [s for s in secs
            if s[1] == SHT_PROGBITS and s[2] & SHF_ALLOC
            and not s[2] & SHF_EXECINSTR and s[5] and s[0] not in skip]


def module_paths(args):
    if args.all:
        paths = sorted(glob.glob(os.path.join(GMODULE, "*.prx")))
        if os.path.exists(EBOOT):
            paths.append(EBOOT)
        return paths
    paths = []
    for m in args.modules:
        if os.path.exists(m):
            paths.append(m)
        elif m in ("EBOOT", "modehsys"):
            paths.append(EBOOT)
        else:
            paths.append(os.path.join(GMODULE, m if m.endswith(".prx") else m + ".prx"))
    return paths


# ------------------------------------------------------------- detection ---

def is_japanese(ch):
    """Kana, CJK punctuation, fullwidth forms, or a JIS LEVEL-1 kanji (the
    2965 common ones, cp932 0x889F-0x9872).  Level-2 / IBM-extension kanji
    are valid text but are exactly what random bytes decode to (0x9A99 is
    the tail of a double, 0x99BF of a float), so they never count."""
    o = ord(ch)
    if 0x3000 <= o <= 0x30FF or 0xFF01 <= o <= 0xFF5E:
        return True
    if 0x4E00 <= o <= 0x9FFF:
        return 0x889F <= int.from_bytes(ch.encode("cp932"), "big") <= 0x9872
    return False


def is_rare_kanji(ch):
    return 0x4E00 <= ord(ch) <= 0xFA2D and not is_japanese(ch)


def is_halfwidth_kana(ch):
    return 0xFF61 <= ord(ch) <= 0xFF9F


def judge(raw, aligned=True):
    """Decoded text if `raw` (no NUL) is a plausible SJIS string, else None.
    `aligned`: starts on a 4-byte boundary, as every MWCC string literal
    does; an unaligned start (binary running into text) needs more proof."""
    try:
        text = raw.decode("cp932")
    except UnicodeDecodeError:
        return None
    jp = hw = rare = 0
    for ch in text:
        if ch in "\t\n\r":
            continue
        cat = unicodedata.category(ch)
        if cat[0] == "C":                       # Cc Cf Co Cn Cs
            return None
        if is_japanese(ch):
            jp += 1
        elif is_halfwidth_kana(ch):
            hw += 1
        elif is_rare_kanji(ch):
            rare += 1
    if jp < (2 if aligned else 4) or hw * 4 > len(text) or rare > jp:
        return None
    # mostly printable: the Japanese chars plus ASCII text, not a soup of
    # ASCII punctuation that happens to sit between two kanji-shaped words
    ascii_noise = sum(1 for ch in text if ord(ch) < 0x80 and not (ch.isalnum() or ch in " \t\n\r%.,:;!?-_/()[]<>=+*'\"#&@"))
    if ascii_noise * 3 > len(text):
        return None
    return text


def scan(data, sec):
    """Yield (vram, raw_bytes, utf8_text) for each SJIS string in one section."""
    name, _t, _f, addr, off, size = sec
    blob = data[off:off + size]
    pos = 0
    while pos < len(blob):
        nul = blob.find(b"\0", pos)
        if nul < 0:
            nul = len(blob)
        seg = blob[pos:nul]
        if seg and any(b >= 0x81 for b in seg):
            # the segment may begin with binary (a pointer, a float) that runs
            # straight into the string; take the EARLIEST start that is valid
            for s in range(len(seg)):
                if len(seg) - s < 4:
                    break
                text = judge(seg[s:], aligned=(addr + pos + s) % 4 == 0)
                if text is not None:
                    yield addr + pos + s, seg[s:], text
                    break
        pos = nul + 1


# ------------------------------------------------------------ formatting ---

HEX = set(b"0123456789abcdefABCDEF")


def c_escape(raw):
    """C literal for `raw`: printable ASCII kept, everything else \\xNN; split
    "..." "..." where a hex escape would otherwise swallow a following hex
    digit (C's \\x is greedy and has no length limit)."""
    out, prev_hex = ['"'], False
    for b in raw:
        if 0x20 <= b < 0x7F:
            if prev_hex and b in HEX:
                out.append('" "')
            ch = chr(b)
            out.append({'"': '\\"', "\\": "\\\\"}.get(ch, ch))
            prev_hex = False
        elif b in (0x0A, 0x09, 0x0D):
            out.append({0x0A: "\\n", 0x09: "\\t", 0x0D: "\\r"}[b])
            prev_hex = False
        else:
            out.append("\\x%02x" % b)
            prev_hex = True
    out.append('"')
    return "".join(out)


def oneline(text):
    return text.replace("\\", "\\\\").replace("\n", "\\n").replace("\t", "\\t").replace("\r", "\\r")


def section_of(secs, addr):
    for s in secs:
        if s[3] <= addr < s[3] + s[5]:
            return s[0]
    return "?"


# --------------------------------------------------------------- symbols ---

def cstr(data, secs, vaddr):
    """NUL-terminated bytes at vram `vaddr`, read through the section table."""
    for name, typ, flags, addr, off, size in secs:
        if typ == SHT_PROGBITS and flags & SHF_ALLOC and addr <= vaddr < addr + size:
            p = off + (vaddr - addr)
            return data[p:data.index(b"\0", p)]
    return None


def symbol_data(data, secs):
    """[(kind, bytes)] for every name-like field: lib names, module name, .comment."""
    by = {s[0]: s for s in secs}
    out = []
    for sec, stride_fn in ((".lib.ent", lambda b, p: b[p + 8] * 4),
                           (".lib.stub", lambda b, p: b[p + 8] * 4)):
        if sec not in by:
            continue
        _n, _t, _f, addr, off, size = by[sec]
        p = off
        while p < off + size:
            namep, = struct.unpack_from("<I", data, p)
            if namep:
                out.append((sec + " name", cstr(data, secs, namep) or b"<unresolved>"))
            stride = stride_fn(data, p) or 4
            p += stride
    if ".rodata.sceModuleInfo" in by:
        off = by[".rodata.sceModuleInfo"][4]
        out.append(("sceModuleInfo name", data[off + 4:off + 32].split(b"\0")[0]))
    if ".comment" in by:
        _n, _t, _f, _a, off, size = by[".comment"]
        out.append((".comment", data[off:off + size].rstrip(b"\0")))
    return out


# ------------------------------------------------------------- check-src ---

def lint_c(path):
    """[(line, what)] for every non-ASCII byte outside comments."""
    src = open(path, "rb").read()
    hits, i, n, line = [], 0, len(src), 1
    state = "code"            # code | line_comment | block_comment | str | chr
    comment_start = None
    reported = set()

    def hit(what):
        if (line, what) not in reported:
            reported.add((line, what))
            hits.append((line, what))

    while i < n:
        b = src[i]
        if state == "code":
            if src.startswith(b"//", i):
                state, comment_start = "line_comment", i
                i += 2
                continue
            if src.startswith(b"/*", i):
                state, comment_start = "block_comment", i
                i += 2
                continue
            if b == 0x22:
                state = "str"
            elif b == 0x27:
                state = "chr"
            elif b >= 0x80:
                hit("non-ASCII byte in code (identifier?)")
        elif state in ("str", "chr"):
            if b == 0x5C:                       # backslash: skip escaped char
                i += 2
                continue
            if (state == "str" and b == 0x22) or (state == "chr" and b == 0x27) or b == 0x0A:
                state = "code"
            elif b >= 0x80:
                hit("raw non-ASCII byte in %s literal (use \\x escapes)"
                    % ("string" if state == "str" else "char"))
        elif state == "line_comment":
            if b == 0x0A:
                try:
                    src[comment_start:i].decode("utf-8")
                except UnicodeDecodeError:
                    hit("comment is not valid UTF-8 (raw cp932 paste?)")
                if src[i - 1] == 0x5C:
                    # `\` + newline continues a // comment: the NEXT LINE is
                    # swallowed. With cp932 this happens by accident, since
                    # 0x5C is a trail byte (e.g. 表 = 95 5C, ソ = 83 5C).
                    hit("// comment ends in a backslash byte: next line is commented out")
                else:
                    state = "code"
        elif state == "block_comment":
            if src.startswith(b"*/", i):
                try:
                    src[comment_start:i].decode("utf-8")
                except UnicodeDecodeError:
                    hit("comment is not valid UTF-8 (raw cp932 paste?)")
                state = "code"
                i += 2
                continue
        if b == 0x0A:
            line += 1
        i += 1
    if state in ("line_comment", "block_comment"):
        try:
            src[comment_start:].decode("utf-8")
        except UnicodeDecodeError:
            hit("comment is not valid UTF-8 (raw cp932 paste?)")
        if state == "block_comment":
            hit("unterminated /* comment at end of file")
    return hits


def lint_symbols(path):
    hits = []
    for ln, raw in enumerate(open(path, "rb").read().split(b"\n"), 1):
        code = raw.split(b"//", 1)[0]
        if any(c >= 0x80 for c in code):
            hits.append((ln, "non-ASCII byte in symbol name/value"))
        try:
            raw.decode("utf-8")
        except UnicodeDecodeError:
            hits.append((ln, "line is not valid UTF-8"))
    return hits


def check_src():
    files = []
    for pat in ("src/**/*.c", "src/**/*.h", "include/**/*.h", "include/**/*.c"):
        files += glob.glob(os.path.join(ROOT, pat), recursive=True)
    syms = sorted(glob.glob(os.path.join(ROOT, "config/symbols/*.txt")))
    total = 0
    for f in sorted(set(files)):
        for ln, what in lint_c(f):
            print("%s:%d: %s" % (os.path.relpath(f, ROOT), ln, what))
            total += 1
    for f in syms:
        for ln, what in lint_symbols(f):
            print("%s:%d: %s" % (os.path.relpath(f, ROOT), ln, what))
            total += 1
    print("checked %d source + %d symbol files: %d problem(s)"
          % (len(set(files)), len(syms), total), file=sys.stderr)
    return 1 if total else 0


# ------------------------------------------------------------------ main ---

def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0],
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("modules", nargs="*", help="rel_xxx, a path, or EBOOT")
    ap.add_argument("--all", action="store_true", help="all 28 gmodule PRXs (+ build/EBOOT.elf if present)")
    fmt = ap.add_mutually_exclusive_group()
    fmt.add_argument("--tsv", action="store_true", help="module, vram, section, len, C literal, utf8")
    fmt.add_argument("--c", action="store_true", help='emit /* 0xADDR: utf8 */ "escaped" lines')
    fmt.add_argument("--summary", action="store_true", help="string count per module only")
    fmt.add_argument("--symbols", action="store_true", help="lib/module names + .comment, flag non-ASCII")
    ap.add_argument("--check-src", action="store_true", help="lint src/ and config/symbols/ for non-ASCII")
    args = ap.parse_args()

    if args.check_src:
        return check_src()
    if not args.all and not args.modules:
        ap.error("give a module name, --all, or --check-src")

    if args.tsv:
        print("module\tvram\tsection\tlen\tc_literal\tutf8")
    grand = 0
    for path in module_paths(args):
        mod = os.path.basename(path).rsplit(".", 1)[0]
        data = open(path, "rb").read()
        secs = read_sections(data)

        if args.symbols:
            for kind, raw in symbol_data(data, secs):
                bad = any(c >= 0x80 for c in raw)
                print("%-18s %-22s %s%s" % (mod, kind, raw.decode("latin-1"),
                                            "   <-- NON-ASCII" if bad else ""))
            continue

        found = [(v, s[0], raw, text) for s in data_sections(secs)
                 for v, raw, text in scan(data, s)]
        grand += len(found)
        if args.summary:
            print("%-20s %5d" % (mod, len(found)))
            continue
        if args.c and found:
            print("/* ---- %s ---- */" % mod)
        for vram, sec, raw, text in found:
            lit = c_escape(raw)
            if args.tsv:
                print("%s\t0x%08X\t%s\t%d\t%s\t%s" % (mod, vram, sec, len(raw) + 1, lit, oneline(text)))
            elif args.c:
                print("/* 0x%08X: %s */ %s" % (vram, oneline(text).replace("*/", "* /"), lit))
            else:
                print("%s 0x%08X %-6s %4d  %s\n    %s" % (mod, vram, sec, len(raw) + 1, oneline(text), lit))
    if args.summary:
        print("%-20s %5d" % ("TOTAL", grand))
    return 0


if __name__ == "__main__":
    import signal
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)   # quiet `| head`
    sys.exit(main())

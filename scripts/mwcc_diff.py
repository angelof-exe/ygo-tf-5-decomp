#!/usr/bin/env python3
"""
Relocation-aware diff between splat's target asm and a locally-built mwccpsp
object, emulating what decomp.me/asm-differ actually check: two instruction
words are equal if either (a) their raw bytes match, or (b) both reference
the same relocation kind (HI16/LO16/26) against the same symbol, regardless
of the specific baked immediate (unlinked .o files carry zeroed placeholders,
and PSP absolute globals are sometimes baked into the ORIGINAL binary with no
runtime relocation at all — see the file header of src/rel_movie_viewer.c).

Usage:
    scripts/mwcc_diff.py asm/rel_movie_viewer/text.s build/mwcc/rel_movie_viewer.o \
        [func_00000184 func_00000034 ...]   # default: every function in target

Extend KNOWN_ADDR below as you name/confirm more absolute addresses baked
into globals without a real relocation entry (see docs/06-splitting-and-matching.md).
"""
import re
import struct
import subprocess
import sys

KNOWN_ADDR = {
    "D_0009DB00": 0x0009DB00,
    "D_0009DB04": 0x0009DB04,
    "D_000A3F08": 0x000A3F08,
    "D_000A3F0C": 0x000A3F0C,
    "D_000A4030": 0x000A4030,
    "D_000A4034": 0x000A4034,
    "D_000A4038": 0x000A4038,
    "D_000A3FF9": 0x000A3FF9,
}


def norm_sym(s):
    if s is None:
        return None
    m = re.match(r"(D|func)_0*([0-9A-Fa-f]+)$", s)
    if m:
        return f"{m.group(1)}_{int(m.group(2), 16):08X}"
    return s


def parse_target(path):
    funcs = {}
    cur = None
    for line in open(path):
        m = re.match(r"glabel (func_[0-9A-F]+)", line)
        if m:
            cur = m.group(1)
            funcs[cur] = []
            continue
        if re.match(r"endlabel (func_[0-9A-F]+)", line):
            cur = None
            continue
        m = re.match(r"\s*/\* [0-9A-F]+ [0-9A-F]+ ([0-9A-F]{8}) \*/\s*(\S+)\s*(.*)", line)
        if m and cur:
            hexstr, mnem, operands = m.groups()
            word = struct.unpack("<I", bytes.fromhex(hexstr))[0]
            sym = kind = None
            symm = re.search(r"%hi\((\w+)\)", operands)
            if symm:
                sym, kind = symm.group(1), "HI16"
            else:
                symm = re.search(r"%lo\((\w+)\)", operands)
                if symm:
                    sym, kind = symm.group(1), "LO16"
                elif mnem in ("jal", "j", "b") and re.match(r"^[A-Za-z_]\w*$", operands.strip()) \
                        and not operands.strip().startswith(".L"):
                    # any identifier, not just func_XXXXXXXX: since
                    # scripts/resolve_nids.py wired config/symbols/*.txt into the
                    # splat configs, the target's calls are named imports
                    # (sceHttpInit, ehsys_memset, cardalbum_1A2B3C4D).
                    sym, kind = operands.strip(), "26"
            funcs[cur].append((word, mnem, operands.strip(), sym, kind))
    return funcs


def parse_candidate(objdump_output):
    funcs = {}
    cur = None
    for line in objdump_output.splitlines():
        m = re.match(r"[0-9a-f]+ <(func_[0-9A-F]+)>:", line)
        if m:
            cur = m.group(1)
            funcs[cur] = []
            continue
        m = re.match(r"\s*[0-9a-f]+:\s+([0-9a-f]{8})\s+(\S+)\s*(.*)", line)
        if m and cur:
            hexstr, mnem, operands = m.groups()
            funcs[cur].append([int(hexstr, 16), mnem, operands.strip(), None, None])
            continue
        m = re.match(r"\s+[0-9a-f]+:\s+(R_MIPS_\w+)\s+(\S+)", line)
        if m and cur and funcs[cur]:
            reloc_type, sym = m.groups()
            kind = {"R_MIPS_HI16": "HI16", "R_MIPS_LO16": "LO16", "R_MIPS_26": "26"}.get(reloc_type, reloc_type)
            funcs[cur][-1][3] = sym
            funcs[cur][-1][4] = kind
    return funcs


def compare(tgt, cand, name):
    t, c = tgt.get(name), cand.get(name)
    if t is None:
        return f"{name}: NOT IN TARGET"
    if c is None:
        return f"{name}: NOT COMPILED"
    if len(t) != len(c):
        return f"{name}: SIZE MISMATCH target={len(t)} words candidate={len(c)} words"
    diffs = []
    for i, (tw, cw) in enumerate(zip(t, c)):
        tword, tmnem, top, tsym, tkind = tw
        cword, cmnem, cop, csym, ckind = cw
        tsym, csym = norm_sym(tsym), norm_sym(csym)
        if tsym is not None or csym is not None:
            if tsym is not None and csym is not None:
                if tsym != csym or tkind != ckind:
                    diffs.append(f"  [{i}] RELOC MISMATCH target={tmnem} {top} ({tkind} {tsym}) "
                                 f"vs candidate={cmnem} {cop} ({ckind} {csym})")
                continue
            if csym is not None:  # only candidate has a reloc: check known-address bake-in
                addr = KNOWN_ADDR.get(csym)
                if addr is None:
                    diffs.append(f"  [{i}] candidate has reloc {ckind} {csym} (unknown addr) but "
                                 f"target raw word=0x{tword:08x} {tmnem} {top}")
                    continue
                hi = (addr + 0x8000) >> 16 & 0xFFFF
                lo = addr & 0xFFFF
                if lo >= 0x8000:
                    lo -= 0x10000
                expect = hi if ckind == "HI16" else (lo & 0xFFFF)
                if expect != (tword & 0xFFFF):
                    diffs.append(f"  [{i}] baked-const check FAILED target=0x{tword:08x} "
                                 f"expect_imm=0x{expect:04x} (candidate reloc {ckind} {csym}=0x{addr:08x})")
                continue
            diffs.append(f"  [{i}] target has reloc {tkind} {tsym} but candidate raw "
                         f"word=0x{cword:08x} {cmnem} {cop} (missing relocation!)")
            continue
        if tword != cword:
            diffs.append(f"  [{i}] target=0x{tword:08x} ({tmnem} {top})  candidate=0x{cword:08x} ({cmnem} {cop})")
    if not diffs:
        return f"{name}: MATCH ({len(t)} words)"
    return f"{name}: {len(diffs)} diff(s)\n" + "\n".join(diffs)


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    target_path, obj_path = sys.argv[1], sys.argv[2]
    objdump_out = subprocess.run(
        ["mips-linux-gnu-objdump", "-dr", obj_path],
        capture_output=True, text=True, check=True,
    ).stdout
    tgt = parse_target(target_path)
    cand = parse_candidate(objdump_out)
    names = sys.argv[3:] or sorted(tgt.keys())
    for n in names:
        print(compare(tgt, cand, n))
        print()

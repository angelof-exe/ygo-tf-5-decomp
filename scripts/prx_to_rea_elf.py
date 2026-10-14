#!/usr/bin/env python3
"""Rewrap a gmodule PRX as a static ELF that REA's Ghidra PSP lane accepts.

REA (github.com/morluto/rea) only admits PSP/Allegrex targets that are static
ET_EXEC images carrying the PSPSDK declaration (e_flags 0x10a23001 plus a
24-byte MIPS ABI flags record). Our modules are relocatable PRXs (e_type
0xFFA0, flags 0x10a23000, no ABI record), so REA refuses them outright.

A PRX loaded at base 0 needs no relocation at all: every HI16/LO16/32/26
relocation adds the load base, and the base is 0. So the PRX's section bytes
ARE the correctly relocated image at vram 0 — the same vram splat uses, so
Ghidra/REA addresses equal `func_XXXXXXXX` names in asm/ and src/.

This script copies those bytes into a fresh ET_EXEC ELF with:
  - one PT_LOAD covering every allocated section (+ .bss as memsz),
  - PT_MIPS_ABIFLAGS + .MIPS.abiflags (MIPS II, 32-bit GPR, single float),
  - .symtab with every name from config/symbols/<module>.txt (+ .extra.txt),
    so REA/Ghidra show `ehsys_memset`, `sceIoDopen`, ... instead of stubs.

The PRX relocation sections are dropped (base 0 makes them no-ops) and the
output is for analysis only — it is never part of the matching build.

The decrypted EBOOT (build/EBOOT.elf, from scripts/decrypt_eboot.sh) is the
same kind of PRX, so `EBOOT` works too. Its symbols are its 1729 libehsys_rel
exports, named exactly as every module's stubs call them (nids/ehsys.csv
gives each export's address; the module symbol file gives the name), plus its
own SDK imports resolved through nids/sdk.csv.

Usage:
  scripts/prx_to_rea_elf.py rel_movie_viewer            # -> build/rea/rel_movie_viewer.elf
  scripts/prx_to_rea_elf.py EBOOT                       # -> build/rea/EBOOT.elf
  scripts/prx_to_rea_elf.py --all
"""
import argparse
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GMODULE = ROOT / "iso_extracted/PSP_GAME/USRDIR/gmodule"
OUT_DIR = ROOT / "build/rea"

SHT_PROGBITS, SHT_SYMTAB, SHT_STRTAB, SHT_NOBITS = 1, 2, 3, 8
SHT_MIPS_ABIFLAGS = 0x7000002A
PT_LOAD, PT_MIPS_ABIFLAGS = 1, 0x70000003
SHF_WRITE, SHF_ALLOC, SHF_EXEC = 1, 2, 4
STT_OBJECT, STT_FUNC = 1, 2
STB_GLOBAL = 1

# version, isa_level, isa_rev, gpr_size, cpr1_size, cpr2_size, fp_abi,
# isa_ext, ases, flags1, flags2 — the record REA's PSP profile verifies.
ABIFLAGS = struct.pack("<HBBBBBBIIII", 0, 2, 0, 1, 1, 0, 2, 0, 0, 1, 0)

SYM_RE = re.compile(r"^\s*([A-Za-z_][\w.$]*)\s*=\s*(0x[0-9A-Fa-f]+|\d+)\s*;(.*)$")


def read_sections(data):
    shoff, = struct.unpack_from("<I", data, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from("<HHH", data, 0x2E)
    raw = [struct.unpack_from("<IIIIIIIIII", data, shoff + i * shentsize) for i in range(shnum)]
    strtab = raw[shstrndx]
    def name(off):
        start = strtab[4] + off
        return data[start:data.index(b"\0", start)].decode("ascii")
    return [dict(name=name(s[0]), type=s[1], flags=s[2], addr=s[3], offset=s[4],
                 size=s[5], align=s[8]) for s in raw]


def load_symbols(module):
    syms = {}
    for path in (ROOT / f"config/symbols/{module}.txt", ROOT / f"config/symbols/{module}.extra.txt"):
        if not path.exists():
            continue
        for line in path.read_text(encoding="utf-8").splitlines():
            m = SYM_RE.match(line)
            if not m:
                continue
            name, addr, rest = m.group(1), int(m.group(2), 0), m.group(3)
            # Identifiers must stay ASCII: Konami's sources leave Shift-JIS in
            # places, and a non-ASCII byte in a symbol name breaks Ghidra
            # imports and every downstream tool. Never let one through.
            if not name.isascii():
                print(f"warning: skipping non-ASCII symbol {name!r}", file=sys.stderr)
                continue
            syms[name] = (addr, STT_FUNC if "type:func" in rest else STT_OBJECT)
    return syms


def eboot_symbols(data):
    """libehsys_rel exports + the EBOOT's own SDK import stubs."""
    syms = {}
    # stub k of every module is export k of the EBOOT (same NID array), so a
    # module's symbol file names them; rel_movie_viewer's stubs start at 0x8B4
    stub_names = {}
    for line in (ROOT / "config/symbols/rel_movie_viewer.txt").read_text(encoding="utf-8").splitlines():
        m = SYM_RE.match(line)
        if m and m.group(1).startswith("ehsys_"):
            stub_names[int(m.group(2), 0)] = m.group(1)
    base = min(stub_names)
    for line in (ROOT / "nids/ehsys.csv").read_text().splitlines()[1:]:
        idx, nid, vaddr = line.split(",")[:3]
        name = stub_names.get(base + 8 * int(idx), f"ehsys_{int(nid, 16):08X}")
        syms[name] = (int(vaddr, 16), STT_FUNC)

    sdk = {}
    for line in (ROOT / "nids/sdk.csv").read_text().splitlines()[1:]:
        lib, nid, name = line.split(",")[:3]
        sdk[(lib, int(nid, 16))] = name
    secs = {s["name"]: s for s in read_sections(data)}
    stub = secs.get(".lib.stub")
    def at(vaddr, n):
        for s in secs.values():
            if s["type"] == SHT_PROGBITS and s["addr"] <= vaddr < s["addr"] + s["size"] and s["flags"] & SHF_ALLOC:
                off = s["offset"] + vaddr - s["addr"]
                return data[off:off + n]
        return None
    pos = 0
    while stub and pos < stub["size"]:
        e = data[stub["offset"] + pos:]
        name_p, _ver, _attr, entsize, _nvars, nfuncs, nid_p, stub_p = struct.unpack_from("<IHHBBHII", e)
        raw = at(name_p, 64) or b"?"
        lib = raw.split(b"\0")[0].decode("ascii", "replace")
        for i in range(nfuncs):
            nid, = struct.unpack("<I", at(nid_p + 4 * i, 4))
            syms[sdk.get((lib, nid), f"{lib}_{nid:08X}")] = (stub_p + 8 * i, STT_FUNC)
        pos += entsize * 4
    return syms


def convert(module):
    src = ROOT / "build/EBOOT.elf" if module == "EBOOT" else GMODULE / f"{module}.prx"
    data = src.read_bytes()
    if data[:4] != b"\x7fELF":
        raise SystemExit(f"{module}: not a plaintext ELF")
    sections = [s for s in read_sections(data) if s["flags"] & SHF_ALLOC and s["type"] in (SHT_PROGBITS, SHT_NOBITS)]
    sections.sort(key=lambda s: s["addr"])

    image_end = max(s["addr"] + s["size"] for s in sections if s["type"] == SHT_PROGBITS)
    mem_end = max(s["addr"] + s["size"] for s in sections)
    image = bytearray(image_end)
    for s in sections:
        if s["type"] == SHT_PROGBITS:
            image[s["addr"]:s["addr"] + s["size"]] = data[s["offset"]:s["offset"] + s["size"]]

    # Entry point: the module_start export if named, else the start of .text.
    syms = eboot_symbols(data) if module == "EBOOT" else load_symbols(module)
    entry = 0

    # Layout: ELF header | 2 phdrs | abiflags | (pad to 0x100) image | symtab | strtab | shstrtab | shdrs
    ehdr_size, phnum = 52, 2
    abi_off = ehdr_size + 32 * phnum
    img_off = 0x100
    out = bytearray(img_off)
    out[abi_off:abi_off + 24] = ABIFLAGS
    out += image

    def align(n):
        while len(out) % n:
            out.append(0)

    # .symtab / .strtab
    strtab = bytearray(b"\0")
    symtab = bytearray(16)  # null symbol
    sym_entries = []
    for name, (addr, typ) in sorted(syms.items(), key=lambda kv: kv[1][0]):
        sym_entries.append((len(strtab), addr, typ))
        strtab += name.encode("ascii") + b"\0"

    # section numbering: 0 null, 1 .MIPS.abiflags, 2.. alloc sections, then symtab, strtab, shstrtab
    sec_index = {id(s): 2 + i for i, s in enumerate(sections)}
    def section_for(addr):
        for s in sections:
            if s["addr"] <= addr < s["addr"] + max(s["size"], 1):
                return sec_index[id(s)]
        return 0xFFF1  # SHN_ABS

    for name_off, addr, typ in sym_entries:
        symtab += struct.pack("<IIIBBH", name_off, addr, 0, (STB_GLOBAL << 4) | typ, 0, section_for(addr))

    align(4)
    symtab_off = len(out); out += symtab
    strtab_off = len(out); out += strtab

    shstr = bytearray(b"\0")
    def shname(n):
        off = len(shstr); shstr.extend(n.encode("ascii") + b"\0"); return off

    shdrs = [struct.pack("<10I", *([0] * 10))]
    shdrs.append(struct.pack("<10I", shname(".MIPS.abiflags"), SHT_MIPS_ABIFLAGS, SHF_ALLOC, 0, abi_off, 24, 0, 0, 8, 24))
    for s in sections:
        off = img_off + s["addr"] if s["type"] == SHT_PROGBITS else img_off + image_end
        shdrs.append(struct.pack("<10I", shname(s["name"]), s["type"], s["flags"], s["addr"], off, s["size"], 0, 0, s["align"] or 1, 0))
    symtab_idx = len(shdrs)
    shdrs.append(struct.pack("<10I", shname(".symtab"), SHT_SYMTAB, 0, 0, symtab_off, len(symtab), symtab_idx + 1, 1, 4, 16))
    shdrs.append(struct.pack("<10I", shname(".strtab"), SHT_STRTAB, 0, 0, strtab_off, len(strtab), 0, 0, 1, 0))
    shstrndx = len(shdrs)
    shstr_name = shname(".shstrtab")
    shstr_off = len(out); out += shstr
    shdrs.append(struct.pack("<10I", shstr_name, SHT_STRTAB, 0, 0, shstr_off, len(shstr), 0, 0, 1, 0))
    align(4)
    shoff = len(out)
    for h in shdrs:
        out += h

    ident = b"\x7fELF" + bytes([1, 1, 1, 0]) + bytes(8)
    out[0:ehdr_size] = ident + struct.pack("<HHIIIIIHHHHHH", 2, 8, 1, entry, ehdr_size, shoff,
                                           0x10A23001, ehdr_size, 32, phnum, 40, len(shdrs), shstrndx)
    out[ehdr_size:ehdr_size + 32] = struct.pack("<8I", PT_LOAD, img_off, 0, 0, image_end, mem_end, 7, 0x100)
    out[ehdr_size + 32:ehdr_size + 64] = struct.pack("<8I", PT_MIPS_ABIFLAGS, abi_off, 0, 0, 24, 24, 4, 8)

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    dest = OUT_DIR / f"{module}.elf"
    dest.write_bytes(out)
    print(f"{module}: {dest.relative_to(ROOT)} ({len(sym_entries)} symbols, image 0x{image_end:X}, mem 0x{mem_end:X})")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("modules", nargs="*")
    ap.add_argument("--all", action="store_true")
    args = ap.parse_args()
    mods = sorted(p.stem for p in GMODULE.glob("rel_*.prx")) if args.all else args.modules
    if not mods:
        ap.error("name a module or pass --all")
    for m in mods:
        convert(m)


if __name__ == "__main__":
    main()

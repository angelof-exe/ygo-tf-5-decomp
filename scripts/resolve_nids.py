#!/usr/bin/env python3
"""
Resolve every import of every gmodule PRX to a stable, project-wide name.

    scripts/resolve_nids.py            # rebuild nids/*.csv and config/symbols/*.txt

WHY THIS WORKS AT ALL
---------------------
A PSP import is identified by (library name, NID), where the NID is the first
4 bytes of SHA-1(function name), little-endian. Three kinds occur here:

  libehsys_rel  1729 functions, the engine that lives in the encrypted EBOOT.
                Every module links a stub for ALL 1729 exports whether it calls
                them or not, and the NID array is BYTE-IDENTICAL in all 28
                modules and in the EBOOT's own export table (sha1 820088858e31).
                So stub i is the same engine function in every module, and the
                EBOOT's .lib.ent pairs each NID with its address. No external
                data needed.

  lib*_rel      module-to-module imports (rel_labo calls into rel_cardalbum,
                rel_title into nine others, ...). Resolved the same way against
                the owning module's own .lib.ent.

  sce*          real SDK libraries. Names come from a candidate corpus
                (tools/nid_db/*.cpp, PPSSPP's HLE tables) and are then VERIFIED
                by recomputing the NID from the name — a match is proof, not a
                guess, so a wrong corpus can never introduce a wrong name.

NAMING
------
  sce* imports      real SDK name            sceHttpInit
  libehsys_rel      ehsys_<NID>              ehsys_B4471B5E
                    ehsys_<name> when the NID hashes to a known name — the
                    engine re-exports much of the C library and several kernel
                    functions under their real names:   ehsys_memset

The NID is the identity: stable across modules, independent of load address,
and renameable in one place (nids/ehsys.csv `name` column) once a function is
actually identified — everything regenerates from there.

OUTPUTS
-------
  nids/sdk.csv        lib, nid, name                      (47 rows, all verified)
  nids/ehsys.csv      index, nid, eboot_vaddr, callers, name
  nids/modules.csv    lib, nid, owner, export_vaddr, callers, name
  config/symbols/<module>.txt   splat symbol_addrs: stub vram -> name
"""
import csv
import glob
import hashlib
import os
import re
import struct
import subprocess
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GMODULE = os.path.join(ROOT, "iso_extracted/PSP_GAME/USRDIR/gmodule")
EBOOT = os.path.join(ROOT, "build/EBOOT.elf")
FILE_BASE = 0x54  # vaddr -> file offset for these single-PT_LOAD images


# Candidate names for the C library and kernel functions the ENGINE re-exports
# under their real names. Anything here is still hash-verified before use, so a
# wrong guess in this list can only fail to match — it can never mis-name
# anything. Extending this list is the cheapest way to name more of the 1729.
EXTRA_CANDIDATES = """
strcpy strncpy strcat strncat strcmp strncmp strchr strrchr strstr strlen strtok strdup strspn strcspn strpbrk
memset memcpy memmove memcmp memchr malloc calloc realloc free sprintf snprintf vsprintf vsnprintf sscanf printf puts
atoi atol atof strtol strtoul strtod abs labs rand srand qsort bsearch toupper tolower isalpha isdigit isspace
sqrt sqrtf sin cos tan sinf cosf tanf atan2 atan2f fabs fabsf floor ceil pow powf exp log fmod fmodf
sceKernelChangeCurrentThreadAttr sceKernelCreateThread sceKernelStartThread sceKernelTerminateDeleteThread
sceKernelDelayThread sceKernelExitThread sceKernelSleepThread sceKernelGetThreadCurrentPriority sceKernelGetThreadId
sceKernelCreateSema sceKernelWaitSema sceKernelSignalSema sceKernelDeleteSema sceKernelGetSystemTimeWide
sceIoOpen sceIoClose sceIoRead sceIoWrite sceIoLseek sceIoDopen sceIoDread sceIoDclose sceIoGetstat sceIoRemove sceIoMkdir
sceUtilityOskInitStart sceUtilityOskUpdate sceUtilityOskGetStatus sceUtilityOskShutdownStart
sceGuStart sceGuFinish sceGuSync sceGuSwapBuffers sceDisplayWaitVblankStart
""".split()


def nid_of(name):
    return struct.unpack("<I", hashlib.sha1(name.encode()).digest()[:4])[0]


def sections(path):
    out = subprocess.run(["readelf", "-SW", path], capture_output=True, text=True).stdout
    r = {}
    for line in out.splitlines():
        m = re.match(r"\s*\[\s*\d+\]\s+(\S+)\s+\S+\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)", line)
        if m:
            r[m.group(1)] = (int(m.group(2), 16), int(m.group(3), 16), int(m.group(4), 16))
    return r


def cstr(d, vaddr):
    o = vaddr + FILE_BASE
    return d[o:d.index(b"\0", o)].decode(errors="replace")


def parse_stubs(d, off, size):
    """.lib.stub -> [(lib, funcs, nid_vaddr, stub_vaddr)]"""
    out, i = [], 0
    while i < size:
        name, _vf, lenv, nidp, stubp = struct.unpack_from("<IIIII", d, off + i)
        ln, fc = lenv & 0xFF, (lenv >> 16) & 0xFFFF
        out.append((cstr(d, name), fc, nidp, stubp))
        i += ln * 4
    return out


def parse_ents(d, off, size):
    """.lib.ent -> [(lib, funcs, vars, entry_vaddr)]"""
    out, i = [], 0
    while i < size:
        name, _ver, _attr, ln, vc, fc, ent = struct.unpack_from("<IHHBBHI", d, off + i)
        out.append((cstr(d, name) if name else "(syslib)", fc, vc, ent))
        i += ln * 4
    return out


def exports(d, ents, lib):
    """lib's exports as [(nid, vaddr)] — the entry table is fcount+vcount NIDs
    followed by fcount+vcount addresses."""
    for name, fc, vc, ent in ents:
        if name == lib:
            nids = struct.unpack_from(f"<{fc}I", d, ent + FILE_BASE)
            addrs = struct.unpack_from(f"<{fc}I", d, ent + FILE_BASE + 4 * (fc + vc))
            return list(zip(nids, addrs))
    return []


def manual_ehsys_names():
    """Hand-identified engine functions from nids/ehsys.names.csv.

    Unlike the sce* names these are NOT hash-verifiable — the engine's original
    symbol names are gone, so a NID can never be re-derived from a name we chose.
    They are inferences backed by the evidence recorded in that file, which is why
    it carries a `confidence` column. Editing that file and re-running renames the
    function across all 28 modules at once."""
    path = os.path.join(ROOT, "nids/ehsys.names.csv")
    out = {}
    if os.path.exists(path):
        for row in csv.DictReader(open(path)):
            if row.get("name"):
                out[int(row["nid"], 16)] = row["name"]
    return out


def sdk_names():
    """NID -> name from the candidate corpus, each one re-hashed to prove it."""
    names = set()
    for f in glob.glob(os.path.join(ROOT, "tools/nid_db/*.cpp")):
        t = open(f, errors="replace").read()
        names |= set(re.findall(r'"([A-Za-z_][A-Za-z0-9_]{3,})"', t))
        names |= set(re.findall(r"\b([A-Za-z_][A-Za-z0-9_]{3,})\s*\(", t))
    table = {}
    for n in names:
        table.setdefault(nid_of(n), n)
    for n in EXTRA_CANDIDATES:
        table.setdefault(nid_of(n), n)
    return table


def called_stubs(module, vram_name):
    """Which stub addresses .text actually calls.

    Must handle BOTH spellings: a module split before this script ran calls
    `jal func_00001D30`, while one split afterwards calls `jal ehsys_memset`
    (the config now feeds splat config/symbols/<module>.txt). Counting only the
    first spelling silently undercounts every module that has been re-split."""
    p = os.path.join(ROOT, "asm", module, "text.s")
    hits = Counter()
    if not os.path.exists(p):
        return hits
    by_name = {n: v for v, n in vram_name.items()}
    for line in open(p, errors="replace"):
        m = re.search(r"\b(?:jal|j)\s+(\w+)", line)
        if not m:
            continue
        op = m.group(1)
        if op in by_name:
            hits[by_name[op]] += 1
        elif re.match(r"^func_[0-9A-F]+$", op):
            v = int(op[5:], 16)
            if v in vram_name:
                hits[v] += 1
    return hits


def main():
    mods = sorted(f[:-4] for f in os.listdir(GMODULE) if f.endswith(".prx"))
    sdk = sdk_names()
    manual = manual_ehsys_names()

    # --- the engine's export table, from the decrypted EBOOT
    ed = open(EBOOT, "rb").read()
    es = sections(EBOOT)
    ents = parse_ents(ed, es[".lib.ent"][1], es[".lib.ent"][2])
    ehsys = exports(ed, ents, "libehsys_rel")
    assert len(ehsys) == 1729, f"expected 1729 libehsys_rel exports, got {len(ehsys)}"
    ehsys_index = {n: i for i, (n, _) in enumerate(ehsys)}

    # --- every module's own exports, so lib*_rel imports resolve too
    mod_exports = {}   # lib -> {nid: (owner, vaddr)}
    for m in mods:
        path = os.path.join(GMODULE, m + ".prx")
        d = open(path, "rb").read()
        s = sections(path)
        if ".lib.ent" not in s:
            continue
        for name, fc, vc, ent in parse_ents(d, s[".lib.ent"][1], s[".lib.ent"][2]):
            if name.endswith("_rel"):
                nids = struct.unpack_from(f"<{fc}I", d, ent + FILE_BASE)
                addrs = struct.unpack_from(f"<{fc}I", d, ent + FILE_BASE + 4 * (fc + vc))
                mod_exports.setdefault(name, {}).update(
                    {n: (m, a) for n, a in zip(nids, addrs)})

    def name_for(lib, nid):
        if not lib.endswith("_rel"):
            return sdk.get(nid, f"{lib}_{nid:08X}")
        if lib == "libehsys_rel":
            # The engine re-exports much of the C library and several kernel
            # functions under their REAL names, so a NID here often hashes to a
            # known name. Keep the ehsys_ prefix anyway: these are the engine's
            # exports, not the SDK's, and a bare `memset` would let MWCC expand
            # its own builtin instead of emitting the call we need to match.
            hit = sdk.get(nid) or manual.get(nid)
            return f"ehsys_{hit}" if hit else f"ehsys_{nid:08X}"
        short = lib[3:-4] if lib.startswith("lib") else lib
        return f"{short}_{nid:08X}"

    eh_callers = Counter()
    rel_callers = Counter()
    sdk_rows = {}
    unresolved_sdk = []
    os.makedirs(os.path.join(ROOT, "config/symbols"), exist_ok=True)

    for m in mods:
        path = os.path.join(GMODULE, m + ".prx")
        d = open(path, "rb").read()
        s = sections(path)
        stubs = parse_stubs(d, s[".lib.stub"][1], s[".lib.stub"][2])
        vram_name, vram_meta = {}, {}
        # 26 of libehsys_rel's 1729 exports are listed TWICE (both entries point
        # at the same EBOOT address — genuine aliases), so a plain NID name is
        # not unique. Suffix repeats by occurrence order; the NID array is
        # identical in every module, so the same stub gets the same suffix
        # everywhere and the names stay cross-module stable.
        used = Counter()
        for lib, fc, nidp, stubp in stubs:
            nids = struct.unpack_from(f"<{fc}I", d, nidp + FILE_BASE)
            for k, n in enumerate(nids):
                v = stubp + k * 8
                nm = name_for(lib, n)
                used[nm] += 1
                if used[nm] > 1:
                    nm = f"{nm}_alias{used[nm]}"
                vram_name[v] = nm
                vram_meta[v] = (lib, n)
                if not lib.endswith("_rel"):
                    if n in sdk:
                        sdk_rows[(lib, n)] = sdk[n]
                    else:
                        unresolved_sdk.append((lib, n))
        for v, c in called_stubs(m, vram_name).items():
            lib, n = vram_meta[v]
            if lib == "libehsys_rel":
                eh_callers[n] += c
            elif lib.endswith("_rel"):
                rel_callers[(lib, n)] += c

        # Hand-written names (data symbols, renamed engine functions, anything a
        # human worked out) live in <module>.extra.txt and are MERGED IN here, so
        # re-running this script never destroys them. That file is the one to edit.
        extra_path = os.path.join(ROOT, "config/symbols", m + ".extra.txt")
        extra = open(extra_path).read() if os.path.exists(extra_path) else ""
        with open(os.path.join(ROOT, "config/symbols", m + ".txt"), "w") as fh:
            fh.write(f"// GENERATED by scripts/resolve_nids.py — do not edit.\n"
                     f"// Hand-written names belong in {m}.extra.txt, which is merged in below.\n"
                     "// Import names are NID-derived and therefore IDENTICAL across all 28\n"
                     "// modules: the same ehsys_XXXXXXXX is the same engine function everywhere.\n")
            for v in sorted(vram_name):
                fh.write(f"{vram_name[v]} = 0x{v:08X}; // type:func\n")
            if extra:
                fh.write(f"\n// ---- merged from {m}.extra.txt ----\n")
                fh.write(extra if extra.endswith("\n") else extra + "\n")

    os.makedirs(os.path.join(ROOT, "nids"), exist_ok=True)
    with open(os.path.join(ROOT, "nids/sdk.csv"), "w", newline="") as fh:
        w = csv.writer(fh)
        w.writerow(["lib", "nid", "name", "verified"])
        for (lib, n), nm in sorted(sdk_rows.items()):
            w.writerow([lib, f"0x{n:08X}", nm, "yes" if nid_of(nm) == n else "NO"])
        for lib, n in sorted(set(unresolved_sdk)):
            w.writerow([lib, f"0x{n:08X}", "", "unresolved"])

    with open(os.path.join(ROOT, "nids/ehsys.csv"), "w", newline="") as fh:
        w = csv.writer(fh)
        w.writerow(["index", "nid", "eboot_vaddr", "call_sites", "name"])
        for n, a in ehsys:
            w.writerow([ehsys_index[n], f"0x{n:08X}", f"0x{a:08X}", eh_callers.get(n, 0),
                        sdk.get(n) or manual.get(n, "")])

    with open(os.path.join(ROOT, "nids/modules.csv"), "w", newline="") as fh:
        w = csv.writer(fh)
        w.writerow(["lib", "nid", "owner", "export_vaddr", "call_sites", "name"])
        for lib in sorted(mod_exports):
            for n, (owner, a) in sorted(mod_exports[lib].items()):
                w.writerow([lib, f"0x{n:08X}", owner, f"0x{a:08X}",
                            rel_callers.get((lib, n), 0), ""])

    print(f"modules: {len(mods)}")
    print(f"libehsys_rel: 1729 exports, {len(eh_callers)} called project-wide")
    print(f"sdk: {len(sdk_rows)} resolved, {len(set(unresolved_sdk))} unresolved")
    print(f"inter-module libs: {len(mod_exports)}, {len(rel_callers)} exports called")
    print("wrote nids/*.csv and config/symbols/*.txt")


if __name__ == "__main__":
    main()

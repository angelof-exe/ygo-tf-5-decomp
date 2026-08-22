---
name: tf5-decomp
description: >
  Orientation and workflow for the matching decompilation of Yu-Gi-Oh! 5D's Tag
  Force 5 (PSP, ULES-01474). Activate this skill whenever working in this repository on
  decompilation, reverse engineering, PRX/EBOOT module analysis, binary splitting,
  function matching, or the game's file formats. Triggers: "decomp", "decompile",
  "matching", "PRX", "EBOOT", "Allegrex", "splat", "duel_eng", "Tag Force", "modehsys",
  "PSP reverse engineering".
---

# Tag Force 5 decomp — operating guide

Project: matching reconstruction of the code of **Yu-Gi-Oh! 5D's Tag Force 5**
(PSP, MIPS Allegrex, Konami 2010, disc **ULES-01474**).

## Project facts NOT to rediscover every time

- **Original compiler = Metrowerks CodeWarrior `MW MIPS C Compiler (2.4.1.01)`**
  (from the `.comment` section of every PRX). Matching uses **`mwccpsp`**, not psp-gcc.
  On decomp.me the PSP builds use the product names **MWCC 1.0 … 1.3 SP7** (internal
  builds 3.0.1_121…219); `2.4.1.01` is a different numbering axis, do NOT look for it
  in the list. Locally it runs via **wibo**. Details in `docs/09-first-match.md`.
- **CONFIRMED config**: compiler **MWCC 1.3 SP7 (mwccpsp_3.0.1_219)**, flags
  **`-O4,s -sdatathreshold 0`** (SIZE, not `,p` — corrected 2026-08-16;
  `func_00000034` is the only function that discriminates: 100% on `,s`, 63/65
  words on `,p`, while the other 15 are byte-identical either way. `-O3,s` gives a
  byte-identical object, so the LEVEL is not pinned, only speed-vs-size).
  `-sdatathreshold 0` = absolute addressing (not gp-relative). Confirmed on 15 real
  functions of `rel_movie_viewer`, not just the trivial empty one.
- **Matching works fully locally now, no decomp.me account needed**:
  `scripts/mwcc_build.sh src/file.c` (wibo + real mwccpsp_3.0.1_219, both
  auto-fetched by `setup_tools.sh`) then `scripts/mwcc_diff.py asm/.../text.s
  build/mwcc/file.o` — relocation-aware, treats a word as matching if either the
  raw bytes are equal or both sides reference the same relocation (HI16/LO16/26)
  against the same symbol (unlinked `.o`s have zeroed placeholders; some globals
  are baked as absolute constants in the shipped PRX with NO relocation at all —
  see `docs/06-splitting-and-matching.md` "Local matching" for the full story
  and the `KNOWN_ADDR` table to extend). Needs `binutils-mips-linux-gnu` (`sudo
  apt install`) for a MIPS-aware `objdump`.
- **Technique**: game **state globals are `volatile`**; to match, access them via a
  local pointer: `volatile int *p = &G; if (*p) { ... *p ...; *p = 0; }` — this
  reproduces the value reload + use of saved registers (`$s0`). Full case in
  `docs/09-first-match.md`.
- **The 28 `USRDIR/gmodule/rel_*.prx` modules are PLAINTEXT Allegrex ELF**: analyzable
  immediately, no decryption needed. Convention: `rel_X.prx` = module `modX`, exports
  `libX_rel`, imports `libehsys_rel` from the main engine.
- **`SYSDIR/EBOOT.BIN` is ENCRYPTED** (magic `~PSP`, module `modehsys`). `BOOT.BIN` is a
  zeroed dummy (NOT the plaintext ELF). Decrypt it with pspdecrypt or PPSSPP.
- **No Tag Force decomp has ever existed**: we're the first. Card logic is **hardcoded**
  in `rel_duel_eng.prx`, not in data → decompiling the code is the only way to really
  understand it.
- **ALL IMPORTS ARE NAMED** (`docs/nids/README.md`, `scripts/resolve_nids.py`). Calls out
  of a module are `sceHttpInit`, `ehsys_memset`, `ehsys_B4471B5E` — not stub addresses.
  The 1729-entry `libehsys_rel` NID array is byte-identical in all 28 modules AND in the
  EBOOT export table (`sha1 820088858e31`), so a name is the SAME engine function
  everywhere; `ehsys_B4471B5E` is the module-registration call that opens every module.
  NID = first 4 bytes of SHA-1(name), little-endian, so any candidate name is provable.
- **A module can be RELINKED and checksum-verified**: `make MODULE=rel_html_view`
  (from asm) prints OK for a correct splat config; `SRC=1` swaps in our compiled C.
  Needs `emit_subalign: False` and a per-module `asset_path` in the config —
  SUBALIGN(16) pads every blob and a shared `assets/` dir means all 28 modules
  overwrite each other's data.
- **Local toolchain already installed and verified**: splat64 (platform `psp`),
  spimdisasm, rabbitizer (category `R4000ALLEGREX`, with VFPU), in `.venv`;
  asm-differ, m2c, decomp-permuter, pspdecrypt, wibo, mwccpsp_3.0.1_219 in
  `tools/`; Ghidra 12.0 + ghidra-allegrex installed system-wide.

## Hard repository rules

- **NEVER commit** ISOs, `.prx`, EBOOT, extracted assets, `iso_extracted/`, `build/`,
  `asm/`. The `.gitignore` already excludes them. Only `src/`, `config/`, `scripts/`,
  `docs/` (and the git-ignored `tools/`) belong in git.
- Mods are distributed as **xdelta patches**, never as complete ISOs.

## The pipeline (details in docs/)

```
ISO ──scripts/extract_iso.sh──► iso_extracted/
EBOOT.BIN (~PSP) ──scripts/decrypt_eboot.sh──► build/EBOOT.elf   (needs libssl-dev)
plaintext PRX / EBOOT.elf
   ──Ghidra+ghidra-allegrex / prxtool──►  analysis, NIDs, function names
   ──splat (config/*.yaml)──►  asm/  +  linker script
   ──m2c──►  draft src/*.c
   ──mwccpsp + asm-differ / decomp.me──►  byte-for-byte match
   ──make + sha1sum vs checksums.sha1──►  verify
```

## How to attack a new module (checklist)

1. `readelf -SW module.prx` → note the section layout (.text/.sceStub.text/.rodata.sceNid/.data/.bss).
2. Write/adapt a splat config (start from `config/rel_movie_viewer.yaml`, a complete
   section-accurate example covering 100% of the file — see its header comments for
   the byte-offset table and a linker-script-ordering gotcha).
3. `splat split config/module.yaml` → generates `asm/`.
4. Imports are ALREADY RESOLVED, project-wide — do not re-derive them per module.
   `scripts/resolve_nids.py` names every import of every module; `config/symbols/<module>.txt`
   feeds splat so the disassembly calls them by name. See `docs/nids/README.md`.
5. NID MATCHING IS PART OF ANALYSING EVERY FUNCTION — not a separate phase.
   Before reasoning about what a function does, resolve the imports it calls:
     - `grep ' = 0xXXXXXXXX;' config/symbols/<module>.txt` for a stub address, or
       look the NID up in `nids/ehsys.csv` / `nids/sdk.csv` / `nids/modules.csv`;
     - a real SDK name usually identifies the STRUCTS and CONSTANTS too (this is how
       rel_html_view's 0xA8 blob turned out to be `pspUtilityHtmlViewerParam`, and how
       `0x100..0x106` turned out to be the net/http/ssl module ids);
     - `ehsys_<NID>` with no name yet: check `nids/ehsys.csv` for its EBOOT address and
       its project-wide call count. A high count means naming it pays off 28 modules over;
       add a candidate to `EXTRA_CANDIDATES` in `scripts/resolve_nids.py` (hash-verified,
       so a wrong guess can only fail, never mis-name), or identify it in `build/EBOOT.elf`.
   Record what you learn in `nids/*.csv`, NOT only in the one `src/*.c` — the same name is
   used by all 28 modules, so one edit renames it everywhere.
6. For each function: `m2c` → draft C → iterate until the diff is empty, verifying
   either on decomp.me OR **locally** with `scripts/mwcc_build.sh` +
   `scripts/mwcc_diff.py` (wibo + real mwccpsp_3.0.1_219, both fetched by
   `setup_tools.sh` — no decomp.me account needed) → mark "MATCH 100%" in the
   function's comment.
7. Rebuild and verify: `make MODULE=<name>` relinks the module from asm + the
   data blobs and checks its sha1 against `checksums.sha1`; `make MODULE=<name>
   SRC=1` does the same with YOUR compiled C in place of the disassembly, and
   only passes once every function matches. `SRC=1` printing OK is the
   definition of done for a module. When it fails it names the first differing
   byte, which points straight at the offending function.

## Recommended module order

`rel_movie_viewer` (26 KB, to shake down the pipeline) → other small modules
(`rel_html_view`, `rel_soundtest`) → **`modehsys`** (EBOOT: shared engine, unlocks
`libehsys_rel`) → **`rel_duel_eng`** (4.3 MB: the rules engine, the heart of the game).

## Full documentation

- `docs/01-overview.md` — what a matching decomp is, expectations, legal notes
- `docs/02-iso-analysis.md` — ISO structure, modules, compiler
- `docs/03-tools.md` — toolchain installation
- `docs/04-extraction-and-decryption.md` — extracting the ISO, decrypting the EBOOT
- `docs/05-ghidra.md` — Ghidra + ghidra-allegrex, prxtool, NIDs
- `docs/06-splitting-and-matching.md` — splitting, m2c, asm-differ, decomp.me, mwccpsp
- `docs/07-file-formats.md` — EHP/CIP/card DB/audio/models + community tools
- `docs/08-resources.md` — links, community, reference project
- `docs/09-first-match.md` — tutorial: first match on decomp.me (empty `func_000001C0`
  and `func_00000184` in rel_movie_viewer; m2c C draft included)

Related skills: **psp-allegrex-tools** (RE toolchain), **tf5-formats** (assets).

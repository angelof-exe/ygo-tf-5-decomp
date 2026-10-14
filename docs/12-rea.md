# REA: agent-driven analysis on top of Ghidra

[REA](https://github.com/morluto/rea) ("reverse engineer anything with agents")
exposes Ghidra's analysis — pseudocode, xrefs, strings, callers/callees — as
CLI commands and as an MCP server. In this project it is a **reading aid**: it
helps you (or an agent) understand what a function does before writing the C
that has to match. It does not replace splat, m2c, mwccpsp or the sha1 check.

## Setup

REA needs Node >= 22.19 (or 24.11+), a Ghidra **12.1.x** install and a full
JDK 21. PSP support comes from the ghidra-allegrex extension (v21.4 is built
for Ghidra 12.1.3). Nothing needs root:

```bash
# Node 24 via nvm, then REA
nvm install 24 && npm install --global rea-agents

# Ghidra 12.1.3 + ghidra-allegrex v21.4 + Temurin JDK 21, e.g. under ~/.local/opt
#   https://github.com/NationalSecurityAgency/ghidra/releases  (Ghidra_12.1.3_build)
#   https://github.com/kotcrab/ghidra-allegrex/releases       (v21.4) -> unzip into Ghidra/Extensions/
#   https://api.adoptium.net/v3/binary/latest/21/ga/linux/x64/jdk/hotspot/normal/eclipse

export GHIDRA_INSTALL_DIR=~/.local/opt/ghidra_12.1.3_PUBLIC
export JAVA_HOME=~/.local/opt/jdk-21...      # a JDK, not a JRE: REA checks
rea doctor --provider ghidra --json          # "healthy": true
```

Keep those exports in a local, untracked `.rea_env.sh` and `source` it.

## Why the PRXs need rewrapping

REA's PSP lane is deliberately strict. It admits only a **static ET_EXEC**
ELF32 with the PSPSDK declaration (`e_flags` 0x10a23001) and a 24-byte MIPS
ABI flags record. Our gmodule PRXs are relocatable (`e_type` 0xFFA0, flags
0x10a23000, no ABI record), so it rejects them:

> The PSP Allegrex profile supports static ET_EXEC only; PRX, shared and
> relocatable images require separate verification.

`scripts/prx_to_rea_elf.py` gets around this without losing anything. A PRX
loaded at base 0 needs no relocation (every reloc adds the base, and the base
is 0), so its section bytes already are the image at vram 0. That is also
splat's vram, so **REA addresses equal the `func_XXXXXXXX` names**. The script
copies the allocated sections into a new ET_EXEC with the required header and
ABI record. It also adds a `.symtab` built from `config/symbols/<module>.txt`
(+ `.extra.txt`), so Ghidra calls imports `ehsys_memset`, `sceIoDopen`, …
rather than stub addresses. Non-ASCII names are refused (see
`docs/11-japanese-text.md`).

```bash
scripts/prx_to_rea_elf.py rel_movie_viewer   # -> build/rea/rel_movie_viewer.elf
scripts/prx_to_rea_elf.py --all
```

REA then reports `loader: PspElfLoader`, `language_id: Allegrex:LE:32:default`.

## Using it

One function from the CLI (each call re-imports into Ghidra, ~13 s):

```bash
rea decompile $PWD/build/rea/rel_movie_viewer.elf 0x6bc --provider ghidra --json
rea xrefs     $PWD/build/rea/rel_movie_viewer.elf 0x5aa4 --provider ghidra --json
```

A whole module in one Ghidra session (MCP `batch_decompile`). This is how to
seed work on a module:

```bash
scripts/rea_dump.py rel_labo     # -> build/rea/rel_labo/<addr>.c + index.tsv
```

`rea_dump.py` asks for Ghidra's procedures plus every `glabel func_` in
`asm/<module>/text.s`. Ghidra sometimes merges a function that ends in a tail
call into its neighbour, and splat's boundaries are the ones `src/` uses. An
address that is not a Ghidra procedure entry is listed as `missing` in
`index.tsv`.

## What the pseudocode is (and is not)

`build/rea/rel_movie_viewer/000006bc.c`, verbatim from Ghidra:

```c
uVar1 = ehsys_sceIoDopen();
while ((iVar2 = ehsys_sceIoDread(uVar1,local_160), 0 < iVar2 && (_DAT_000a3f08 < 100))) {
  if ((local_160[0] & 0x1000) == 0) {
    if ((local_160[0] & 0x2000) != 0) {
      iVar2 = _DAT_000a3f08 * 0x100 + 0x9db08;
      ...
```

That is enough to see a recursive directory walk collecting up to 100 file
paths of 0x100 bytes each. It is **not** the source: its types, globals and
control flow are not what MWCC was given, so compiling it will not match. Use
it to understand the function. Then write the C against the asm (m2c draft →
`scripts/mwcc_build.sh` → `scripts/mwcc_diff.py`) as described in
`docs/06-splitting-and-matching.md`.

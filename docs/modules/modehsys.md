# modehsys — the EBOOT (shared engine)

`PSP_GAME/SYSDIR/EBOOT.BIN`, decrypted to `build/EBOOT.elf` by
`scripts/decrypt_eboot.sh` (sha1 `cc92ddd45f550ac1b906842d587c883fba22be34`).
Module name `modehsys`. It exports `libehsys_rel`, the 1729 functions every
gmodule PRX imports, so a function named here is named in all 28 modules.

## Split and relink

```bash
scripts/decrypt_eboot.sh                       # build/EBOOT.elf
scripts/prx_to_rea_elf.py --splat-symbols EBOOT  # config/symbols/modehsys.txt + build/rea/EBOOT.elf
splat split config/modehsys.yaml               # asm/modehsys/
make MODULE=modehsys                           # OK: relinks to the sha1 above
```

`make MODULE=modehsys` rebuilds the whole 1,575,180-byte file from the
disassembly plus the data blobs and matches the shipped sha1.

Two things were needed that no gmodule PRX had exercised:

- **Allegrex `clz`/`madd`** (13 of them). On Allegrex these sit in the SPECIAL
  opcode; MIPS32 puts them in SPECIAL2. gas assembles them without an error
  into the wrong bytes, so `scripts/asm_prepare.py` now always emits
  `clz`, `clo`, `madd`, `maddu`, `msub`, `msubu` as `.word`.
- **Several names per address.** Some exports *are* an SDK import stub
  (`ehsys_sceIoMkdir` is the `sceIoMkdir` trampoline) and some functions are
  exported under more than one NID. splat accepts one name per address, so
  the symbol file keeps the most telling one: a real name over a bare NID,
  then the `ehsys_` spelling the modules use.

## Shape

| | |
|---|---|
| `.text` | 0x6DE7C bytes (450 KB), vram 0x0 |
| functions | 2,726: 1,564 already named (`ehsys_*` exports), 1,162 `func_XXXXXXXX` |
| small (<= 16 words) | 1,305, about half; median 17 words |
| `.sceStub.text` | the EBOOT's own SDK imports, named through `nids/sdk.csv` |
| `.data` | 0xDFE38 bytes (900 KB); MWCC keeps the constants here too |
| `.bss` | declared size 0; real extent from PT_LOAD memsz, vram 0x152E30-0x59A600 (4.4 MB) |

## Japanese text

`scripts/sjis_strings.py EBOOT` reports 11 candidates, all binary data that
happens to decode as cp932. No real Japanese text was found in the engine.

## Where to start

- The 1,305 small functions, many of them getters and setters behind the
  `ehsys_*` exports. Matching one also documents an import used by every
  module.
- Exports that the modules call most (`call_sites` in `nids/ehsys.csv`) and
  that are still `ehsys_<NID>`: decompiling one usually reveals its name.
- REA pseudocode: `rea decompile $PWD/build/rea/EBOOT.elf 0xADDR --provider ghidra`,
  or `scripts/rea_dump.py EBOOT` for all of it (docs/12-rea.md).

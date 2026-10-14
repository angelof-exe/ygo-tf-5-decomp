# rel_password.prx — module survey

The Lab's **password machine**: type an 8-digit card password on a keypad, the
digits light up on eight nixie tubes, a valid password puts the card in your
save. **Fully decompiled**: all 41 functions of `src/rel_password.c` match, and
`make MODULE=rel_password SRC=1` reproduces the shipped sha1. The sections from
"Identity" down are the original mechanical survey
(`scripts/module_survey.py`); the hand-verified findings come first.

## Status

| | |
|---|---|
| functions | 41 / 41 MATCH 100% (0x37B8 bytes of .text) |
| `make MODULE=rel_password SRC=1` | **OK**, sha1 `d0b6b3fd…` |
| names | proposed in `config/symbols/rel_password.extra.txt` (data applied, functions as comments) |
| Shift-JIS text | none in the module (UI text is in `strTbl_<lang>.bin` inside the archive) |

## Password -> card

All of it is in `func_0000241C` (mode 3, "check"), step 10:

1. `value = d0*10^7 + d1*10^6 + ... + d7` — the eight entered digits
   (`PwState.digits`, one per byte) read as a decimal number, leading zeros
   allowed. It is kept in `PwState.password` for the result messages.
2. `card = ehsys_ACAC5C40(value)` — the actual password lookup lives in the
   engine (EBOOT `0x0002C560`). This module is its **only caller in the whole
   game** (`nids/ehsys.csv`, call_sites = 1), so it is the password table
   lookup and nothing else. It returns a card id or an out-of-range value.
3. The card is accepted only if **all** of these hold:
   - `0x0E10 <= card < 0x23FF` (3600..9214, the internal card-id range);
   - `ehsys_28F7224B(card) != 0` (card exists / is valid; 33 callers
     project-wide);
   - `card` is **not** one of 18 ids hard-coded at `.data` 0x8C08
     (`func_0000337C`): 0x1445, 0x1387, 0x2352, 0x1AB2, 0x1388, 0x22BC,
     0x0EA8, 0x1386, 0x22A0, 0x0E42, 0x0E12, 0x22A1, 0x0ECE, 0x0E62, 0x2188,
     0x2189, 0x218A, 0x218B (decimal 5189, 4999, 9042, 6834, 5000, 8892, 3752,
     4998, 8864, 3650, 3602, 8865, 3790, 3682, 8584..8587). These are cards
     whose printed password must not work in this game.
4. Accepted: `PwState.card_id = card`, then
   `ehsys_8C5B148E(PwState.handle, card, ehsys_B401BB25(card))` gives the card
   (the third argument is computed per card by the engine), mode 4 (success).
   Rejected: mode 5 (message 0xA formatted with the password, then 0xB).

Nothing else gates the result in this module: no "already owned" check and no
progression check happen here; if they exist they are inside the three engine
calls. Next step for the full picture: decompile EBOOT `0x2C560`
(`ehsys_ACAC5C40`) to find the password table it searches.

## How the module works

`PwState` (0x1150 bytes at .bss 0x1DC50) holds everything; see
`src/rel_password.c` for the field list. `func_00001688` (exported update)
runs a mode machine:

| mode | function | what happens |
|---|---|---|
| 0 | `func_00001128` | load `labo/password.ehp` (async), set up the card renderer |
| 1 | `func_00001A1C` | type out the intro message while random keys flash |
| 2 | `func_00001D54` | keypad input (see below) |
| 3 | `func_0000241C` | "checking": light the tubes one by one, then look up the password |
| 4 | `func_000027A8` | success: message, card name typed out, card picture "tunes in" (`func_00000D8C`) |
| 5 | `func_00002D68` | no card for this password |
| 6 | `func_000030D4` | quit prompt (cancel pressed on the keypad) |
| 7 | — | fade out |
| 8 | `func_00001864` | unload; update returns 1 |

Keypad (`func_00003610`): row 0 = OK (key 12), row 1 = clear-all (10) / 0 /
delete-last (11), rows 2..4 = 1..9. Confirm presses the key under the cursor,
button mask 0x8000 also deletes the last digit, cancel opens the quit prompt.
After 8 digits the cursor jumps to OK and only rows 0/1 stay reachable.
Each nixie tube is its own copy of `nixi.ehf` (`func_00001358` duplicates
resources 0x1011..0x1018 on the heap) so the eight tubes animate
independently; glyphs are 0..9, 10 = blank, 12 = blinking cursor
(`func_0000354C`).

Resources (`.data` 0x8ADC, 17 x 16 bytes, qsorted by id; `%c` = language
letter from `"jegfis"` = Japanese/English/German/French/Italian/Spanish):

| id | file | used for |
|---|---|---|
| 0x1001 | bg.gim | background |
| 0x1002 | monitor_shadow.gim | monitor shadow |
| 0x1003 / 0x1004 | icon01.gim / icon03.gim | card-drawing resources, handed to `ehsys_1CC5878C` by `func_00001128` |
| 0x1005 / 0x1006 | frame.gim / spelltrap_%c.gim | card-drawing resources (same call) |
| 0x1010 | key_%c.ehf | keypad animation (30 frames per key) |
| 0x1011..0x1018 | nixi.ehf + nixi.tmf | the eight tubes |
| 0x1019 | score-atkdef10.ehf | card-drawing resources (same call) |
| 0x1020 | strTbl_%c.bin | UI strings (1 intro, 2 card-name line, 3/4 success, 5..8 button prompts, 9 quit, 0xA/0xB wrong password, 0xC checking) |

## Matching notes (details in the header of `src/rel_password.c`)

- `make SRC=1` keeps `.data` from the shipped module and only places this
  object's `.rodata` at the first jump table. The source therefore emits no
  `.data`/`.bss`, and a 12-byte const shim keeps the 2nd/3rd jump tables at
  0x89F8/0x8A1C (the shipped `.data` has 0.05f, 0.1f and `module_sdk_version`
  between the 1st and 2nd tables, which one MWCC translation unit cannot
  produce: the original module was most likely several source files).
- `scripts/mwcc_diff.py` reports false mismatches for 17 of the functions:
  when only the candidate has a relocation it compares the target immediate
  against the symbol address WITHOUT the addend that a REL `.o` stores in
  the instruction itself, so every `D_0001DC50.field` access (addend =
  field offset) fails the baked-constant check. Linking first (as `make
  SRC=1` does) shows they are byte-identical. Fix: add the candidate's
  own sign-extended `%lo` immediate (or, for HI16, the paired LO16) to
  `addr` in the baked-constant branch.

## Identity

| | |
|---|---|
| file size | 46776 bytes (0xB6B8) |
| sha1 | `d0b6b3fdbe8d5625304def63361593244d3de21b` |
| .text | 0x37B8 bytes |
| import stubs | 1729 (.sceStub.text 0x3608) |
| .data | 0x278 bytes |
| NID table | 0x1B04 bytes (1729 entries) |

Library names in `.rodata.sceResident` (this module's export name
first, then the libraries it imports):

- `libehsys_rel`
- `libpassword_rel`
- `ulQ`

splat coverage: Split 46 KB (100.00%) in defined segments; unknown:      0 B (0.00%) from unknown bin files

## Sections

| section | file offset | vram | size |
|---|---|---|---|
| `.text` | 0x000054 | 0x000000 | 0x37B8 |
| `.sceStub.text` | 0x00380C | 0x0037B8 | 0x3608 |
| `.lib.ent.top` | 0x006E14 | 0x006DC0 | 0x4 |
| `.lib.ent` | 0x006E18 | 0x006DC4 | 0x20 |
| `.lib.ent.btm` | 0x006E38 | 0x006DE4 | 0x4 |
| `.lib.stub.top` | 0x006E3C | 0x006DE8 | 0x4 |
| `.lib.stub` | 0x006E40 | 0x006DEC | 0x18 |
| `.lib.stub.btm` | 0x006E58 | 0x006E04 | 0x4 |
| `.rodata.sceModuleInfo` | 0x006E5C | 0x006E08 | 0x34 |
| `.rodata.sceResident` | 0x006E90 | 0x006E3C | 0x48 |
| `.rodata.sceNid` | 0x006ED8 | 0x006E84 | 0x1B04 |
| `.rodata.sceVstub` | 0x0089DC | 0x008988 | 0x40 |
| `.data` | 0x008A1C | 0x0089C8 | 0x278 |

## Code

- **41 functions** in 0x37B8 bytes of `.text`
- largest: `func_000002D0` (464 words), `func_00001D54` (434 words), `func_000027A8` (368 words), `func_0000241C` (227 words), `func_00002D68` (219 words)
- 9 functions are <= 16 words — the cheap first targets:
  `func_00000A10`(5), `func_00000A24`(4), `func_00000A34`(2), `func_00000A3C`(7), `func_00000A58`(5), `func_00000D6C`(8), `func_0000152C`(4), `func_0000159C`(2), `func_00001D2C`(10)
- median function size: 44 words
- distinct import stubs called from `.text`: 100
- most-called imports: `func_00003C78`(13x), `func_00004760`(12x), `func_00004C80`(10x), `func_00004AA0`(9x), `func_00004718`(8x), `func_00003D28`(7x), `func_00005EB0`(7x), `func_00006AA0`(7x)

Jump tables referenced by the code (they live in `.data`): `jtbl_000089D0`, `jtbl_000089F8`, `jtbl_00008A1C`.

Globals referenced by the code: 18 distinct symbols, 12 of them past the end of `.data` (vram 0x8C40), i.e. `.bss` state.

## Strings in `.data`

(vram address, first 60 found — the fastest read on what the module does)

```
0x008A40  bg.gim
0x008A48  monitor_shadow.gim
0x008A5C  icon01.gim
0x008A68  icon03.gim
0x008A74  frame.gim
0x008A80  spelltrap_%c.gim
0x008A94  key_%c.ehf
0x008AA0  nixi.ehf
0x008AAC  nixi.tmf
0x008AB8  score-atkdef10.ehf
0x008ACC  strTbl_%c.bin
0x008BEC  labo/password.ehp
0x008C00  jegfis
```

## Next steps

1. `splat split config/rel_password.yaml` (already run; `asm/rel_password/` is gitignored)
2. m2c or hand-decompile the <= 16-word functions listed above into `src/rel_password.c`
3. verify with `scripts/mwcc_build.sh` + `scripts/mwcc_diff.py` (MWCC 1.3 SP7, `-O4,s -sdatathreshold 0`)
4. read `src/rel_movie_viewer.c` and `src/rel_html_view.c` headers first — every
   MWCC codegen lever found so far is documented there.

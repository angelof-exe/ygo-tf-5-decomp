# rel_debug_menu.prx — module survey

**Status: fully decompiled.** `src/rel_debug_menu.c` matches all 44 functions
byte for byte, and `make MODULE=rel_debug_menu SRC=1` relinks the module to its
shipped sha1 (`a989a168…`), jump tables included. See "Decompilation notes"
at the end. The survey below was generated mechanically by
`scripts/module_survey.py` and is kept as-is.

## Identity

| | |
|---|---|
| file size | 53204 bytes (0xCFD4) |
| sha1 | `a989a16846726d49f5fdd5f4f1e11f96171dfe4e` |
| .text | 0x4018 bytes |
| import stubs | 1729 (.sceStub.text 0x3608) |
| .data | 0xABC bytes |
| NID table | 0x1B04 bytes (1729 entries) |

Library names in `.rodata.sceResident` (this module's export name
first, then the libraries it imports):

- `libehsys_rel`
- `libdebug_menu_rel`

splat coverage: Split 53 KB (100.00%) in defined segments; unknown:      0 B (0.00%) from unknown bin files

## Sections

| section | file offset | vram | size |
|---|---|---|---|
| `.text` | 0x000054 | 0x000000 | 0x4018 |
| `.sceStub.text` | 0x00406C | 0x004018 | 0x3608 |
| `.lib.ent.top` | 0x007674 | 0x007620 | 0x4 |
| `.lib.ent` | 0x007678 | 0x007624 | 0x20 |
| `.lib.ent.btm` | 0x007698 | 0x007644 | 0x4 |
| `.lib.stub.top` | 0x00769C | 0x007648 | 0x4 |
| `.lib.stub` | 0x0076A0 | 0x00764C | 0x18 |
| `.lib.stub.btm` | 0x0076B8 | 0x007664 | 0x4 |
| `.rodata.sceModuleInfo` | 0x0076BC | 0x007668 | 0x34 |
| `.rodata.sceResident` | 0x0076F0 | 0x00769C | 0x34 |
| `.rodata.sceNid` | 0x007724 | 0x0076D0 | 0x1B04 |
| `.data` | 0x009228 | 0x0091D4 | 0xABC |

## Code

- **44 functions** in 0x4018 bytes of `.text`
- largest: `func_000002DC` (578 words), `func_00002D98` (522 words), `func_0000160C` (350 words), `func_000010FC` (300 words), `func_000020CC` (261 words)
- 12 functions are <= 16 words — the cheap first targets:
  `func_00000000`(13), `func_00000264`(2), `func_00000BE4`(8), `func_00000C04`(13), `func_00000C38`(16), `func_00000C78`(2), `func_00001B84`(14), `func_00002044`(10), `func_0000289C`(8), `func_000028BC`(12), `func_00003B64`(8), `func_00003B84`(16)
- median function size: 34 words
- distinct import stubs called from `.text`: 115
- most-called imports: `func_000065F8`(46x), `func_00005520`(18x), `func_000054E0`(10x), `func_000044D8`(10x), `func_000054F0`(10x), `func_00004EC0`(9x), `func_00006738`(9x), `func_000068A8`(9x)

`func_00000000` is the module entry point; it registers `func_00000034`, `func_00000264`.

Jump tables referenced by the code (they live in `.data`): `jtbl_000095B4`, `jtbl_000095DC`, `jtbl_000095FC`, `jtbl_0000961C`, `jtbl_00009644`, `jtbl_00009674`.

Globals referenced by the code: 92 distinct symbols, 31 of them past the end of `.data` (vram 0x9C90), i.e. `.bss` state.

## Strings in `.data`

(vram address, first 60 found — the fastest read on what the module does)

```
0x009210  JAPAN
0x009218  U.S.A.
0x009220  Europe
0x009228  Europe
0x009230  Europe
0x009238  Europe
0x009240  Europe
0x009248  Europe
0x009250  language
0x009270  clear all data
0x009290  get all card
0x0092B0  save data
0x0092D0  load data
0x0092F0  demonstration
0x009310  delivery test
0x009330  Sale Zone 
0x009350  BGM: 
0x009370  SE PACK: 
0x009390  SE: 
0x0093B0  NPC: 
0x0093D0  NPC-OFS: 
0x0093F0  DUEL VOICE: 
0x009410  TITLE VOICE: 
0x009430  VOLUME BGM: 
0x009450  VOLUME SE: 
0x009470  VOLUME VOICE: 
0x009490  SEPAC_STATIC
0x0094B0  SEPAC_DUEL
0x0094D0  SEPAC_VS
0x0094F0  SEPAC_STORY
0x0096A8  story test
0x0096D4  effect test
0x0096EC  deck construction
0x009718  deck const(Partner)
0x009740  map test
0x009768  duel test
0x009780  shop test
0x00979C  database test
0x0097C0  movie test
0x0097E0  model test
0x009804  draw test
0x00982C  title test
0x009844  html viewer
0x009864  matching test
0x009888  umd replace test
0x0098A8  gallery
0x009A04  debug_menu.c
0x009A14  YUGI OH 5Ds -TAGFORCE-  TEST MODE
0x009A48  DEBUG OPTION
0x009A58  %s / %s
0x009A68  SOUND TEST
0x009A74  %s%d
0x009A7C  %s EMPTY
0x009A88  %s%d(%d)
0x009A94  %s%s
0x009AA0  %s%d(%d)
0x009ABC  %04d
0x009AC4  ?? Unknown ??
0x009AD4  ## Entry List ##
0x009AE8  empty
```

## Next steps

1. `splat split config/rel_debug_menu.yaml` (already run; `asm/rel_debug_menu/` is gitignored)
2. m2c or hand-decompile the <= 16-word functions listed above into `src/rel_debug_menu.c`
3. verify with `scripts/mwcc_build.sh` + `scripts/mwcc_diff.py` (MWCC 1.3 SP7, `-O4,s -sdatathreshold 0`)
4. read `src/rel_movie_viewer.c` and `src/rel_html_view.c` headers first — every
   MWCC codegen lever found so far is documented there.

## Decompilation notes

What the module is: the developer test menu ("YUGI OH 5Ds -TAGFORCE-  TEST
MODE"). Screens, by state of `D_00009C90`:

| state | screen | update / draw |
|---|---|---|
| 0, 1, 2 | test menu: fade in, list input, fade out + launch | func_00000CE4, func_00000D6C, func_00001058 / func_000002DC |
| 3 | DEBUG OPTION (language, clear all data, get all card, save data, load data, demonstration, delivery test, Sale Zone) | func_000010FC / func_000002DC |
| 4 | waiting for a savedata operation | func_000015AC |
| 7 | SOUND TEST (BGM, SE PACK, SE, NPC, NPC-OFS, DUEL VOICE, TITLE VOICE, 3 volumes) | func_0000160C / func_000002DC |
| 0x5A..0x5C | boot-time savedata load | func_00001B84, func_00001BBC, func_00001C18 |
| 0x63 | ad-hoc "matching test" (groups, duel rules, deck exchange, duel start) | func_0000362C / func_00003F40 |
| 0xC7 | card "delivery test" | func_000020CC / func_000024E0 |

The built-in menu table at 0x98B0 has 16 rows, each `{Japanese name, English
name, handler, arg0, arg1}`. Every handler is func_00001D84 =
`ehsys_551A8EF2(1, arg0, arg1)`, and arg0 runs 2..15 in step with the gmodule
list (2 story, 4 deck, 9 movie test, 13 html viewer, 15 gallery …). The
Japanese labels are Shift-JIS; their translations are in the header of
`src/rel_debug_menu.c`. Other Japanese text: five status messages of the
matching test at 0x9BEC..0x9C68 ("… is applying to join.", "Deck select",
"Could not join.", "Could not confirm the duel members.", "Disbanding the
group."), and the glyphs ◆ ▲ ▼ (`81 9F`, `81 A3`, `81 A5`) used as the cursor
and scroll arrows.

The PRX was linked from at least three C files: the same static text helper
appears three times (func_00000BE4, func_0000289C, func_00003B64). The
`__FILE__` passed into func_00000C04 is "debug_menu.c".

MWCC levers that settled the last diffs (details in the C file header):
- switch tests are emitted in reverse source order, bodies in source order;
- a basic block with more than one predecessor is never stolen into a
  branch-likely delay slot (unfilled `beqz`+`nop` = "this block is a join");
- saved registers and stack slots follow local declaration order;
- a few re-reads of plain globals need volatile casts at exactly those
  accesses (func_00000D6C, func_0000160C, func_000020CC);
- `(x >> 3) & 1` gives the Allegrex `ext`.

Engine-import names proposed from this module (usage evidence only, not
hash-verified; see the import comments in the C file):
`ehsys_F5E3080C` returns the pad state (sceCtrl masks at +0x18 held, +0x1C
pressed, +0x24 repeat; 237 call sites), `ehsys_C87BAB32(x, y, fmt, ...)` is a
printf-style debug text draw, `ehsys_551A8EF2(kind, scene, arg)` requests a
scene change, `ehsys_1F710553` sets the language (pair of
`ehsys_get_language`), `ehsys_94A4EC34`/`ehsys_FFA11C10` get/set a volume
channel, `ehsys_6FFA2A28`/`ehsys_778817A2` start/poll a savedata operation,
`ehsys_41AABF28`/`ehsys_20E340D9` heap alloc/free.

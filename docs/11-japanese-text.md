# Japanese (Shift-JIS) text in the binaries

Konami left Japanese text in the modules: debug menus, test-screen help, a
stringified C enum with its comments, placeholder dialogue. It is encoded in
**Shift-JIS / cp932**, not UTF-8, so our tools handle it badly: gas and
splat's `.asciz` output, editors that re-save files as UTF-8, and (as tested
below) MWCC itself. This page covers what is in the binaries, how to find it,
and the rules for writing it in our sources.

Tool: `scripts/sjis_strings.py` (stdlib only).

```
scripts/sjis_strings.py rel_debug_menu          # table: vram, section, len, text, C literal
scripts/sjis_strings.py rel_debug_menu --c      # /* 0xADDR: <utf8> */ "\x83..." -- paste-ready
scripts/sjis_strings.py --all --tsv             # every module (+ build/EBOOT.elf if decrypted)
scripts/sjis_strings.py --all --summary         # counts per module
scripts/sjis_strings.py --all --symbols         # lib/module names + .comment, flags non-ASCII
scripts/sjis_strings.py --check-src             # lint src/ + config/symbols/, exit 1 on a hit
```

Detection is strict on purpose, because binary data often decodes as valid cp932.
A string is kept only if it meets all of these:

- It is NUL-terminated and decodes cleanly as cp932, with no control or
  private-use characters.
- It has at least 2 kana, fullwidth or **JIS level-1** kanji characters. Rare
  level-2 kanji are what random bytes decode to: `囮卅` is the tail of the
  double `-1.2`.
- Halfwidth katakana is not the main content.
- If the string does not start on a 4-byte boundary, it needs at least 4 such
  characters. MWCC aligns every string literal to 4.

On this game the result is 379 strings with no false positives. Every hit is
4-aligned, and every rejected candidate was binary.

## Survey (all 28 modules, 2026-10-14)

All hits are in `.data`. The modules have no `.rodata`, because MWCC puts
string literals in `.data`.

| module | SJIS strings | what they are |
|---|---:|---|
| `rel_labo` | 307 | 305 = the **voice-ID enum as source text** (`"ID_VCM_DUEL_ST,          //開幕の口上"`, see below); 2 = voice-test HUD |
| `rel_duel_draw` | 38 | in-duel **debug menu**: card spawn/copy, life edit, flags, coin toss; help lines |
| `rel_debug_menu` | 21 | developer menu entries (16) + ad-hoc multiplayer status messages (5) |
| `rel_field` | 9 | **font/test placeholders**: kana table, fullwidth A-Z, dummy NPC lines with `$R` (ruby) / `$C<n>` (colour) codes |
| `rel_movie_viewer` | 2 | template leftovers, "sample" / "reading directory..." |
| `rel_cutin_viewer` | 2 | the same two strings. Both modules are named `modtemplate` / `libtemplate_rel`: one shared template |
| the other 22 | 0 | |

`build/EBOOT.elf` was not decrypted when this survey was run. Rerun with
`--all` once it exists, because the engine is the most likely place for
asserts.

Categories found:

- **Debug-menu UI**, in `rel_debug_menu` and `rel_duel_draw`.
- **Stringified enum**, in `rel_labo`.
- **Placeholder and test text**, in `rel_field`, `rel_movie_viewer` and
  `rel_cutin_viewer`.

Categories **not** found:

- No assert or `__FILE__` strings.
- No SJIS file names. All asset paths are ASCII.
- No player-facing game text. Shipped text lives in the message files and
  `.ehp` archives, not in code.

### The `rel_labo` enum: symbol-like data with Japanese in it

The voice test in `rel_labo` shows the voice-line IDs by name. Konami did this by
including the enum's **source lines, comments and padding included**, as
strings. There are 318 `ID_VCM_*` strings; 305 of them have a Japanese
`//` comment, e.g. `"ID_VCM_SUMSY_A_1,               //シンクロ召喚1"`
("synchro summon 1"). The original enum identifiers are therefore known
(`ID_VCM_TC_A_1_1`, `ID_VCM_DMG_A_2`, ...). Reuse them as our names, and put
the comments, translated, into ours. The literals have to be reproduced
**byte for byte**, including the run of spaces and the SJIS comment. Take them
from `--c` and do not retype them.

### Representative examples

| where | text | English |
|---|---|---|
| debug_menu `0x9694` | ストーリーテスト | story test |
| debug_menu `0x974C` | デュエルマネーヂャテスト | duel manager test (sic: ヂャ) |
| debug_menu `0x9774` | 購買テスト | shop test |
| debug_menu `0x9874` | UMD差し替えテスト | UMD swap test |
| debug_menu `0x9C28` | 参加できませんでした。 | could not join. |
| duel_draw `0x7E328` | カードを生成 | create card |
| duel_draw `0x7E304` | カード配置（dueldebug1.txtが必要） | card placement (needs dueldebug1.txt) |
| duel_draw `0x7E3D0` | 墓地送り(捨てる) | send to graveyard (discard) |
| duel_draw `0x7E414` | チューナー化 | turn into a Tuner |
| duel_draw `0x7E54C` | 表攻 / 表守 / 裏守 | face-up ATK / face-up DEF / face-down DEF |
| labo `0x6BF0` | `ID_VCM_DUEL_ST, //開幕の口上` | opening speech |
| labo `0xADB4` | ○：再生　×：戻る | O: play, X: back |
| field `0x40CA8` | クロノス・デ・メディチ | Chronos de Medici (Crowler's JP name) |
| movie_viewer `0x5A88` | ディレクトリ読み込み中... | reading directory... |

`dueldebug1.txt` is a debug input file that the duel debug menu reads.

### Symbol-like data: all ASCII

Checked with `--symbols` on all 28 modules (167 fields): every `.lib.ent` and
`.lib.stub` library name, every `.rodata.sceModuleInfo` module name, and
`.comment` (`MW MIPS C Compiler (2.4.1.01) PSP`). **None contains a non-ASCII
byte.** A few things are odd, though none of them are Japanese:

- `rel_movie_viewer` and `rel_cutin_viewer` both call themselves
  `modtemplate` / `libtemplate_rel`.
- `rel_html_view` is `module_htmlview`, not `modhtml_view`.

There are no symbol tables (the PRXs are stripped), so no function or global
names can contain Japanese. The only Japanese next to identifiers is in the
`rel_labo` enum strings.

## How MWCC treats SJIS in source (tested)

Tested with `scripts/mwcc_build.sh` (mwccpsp 3.0.1_219, default flags) and
`objdump -s` on the result:

| source | result |
|---|---|
| `"\x83\x66\x83\x85..."` (escaped) | correct bytes, every time |
| raw cp932 `"デュエル開始"` (no 0x5C trail byte) | correct bytes, but only by luck |
| raw cp932 `"表示ソフト能"` | **compile error** `illegal string constant`: 能 = `94 5C`, the 0x5C escapes the closing quote |
| raw cp932 `"表n"` | **compiles to the WRONG bytes** `95 0A`: `5C 6E` was read as `\n`. A silent mismatch |
| raw UTF-8 `"デュエル開始"` | UTF-8 bytes in the object. Wrong, the game expects cp932 |
| same with `-multibyte` | still `illegal string constant` under wibo |
| UTF-8 in `//` and `/* */` comments, incl. bytes 0x81-0x9F before `\n` / `*/` | harmless, code after the comment compiles |

MWCC is **not** Shift-JIS-aware here. It handles source as single-byte text,
so any SJIS character whose trail byte is 0x5C (表 ソ 能 十 ...) breaks the
literal it is in. UTF-8 never contains 0x5C in a multibyte sequence, so UTF-8
comments are safe. A raw cp932 *comment* is not: if it ends in 0x5C, the
backslash continues the `//` comment and **the next line of code disappears**.

## Rules for contributors

1. **Identifiers are ASCII only.** Romanize or translate: `ID_VCM_DUEL_ST`
   stays as is, and a menu table becomes `s_debug_menu_story_test`, not a
   Japanese name.
2. **SJIS literals are written as `\x` escapes**, exactly as
   `scripts/sjis_strings.py <module> --c` prints them. ASCII stays readable.
   The tool splits `"\x83" "f"` wherever a hex digit follows an escape,
   because C's `\x` is greedy.
3. **Put the English translation in a comment** next to the literal. The
   original Japanese may go there too, in UTF-8:
   ```c
   /* 0x000096E0: デッキ編集 = "deck edit" */
   char s_debug_deck_edit[] = "\x83" "f\x83" "b\x83L\x95\xd2\x8fW";
   ```
4. Source files are **UTF-8 (or ASCII)**. Never save a file as cp932.
5. **Run `scripts/sjis_strings.py --check-src` before committing.** It
   reports `file:line` for:
   - any non-ASCII byte in code, a string literal or a char literal;
   - a comment that is not valid UTF-8;
   - a `//` comment that ends in 0x5C;
   - any non-ASCII byte before the `//` in `config/symbols/*.txt`.

   It exits 1 if it finds anything. Clean as of this writing: 7 source files
   and 31 symbol files, 0 problems.
6. Keep it short in docs and comments. Quote a few words to identify a string,
   but do not paste the game's full text into the repo: it is Konami's
   copyrighted content.

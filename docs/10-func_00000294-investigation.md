# `func_00000294` — why it is not matched, and what was ruled out

Status as of 2026-08-17: **`rel_movie_viewer` is 15/16 matched.** This document
records the investigation into the one remaining function so the dead ends are
not re-walked. A condensed version lives in the comment block above
`func_00000294` in `src/rel_movie_viewer.c`.

## What the function does

`func_00000294` initialises a configuration blob at `D_000A3F0C` — font/frame
geometry, a set of flags, and a render callback pointer (`func_000007C8`). It is
mostly straight-line: a `memset`-style clear via `func_00001D7C`, ~40 field
stores of constants, a call to `func_00001694`, then a small geometry clamp
using the Allegrex `min`/`max` intrinsics.

There are **no loops** and essentially no branching. This matters — see
"The differentiator" below.

## The discrepancy

Our version is semantically complete and correct. It emits every store the
target does, verified by kind: **13 `sb`, 11 `sh`, 14 data `sw`** — matching the
target exactly.

It is **13 instructions too long**: target 116 words, ours 129.

The entire difference is how the blob's address is formed:

| | Address handling | `lui` count |
|---|---|---|
| **Target** | `&D_000A3F0C` loaded once into `$s0`; record base (`$s0 + 0x4C`) into `$s1`; both held for the whole function. Each access is one instruction with an offset (`sw $v0, 0x14($s1)`). | 5 |
| **Ours** | Address re-derived at each use: `lui` plus a store with `%lo` folded into the offset. | 31 |

Same stores, same values, same order — different addressing.

## Root cause

At `-O2` and above MWCC enables what its own `-help` text calls *"copy and
expression propagation"*. Because `&D_000A3F0C` is a compile-time constant as
far as the optimiser is concerned, that pass propagates it into all 31 use sites
and rematerialises it at each one instead of keeping it live in a callee-saved
register.

This is not a compiler bug. Rematerialising a cheap constant instead of tying up
a saved register (which costs a save/restore pair) is a legitimate choice.

## Ruled out, with evidence

| Hypothesis | Result |
|---|---|
| Wrong optimisation flag | `-O4,s` / `-O4,p` / `-O4` / `-O3,s` are **identical**: 128 words, `lui=31`. This function does not discriminate on the flag. |
| Wrong compiler build | All **11** installed builds (`mwccpsp_3.0.1_121` … `_219`) at `-O4,s`: identical 128 / `lui=31`. |
| `register` keyword on the pointers | No effect. |
| Forced inlining | `-inline on/all/auto/level=8/deferred`: no effect. Inlining re-exposes the constant and it folds again. |
| A switch to disable propagation | Does not exist — `-opt` exposes only `level=` and `[no]intrinsics`. `-O1,s` *does* give the cached-base shape (`lui=5`) but leaves the rest unoptimised at 143 words, and would regress the 15 matched functions. |
| Modelling the blob as a real struct | 128 / `lui=31`. |
| Declaring it `volatile` (struct and pointer) | 128 / `lui=31`. |
| `decomp-permuter`, 337,244 iterations / 30 min | Best diff score 3875 (from 6830 base), nowhere near 0. Best candidate reached 123 words **by dropping a byte store and widening another** (`sb`=12/`sh`=12 vs 13/11) — semantically wrong, rejected. |

### A previously recorded theory that did not hold up

Earlier comments in `src/rel_movie_viewer.c` described an `-O4,p`-specific
mwccpsp **store-elimination bug** that silently dropped four byte stores once
two pointers shared a base, and claimed a struct form reached 107 words with
`lui=5`. Neither reproduces with the layout used now: under **both** flags all
13 `sb` and 11 `sh` are emitted, and no source shape tried reached `lui=5` at
`-O2`+. If the original observation was real it depended on some detail of that
session's struct layout, not on the flag. The comment block has been corrected.

## Proof the compiler can produce the target's shape

Written as a helper taking the blob as a parameter — `void init(Cfg *cfg)` — the
identical body compiles to **103 words, `lui=4`**, all stores correct, with a
prologue matching the target exactly including `addiu <rec>, <cfg>, 0x4C`
scheduled into the `func_00001D7C` delay slot. A pointer *parameter* cannot be
rematerialised, so the base necessarily stays in a saved register.

Note the symmetry: direct = 129 (+13), parameter = 103 (-13), target = 116.

This is **diagnostic only** — the target takes no arguments, so the parameter
form is not a legal candidate.

## Cross-module scan (2026-08-17)

Disassembling other modules directly (`mips-linux-gnu-objdump -d --section=.text
<module>.prx`) and searching for an address materialised into a callee-saved
register (`lui $sN,…` followed by `addiu $sN,$sN,…`):

| Module | `.text` instructions | Cached-base sites |
|---|---|---|
| `rel_movie_viewer` | 557 | 2 |
| `rel_html_view` | 472 | 0 |
| `rel_soundtest` | 1402 | 0 |
| `rel_tutoriallist` | 2655 | 0 |
| `rel_password` | 3566 | 2 |

The pattern is **rare**. That is reassuring: it means the project's compiler
configuration is not systematically wrong. (Caveat: 5 of 28 modules sampled —
"rare" is well supported, the per-module rate is not.)

### The differentiator

`rel_movie_viewer`'s two sites are:

- `0x44` → `0xA0000-0x2500` = `D_0009DB00`, inside **`func_00000034`**, which
  matches at 100%
- `0x2a4` → `0xA0000+0x3F0C` = `D_000A3F0C`, inside `func_00000294`

So this codegen **is** reproducible and we already do it once. The structural
difference: in `func_00000034` the base is used inside a `while` loop, where the
address is loop-invariant and gets hoisted into a saved register that must
survive the calls in the loop body. `func_00000294` is straight-line, so there
is nothing to hoist.

(Inference, not isolated experimentally — it is the only other instance in the
module and it sits in a loop.)

## The open question

**What construct makes MWCC hold a global's address in a callee-saved register
across straight-line code at `-O4`?**

Every shape tried leaves the address a visible compile-time constant, after
which the optimiser correctly prefers rematerialisation.

One unexplained detail that may be a clue: the target mixes two addressing modes
inside this single function. `&D_000A3F0C` appears as a relocated symbol
reference (`%hi`/`%lo`), while `D_0009DB00`, `D_000A3F08` and `D_000A4030` are
raw absolute (`lui $v0, 0xA` / `lw $v0, -0x2500($v0)`) with no relocation. That
split suggests these were declared or reached differently in the original
source.

## Why the unmatched version is the one committed

A 13-word-smaller version exists but drops a field write. A decompilation that
produces a subtly wrong object is worse than one honestly marked as not-yet
matching: the first corrupts state at runtime, the second is merely unfinished.

The committed version is the **correct, oversized** one, tagged NONMATCHING.

## Reproducing / continuing

```sh
# baseline
scripts/mwcc_build.sh src/rel_movie_viewer.c
scripts/mwcc_diff.py asm/rel_movie_viewer/text.s build/mwcc/rel_movie_viewer.o func_00000294
# -> SIZE MISMATCH target=116 words candidate=129 words

# flag / build sweeps: pass flags as extra args to mwcc_build.sh, or invoke
# tools/wibo-bin/wibo tools/mwccpsp_3.0.1_<N>/mwccpsp.exe directly

# the full Allegrex intrinsic list
strings tools/mwccpsp_3.0.1_219/mwccpsp.exe | grep __builtin_allegrex

# cross-module codegen scan
mips-linux-gnu-objdump -d --section=.text \
  iso_extracted/PSP_GAME/USRDIR/gmodule/<module>.prx
```

### decomp-permuter scaffold

A working scaffold was built for this function and is reusable for any other.
It needs four files in one directory: `base.c` (single function), `target.o`,
`compile.sh`, `settings.toml` (`func_name`, `compiler_type = "mwcc"`).

Gotchas worth remembering:

- **GNU `as` cannot assemble the Allegrex `min`/`max`** opcodes, so when
  building `target.o` from splat's asm those two instructions must be emitted as
  `.word 0x…` (little-endian from the hex in splat's comment column).
- **Do not declare `__builtin_allegrex_min`/`_max`** in the source — MWCC
  already declares them as `long(long, long)` and a redeclaration is a hard
  error. (`pycparser` tolerates the implicit declaration.)
- The permuter needs `toml` and `Levenshtein` installed in `.venv`.

Do **not** re-run the permuter on this function without first changing something
that denies the compiler the constant address — otherwise it is provably
searching the wrong space.

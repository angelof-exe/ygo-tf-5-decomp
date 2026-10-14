# MWCC codegen cookbook

What makes mwccpsp 3.0.1_219 (`-O4,s -sdatathreshold 0`) emit a given shape.
Every entry here fixed a real function; the module doc or the function's
comment in `src/` has the full story. Read this before fighting a diff.

## Branches and delay slots

- **Only one branch gets the fill.** When several branches jump to a shared
  block (`li v0,1` + epilogue), MWCC moves that block's first instruction into
  the delay slot (`bnezl`/`beql` + `li`) of only the **first** branch in
  layout order; later ones get `nop`. An explicit `return 1;` emits its own
  `b epilogue; li v0,1` and does not use up the fill. To move the fill, turn an
  explicit return into a fall-through to the shared return, or the other way.
  (rel_soundtest func_000004AC)
- **A block reachable from more than one place never goes into a
  branch-likely slot.** An unfilled `beqz` + `nop` in the target means that
  block has several predecessors. Add a `return` at the end of a branch, or
  split an `else if` into a separate `if`. (rel_debug_menu func_00002A88)
- **Mirror the comparison.** `if (k > cur)` gives a fused `slt $at` + branch;
  `if (cur < k)` gives a compare into a register + `bnez`, with then/else
  swapped. `x > K` puts a store in the delay slot (`slti $at`); `x >= K` puts it
  before the compare. When a target branch looks inverted, try the mirrored
  operator. (rel_tutoriallist, rel_soundtest)
- **`while ((f(x) != 0) ^ 1)`** is the only spelling that gives
  `sltu; xori 1; bnez`. `== 0`, `!` and `!(x != 0)` give a plain `beqz`.
  (rel_soundtest)
- **Clamps:** one `if (a < 0 || a >= 6)` matches where the `else if` version
  doesn't. `if (a == 0 || b == 0) return;` lays out differently from two
  separate `if`s.
- **Early `return 0` vs falling through** to the shared return decides
  whether a later branch gets a branch-likely fill. (rel_labo)

## switch

- MWCC emits the **tests in reverse source order** and the **bodies in source
  order**. Reorder the cases to reorder the tests. (rel_debug_menu)
- `break` and `return 0` in a case give different branch pairs.
  (rel_html_view func_000005A0)
- A single-case `switch` is not the same as an `if`: it branches INTO the case
  (`beql`), with a separate `b` for the fall-through. (rel_movie_viewer)
- The dead delay-slot copies seen thousands of times in big modules come from
  control flow, not from flags: an early-return `switch` keeps them, an
  if/else-if chain loses them. (rel_tutoriallist)

## Registers and stack

- **Declaration order** of locals sets their stack slots and which of
  `$s0`/`$s1`/... they get. Try permutations first.
- **Local aggregate to swap saved registers.** If two locals have
  `$s2`/`$s3` swapped, put them in `struct { int a; int b; } t;` (or
  `int v[2]`). (rel_soundtest func_00000908)
- **Reusing a variable** (`v = y - 0xA; lim = v; v = G;`) changes which
  register the second load gets. (rel_movie_viewer func_00000294)
- `(x >> 3) & 1` compiles to Allegrex `ext`.

## Globals and addresses

- **Address kept in `$s0` vs recomputed per field.** MWCC rematerialises
  `&g` at each field store when `p = &g` is in the same basic block as the
  stores. Split the block with a check it removes later:
  `int size = sizeof(*p); if (size != 0) memset(p, 0, size);`.
  (rel_html_view func_00000470, rel_movie_viewer func_00000294)
- **Declare globals at their real size and type.** `extern char blob;`
  tells MWCC the blob is 1 byte, and it then drops constant byte stores past
  it. Use the real struct. (rel_movie_viewer func_00000294)
- **One struct vs separate globals is an alias-analysis change**, not just
  spelling: some functions only match one way. If `func_00000008`-style init
  memsets N bytes, the state is one N-byte struct. (rel_labo, rel_soundtest)
- Globals are **not** volatile everywhere: some modules need `volatile`
  casts on exactly the accesses the target re-reads. (rel_debug_menu)

## Data the C must not emit

`make SRC=1` links `.data` verbatim from the shipped module, so the object
must emit no `.data`/`.bss`:
- strings and tables are `extern D_XXXXXXXX` declarations;
- `char s[6] = "jegfis"` becomes a struct copy from the extern
  (`*(LangCodes *)D_...`), which compiles to the same copy loop;
- the float constants and jump tables can come out in a different order from
  the original (one source file vs several). `static const` padding or arrays
  placed at the right points reproduce it. (rel_password, rel_umd_replace)

## When hand-editing stalls: cut-out permuting

decomp-permuter on a 300-word function rarely converges. Instead:
1. carve the region that differs into a small standalone function with the
   same live ranges;
2. build a target for just that piece: hand-assemble it, or take our own `.o`
   and patch the differing register fields;
3. run the permuter on the small piece (score 0 in ~2,000–5,000 iterations);
4. move the shape it found back into the real function.

This fixed the last words of rel_movie_viewer func_00000294 and
rel_soundtest func_00000908. `source .venv/bin/activate` has the permuter's
`toml` and `Levenshtein` dependencies.

## Tool gotchas

- wibo lower-cases output filenames: `-o v_A.o` writes `v_a.o`. Use
  lower-case scratch names.
- After a `make SRC=1`, delete `build/<m>.elf build/<m>.prx` before a plain
  `make` (both write the same file).
- An `@NNN` jump table in our object vs `jtbl_XXXXXXXX` in the target, and a
  `D_X + 4` field vs splat's `D_X+4` name, are the same thing:
  `scripts/mwcc_diff.py` resolves both. `make SRC=1` is the final word.

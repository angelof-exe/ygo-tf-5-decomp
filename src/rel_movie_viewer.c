/*
 * rel_movie_viewer.prx — reconstructed code (matching decompilation)
 *
 * Compiler:     Metrowerks CodeWarrior for PSP — MWCC 1.3 SP7 (mwccpsp_3.0.1_219)
 * Flags:        -O4,s -sdatathreshold 0   (SIZE, not ,p — see docs/09)
 * Verification: local build with wibo + mwccpsp_3.0.1_219 (see docs/03-tools.md
 *               "local matching" section) — compare with
 *               `mips-linux-gnu-objdump -dr` against asm/rel_movie_viewer/text.s,
 *               treating any word with a matching relocation (HI16/LO16/26,
 *               same symbol) on both sides as equal regardless of its baked
 *               immediate. Only functions marked "MATCH 100%" below have been
 *               confirmed this way; 16/16 functions in this file currently are.
 *
 * IMPORTS ARE NAMED (docs/nids/README.md). Every call out of this module is
 * now a resolved name rather than a stub address, and those names are shared
 * with all 27 other modules: ehsys_B4471B5E (the module-registration call that
 * opens every module), ehsys_memset, ehsys_strcpy, ehsys_strcat,
 * ehsys_strlen, and sceIoDopen/sceIoDread/sceIoDclose — which confirm that
 * func_000006BC really is a directory walk. Six of these names also appear in
 * src/rel_html_view.c, spelled identically.
 *
 * Conventions (see docs/09-first-match.md):
 *  - global game-state variables are `volatile` and are accessed via a local
 *    pointer to reproduce the original codegen (value reload, use of $s0);
 *  - names `func_XXXXXXXX` / `D_XXXXXXXX` = placeholders (address), to be renamed
 *    as their purpose becomes clear.
 *  - whether a state-blob field is a fresh named extern (`D_0009DB04`, ...) or
 *    `offset(basePointer)` off a `volatile T *p = &D_0009DB00;` depends on
 *    whether the ORIGINAL reuses one register across multiple fields of the
 *    blob (offset form) or recomputes a fresh %hi/%lo per field (separate
 *    extern form) — this must be checked per function against the target asm,
 *    it is not a blanket rule (func_00000138 does NOT share a register between
 *    D_0009DB00 and D_0009DB04; func_00000034/func_000001C8/func_00000470/
 *    func_00000540/func_000005C8/func_000006BC DO share one across several
 *    fields).
 *
 * Functions below `func_00000184` without a "MATCH 100%" tag are DRAFTS: they
 * compile and their instruction COUNT/logic is right (each has been narrowed
 * to the closest word-count achievable and its remaining gap root-caused —
 * see its own comment), but a handful of instructions differ from the
 * target, almost always one of these confirmed-uncontrollable-from-portable-C
 * MWCC codegen choices:
 *   - branch polarity in an if/else-if chain (`beq`-to-handler vs
 *     `bne`-skip), and whether a loop test gets rotated to the bottom
 *     without duplication or WITH a duplicated pre-test — both are
 *     sometimes forceable with an explicit `goto`-based rewrite that
 *     mirrors the target's exact block layout (worked for func_00000540,
 *     func_000007C8's copy loop, and the branch shapes in func_000001C8/
 *     func_000005C8/func_000006BC), sometimes not (func_00000034's loop
 *     test). When `goto` will not do it, try a single-case `switch`: it is
 *     NOT equivalent to `if` here. MWCC compiles a `switch` by branching
 *     INTO the case body (`beql`, with a body instruction scheduled in the
 *     delay slot) and giving the fall-through path its own `b` to the
 *     epilogue, whereas every `if`/`goto` phrasing of the same test folds
 *     the two into a single `bnel`-skip that is one word shorter. This is
 *     what matched func_00000470's tail and func_00000034's dispatch;
 *   - [RESOLVED — was wrong] the Allegrex `min`/`max` instructions were
 *     recorded here as an unreliable compiler fold of a `(x < 0) ? 0 : x`
 *     clamp. They are not a fold at all: MWCC NEVER emits them from portable
 *     C (verified across every `?:`/`if` phrasing, every -O level, and all 11
 *     installed builds). They are the compiler intrinsics
 *     `__builtin_allegrex_max` / `__builtin_allegrex_min`, and the original
 *     source called them directly. Argument order is
 *     `__builtin_allegrex_max(rs, rt)` -> `max rd, rs, rt`, so the target's
 *     `max $v1, $zero, $v1` is `__builtin_allegrex_max(0, x)`.
 *     Beware when probing for these: `mips-linux-gnu-objdump` does not know
 *     the Allegrex opcodes and prints them as a bare `.word`, so a successful
 *     intrinsic call looks like a failure unless you check the encoding
 *     (`max` = funct 0x2C, `min` = 0x2D). The full intrinsic list is
 *     recoverable with `strings tools/mwccpsp_3.0.1_219/mwccpsp.exe |
 *     grep __builtin_allegrex` — bitrev, clz/clo, ctz/cto, ext/ins, rotl/rotr,
 *     seb/seh, wsbh/wsbw, sqrt_s, and the float round/floor/ceil/trunc.
 *     Used by func_00000470; func_00000294 needs both;
 *   - [RESOLVED — was wrong] whether it caches a global's address in a
 *     saved register across many straight-line statements. This was
 *     recorded as uncontrollable; it is not. MWCC only propagates the
 *     constant address inside the basic block that holds `p = &global`, so
 *     splitting that block with a branch the optimiser removes later
 *     (`int size = sizeof(*p); if (size != 0) memset(p, ...)`) keeps it in
 *     $s0. See func_00000294 and docs/10;
 * Treat DRAFTs as very-close NONMATCHING, not as verified.
 *
 * CAUTION on "the compiler just allocates registers differently" diagnoses:
 * func_000007C8 was documented as exactly that (`$a0` vs `$a1` for the
 * copy-loop pointer) and it was NOT — the leading parameter had been typed as
 * unused when the target actually forwards it into ehsys_CABEA834, so $a0 was
 * never free to begin with. Fixing the dataflow matched the function outright.
 * An "unused" leading parameter that the target never writes before a call is
 * a strong hint that the parameter is being passed straight through.
 *
 * Layout notes discovered while drafting (see asm/rel_movie_viewer/text.s):
 *  - D_0009DB00 is the start of a much larger state blob. func_00000138
 *    zero-fills it with size 0x653C via ehsys_memset(&D_0009DB00, 0, 0x653C),
 *    which is exactly (last known field 0x6538) + 4 — i.e. 0x653C is very
 *    likely the blob's real size.
 *  - D_000A3F0C (referenced directly elsewhere via its own %hi/%lo symbol)
 *    is exactly D_0009DB00 + 0x640C: the two names denote adjacent globals
 *    in the SAME data blob, not unrelated objects.
 *  - Fields at D_0009DB00+0x6408 and +0x6530 are read both via a register
 *    holding &D_0009DB00 (spimdisasm resolves these to D_0009DB00 in the
 *    asm) and via raw absolute immediates (0xA3F08 / 0xA4030, left
 *    unresolved by spimdisasm because no relocation entry pointed at the
 *    middle of the blob) — same memory, two addressing idioms.
 *  - D_0009DB00+0x0008 up to +0x6408 looks like an array of 0x100-byte
 *    records (100 of them: 100*0x100+8 = 0x6408), indexed via
 *    `(index << 8) + 8`; +0x6408 itself is used as a slot counter/index
 *    compared against 0x64 (100).
 */

extern volatile int D_0009DB00;
extern void ehsys_1078C73B(int);


/* ============================================================
 * Remaining functions of the module. Each is individually tagged
 * "MATCH 100%" if verified, otherwise treat as a close NONMATCHING draft
 * (see the file header for how these were checked and what's usually left).
 * ============================================================ */

/* externs used by the drafts (types are best-effort guesses from usage,
 * to be corrected once matched or once the modules they live in are
 * decompiled) */
extern void ehsys_B4471B5E(void (*)(void), void (*)(void), void *);
extern void ehsys_C2E59A21(int, void (*)(void), void (*)(void));
extern int ehsys_7B552C34(int);
extern int ehsys_1856E536(void *, int);
extern void *ehsys_F5E3080C(void);
extern int ehsys_DFCA450B(int, int);
extern void ehsys_1EC5342B(int);
extern int ehsys_CABEA834(void *, int);
extern int ehsys_E6E4401F(int, int, int);
extern void ehsys_2012884C(void *);
extern void ehsys_EF9B5D06(void *, void *);
extern int ehsys_39ADDA9F(void *);
extern void ehsys_E1139F1A(void *);
extern void ehsys_13A4081A(int);
extern void ehsys_8060707A(void);
extern void ehsys_08813E19(int, int);
extern void ehsys_B48A11FB(void *, int);
extern void ehsys_BE3756D0(int, int);
extern void ehsys_memset(void *, int, int);
extern void ehsys_strcpy(void *, void *);
extern void ehsys_strcat(void *, void *);
extern int ehsys_strlen(void *);
extern int ehsys_sceIoDopen(void);
extern void ehsys_sceIoDclose(int);
extern int ehsys_sceIoDread(int, void *);
extern void ehsys_70CB33BE(int, int);
extern int ehsys_DCA0DCE1(void);
extern void ehsys_50EA4D71(void *, int);
extern void ehsys_245526EE(void);
extern void ehsys_C87BAB32(int, int, void *);
extern void ehsys_942B03D0(int, int);
extern void ehsys_60B55A50(int);
extern void ehsys_B89D38DC(int, int, int, void *);
extern void ehsys_B8AD96EA(int);
extern void ehsys_A4AFF8E6(void);
extern void ehsys_frame_sync(void);

extern char D_00005A74;
extern char D_00005A7C;
extern char D_00005A88;
extern char D_00005AA4;
extern char D_00005AC0;
extern char D_00005AD0;
extern char D_00085B00;
/* The "viewer config" blob at D_000A3F0C (0x124 bytes). The same layout is
 * the MsgBox of src/rel_umd_replace.c (also cleared with 0x124 and handed
 * to ehsys_2012884C). Field names are byte offsets until their meaning is
 * known.
 *
 * D_000A3F0C MUST be declared with this real type. Declared as a 1-byte
 * `extern char`, MWCC's dead-store pass treats every byte store past the
 * end of that object as aliasing the same location, and silently drops
 * four of the 13 `sb` stores in func_00000294 (that was the "store
 * elimination bug" of earlier notes, see docs/10). */
typedef struct {
    unsigned char b0 : 1;
    unsigned char b1 : 1;
    unsigned char b2 : 1;
    unsigned char b3 : 1;
    unsigned char b4 : 1;
    unsigned char b5 : 1;
    unsigned char b6 : 1;
    unsigned char b7 : 1;
} CfgBitsCC;

typedef struct {
    unsigned char lo : 3;
    unsigned char mid : 4; /* 0x78 */
    unsigned char hi : 1;
} CfgBitsCD;

typedef struct {
    unsigned char lo : 5; /* 0x1F */
    unsigned char hi : 3;
} CfgBitsD6;

/* record at cfg + 0x4C (offsets below are relative to the record) */
typedef struct {
    int f00, f04;
    signed char f08, f09;
    short pad0A;
    short f0C, f0E;
    int f10, f14, f18, f1C, f20, f24, f28, f2C, f30, f34, f38, f3C, f40, f44;
    short f48, f4A, f4C, f4E;
    signed char f50;
    char pad51[3];
    int f54, f58;
    signed char f5C;
    char pad5D[3];
    int f60, f64, f68;
    char pad6C[0x7C - 0x6C];
} CfgRec;

typedef struct {
    char work[0x4C];
    CfgRec rec;      /* 0x4C */
    int fC8;
    CfgBitsCC fCC;
    CfgBitsCD fCD;
    short padCE;
    short fD0;
    signed char fD2;
    char padD3;
    short fD4;
    CfgBitsD6 fD6;
    char padD7[0xF4 - 0xD7];
    short fF4, fF6;
    char padF8[8];
    void *f100;      /* render callback (func_000007C8) */
    char pad104[0x124 - 0x104];
} ViewerCfg;

extern ViewerCfg D_000A3F0C;

/* other fields of the D_0009DB00 state blob (see file header) — declared as
 * their OWN extern symbols, not as `D_0009DB00 + offset` pointer arithmetic:
 * the target reloads a fresh %hi/%lo pair per distinct global instead of
 * reusing one base register across unrelated fields (confirmed by matching
 * func_00000138, which recomputes `lui $v1,0xA0000>>16` twice back-to-back
 * for D_0009DB00 and D_0009DB04 instead of reusing the register). */
extern volatile int D_0009DB04;           /* +0x4: current mode (1..4) */
extern volatile int D_000A3F08;           /* +0x6408: slot cursor/count (<0x64) */
extern volatile int D_000A4030;           /* +0x6530: last-picked slot index */
extern volatile int D_000A4034;           /* +0x6534: fade-pending flag */
extern volatile int D_000A4038;           /* +0x6538: load counter */
extern volatile unsigned char D_000A3FF9; /* +0x64F9: substate byte */

/* forward declarations: call graph is not in address order */
void func_00000034(void);
void func_00000138(void);
void func_00000184(void);
void func_000001C0(void);
void func_000001C8(int mode);
void func_000006B4(void);
void func_0000024C(void);
void func_00000294(void);
void func_00000464(void);
void func_00000470(void);
void func_00000540(void);
void func_000005C8(void);
void func_000006BC(void *arg0);
void func_000007C8(void *unused, int arg1, int arg2, int arg3);
void func_00000894(int arg0, int arg1, void *arg2);

/* func_00000000 — module entry point: registers the viewer's main loop
 * (func_00000034) and teardown (func_000001C0) callbacks.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000000(void) {
    ehsys_B4471B5E(func_00000034, func_000001C0, (void *) &D_00005AC0);
    return 0;
}

/* func_00000034 — viewer main loop: dispatches on the current mode
 * (D_0009DB00+0x4) each frame until told to exit (mode 4).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0).
 *
 * The dispatch is a `switch`, not an if/else-if chain: the target tests the
 * cases DESCENDING (4,3,2,1) while laying the bodies out ASCENDING (case 1,
 * then 2, then 3), which an if-chain cannot produce — MWCC emits switch tests
 * in reverse and bodies in source order.
 *
 * This function is what pinned the project's optimization flag. Its last two
 * words were the loop's entry test (`b .L000000F4` + `sltu $v0,$zero,$s1`),
 * which the target emits even though $s1 was just zeroed and the test is
 * provably true. No source phrasing brought it back: `while`, `for`, a
 * guarded `do/while`, an explicit `goto` rotation and
 * `while (1) { if (done) break; }` all fold identically, as does every
 * storage form for the flag (char/short/array/struct/register) and every one
 * of the 11 compiler builds. It was never a source problem — eliding a loop
 * entry test is a SPEED optimization and the documented flag was wrong.
 * Under `-O4,s` (optimize for size, which is what TF5 was built with) MWCC
 * keeps the test and this function matches as written. See docs/09. */
void func_00000034(void) {
    volatile int *state = &D_0009DB00;
    int done = 0;
    int mode;

    func_00000138();
    ehsys_13A4081A(0);
    ehsys_C2E59A21(0, func_000005C8, func_000006B4);

    while (!done) {
        ehsys_frame_sync();
        mode = state[1]; /* +0x4 */
        switch (mode) {
        case 1:
            func_0000024C();
            func_000001C8(2);
            break;
        case 2:
            func_00000470();
            break;
        case 3:
            func_00000540();
            break;
        case 4:
            done = 1;
            break;
        }
    }

    func_00000184();
    ehsys_8060707A();
    ehsys_frame_sync();
    ehsys_08813E19(9, 0);
}

/* func_00000138 — viewer init: zero the whole state blob, open the movie
 * resource, and set the initial mode.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000138(void) {
    ehsys_memset((void *) &D_0009DB00, 0, 0x653C);
    D_0009DB00 = ehsys_1856E536((void *) &D_00005AD0, 0x80000);
    D_0009DB04 = 1;
}

/* func_00000184 — "release-and-clear" cleanup of a global handle.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000184(void) {
    volatile int *p = &D_0009DB00;
    if (*p != 0) {
        ehsys_1078C73B(*p);
        *p = 0;
    }
}

/* func_000001C0 — empty callback (teardown hook registered by
 * func_00000000; nothing to release here).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000001C0(void) {
}

/* func_000001C8 — the viewer's mode-transition routine, and the only writer
 * of the mode field. Every mode change in this module goes through here:
 * func_00000034's main loop calls it to enter mode 2, and func_00000470
 * calls it to reach modes 3 and 4.
 *
 * Behaviour, in order:
 *   1. read the CURRENT mode from D_0009DB04 and run that mode's EXIT hook —
 *      only mode 2 has one (func_00000464, which tears down the config blob
 *      built by func_00000294);
 *   2. store the NEW mode into the state blob at +0x4 (same field, reached
 *      here through the shared $s1 base rather than the D_0009DB04 symbol —
 *      see the file header on when each addressing style is correct);
 *   3. run the new mode's ENTRY hook — mode 2 builds the viewer config
 *      (func_00000294), mode 3 clears the fade-pending flag at +0x6534.
 * Modes 1 and 4 have neither hook, which is why they are absent from both
 * switches rather than present as empty cases.
 *
 * MATCHING NOTE — BOTH dispatches must be `switch`, including the first,
 * which has only a single case. MWCC compiles a switch by branching TO an
 * out-of-line case body and jumping over it (`beq` to the body, `b` past it,
 * test constants duplicated into both paths), whereas an equivalent `if` —
 * however it is written, goto chains included — gets an inverted branch with
 * the body inline, which is 2 words shorter and cannot be talked out of it.
 * Recognising that shape is what matched this function, and the same lever
 * later matched func_00000034's dispatch and func_00000470's tail.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000001C8(int mode) {
    int old_mode = D_0009DB04;
    volatile int *s1 = &D_0009DB00;

    switch (old_mode) {
    case 2:
        func_00000464();
        break;
    }
    s1[1] = mode; /* +0x4 */

    switch (mode) {
    case 2:
        func_00000294();
        break;
    case 3:
        *(volatile int *) ((volatile char *) s1 + 0x6534) = 0;
        break;
    }
}

/* func_0000024C — one-shot: format a string into a stack buffer and hand
 * it to func_000006BC (a size-measurement/wrap routine, going by its own
 * body); buffer size confirmed by the match (0x110 frame - 0x10 = 0x100).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_0000024C(void) {
    char buf[0x100];

    ehsys_strcpy(buf, (void *) (long) ehsys_7B552C34(3));
    ehsys_strcat(buf, &D_00005A74);
    D_000A3F08 = 0;
    func_000006BC(buf);
}

/* func_00000294 — init the "viewer config" blob at D_000A3F0C (font/frame
 * geometry, flags, the func_000007C8 render callback), then derive its
 * D0/D4/D6 fields from the slot cursor (D_000A3F08) and the last-picked
 * slot (D_000A4030), clamped with the Allegrex min/max intrinsics.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0).
 *
 * This was the module's holdout for a long time (docs/10 has the whole
 * story). Three things were needed, each confirmed in isolation:
 *  1. `int size = sizeof(*cfg); if (size != 0) memset(...)`. MWCC's copy
 *     propagation of the constant &D_000A3F0C only rewrites uses inside the
 *     basic block holding `cfg = &D_000A3F0C`; splitting that block with a
 *     branch the optimiser removes later keeps the address in $s0 (and the
 *     record base in $s1) for the whole function, as the target does,
 *     instead of a fresh lui per store. Lever found in rel_html_view.
 *  2. D_000A3F0C declared as the real 0x124-byte struct (not `extern char`),
 *     otherwise four byte stores are dropped — see the ViewerCfg comment.
 *     +0xCC/+0xCD/+0xD6 are bitfields; `flag` is the variable whose
 *     `andi $a2,$zero,1` the target keeps.
 *  3. The tail: one variable (`v`) first holds `y - 10`, is copied to
 *     `lim`, and is then reused for D_000A4030. Without that reuse MWCC
 *     colours the tail one register higher (a2/a1 instead of a1/v1) and
 *     schedules the D0 store before the `addiu` (4 words differ). Found by
 *     decomp-permuter on a reduced copy of the tail.
 * Also load-bearing, oddly: the redundant `(ViewerCfg *)` cast on
 * &D_000A3F0C. Without it the record base is re-derived as its own
 * lui/addiu pair (`lui $s1` + `addiu $s1,$s1,0x4C+%lo`) instead of
 * `addiu $s1,$s0,0x4C`, one word longer (117). */
void func_00000294(void) {
    ViewerCfg *cfg = (ViewerCfg *) &D_000A3F0C;
    CfgRec *rec = &cfg->rec;
    int size = sizeof(*cfg);
    int flag = 0;
    int y, v, lim;

    if (size != 0) {
        ehsys_memset(cfg, 0, sizeof(*cfg));
    }

    rec->f00 = 1;
    rec->f14 = 1;
    rec->f3C = 1;
    rec->f2C = 1;
    rec->f30 = 1;
    rec->f24 = -1;
    rec->f54 = -1;
    rec->f60 = -1;
    rec->f48 = 0xC;
    rec->f4C = 0xC;
    rec->f4A = 0xC;
    rec->f4E = 0xC;
    rec->f50 = 0xC;
    rec->f5C = 0xC;
    rec->f09 = 2;
    rec->f64 = 2;
    rec->f68 = 2;

    cfg->fC8 = D_0009DB00;
    cfg->fCC.b0 = flag;
    cfg->fCC.b1 = 1;
    cfg->fCC.b2 = flag;
    cfg->fCC.b3 = flag;
    cfg->fCC.b4 = flag;
    cfg->fCC.b5 = flag;
    cfg->fCD.mid = 2;
    cfg->fD0 = 0xA;
    cfg->fD2 = 0xA;
    cfg->fF4 = 0xF0;
    cfg->fF6 = 0x10;
    cfg->f100 = (void *) func_000007C8;

    rec->f08 = 0;
    rec->f0C = 0;
    rec->f0E = 0;
    rec->f38 = 0;
    rec->f58 = 0;

    ehsys_2012884C(cfg);

    y = D_000A3F08;
    v = y - 0xA;
    lim = v;
    v = D_000A4030;
    cfg->fD0 = y;
    cfg->fD4 = __builtin_allegrex_min(v, lim);
    cfg->fD6.lo = __builtin_allegrex_max(0, (v - y) + 0xA);
}

/* func_00000464 — tail-calls into ehsys_E1139F1A with the config blob.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000464(void) {
    ehsys_E1139F1A(&D_000A3F0C);
}

/* func_00000470 — per-frame update for mode 2: tracks a load counter and,
 * once a completion flag is set, transitions to mode 3 or 4.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0).
 *
 * Two findings got this over the line, both previously mis-diagnosed as
 * "uncontrollable MWCC codegen":
 *  - the clamp is NOT reachable from portable C (no `x<0?0:x` phrasing, no
 *    optimisation level, and none of the 11 installed builds emit it) — it is
 *    the compiler intrinsic `__builtin_allegrex_max`. See the file header.
 *  - the tail's branch polarity needs a single-case `switch`; see the body. */
void func_00000470(void) {
    volatile int *s1 = &D_0009DB00;
    void *obj = ehsys_F5E3080C();
    int result = ehsys_39ADDA9F(&D_000A3F0C);
    int flags = *(int *) ((char *) obj + 0xC);
    int changed = 0;
    int counter;

    /* 0x194E = 0x6538/4: indexed off s1 so the offset folds into the load and
     * store immediates; hoisting it into a `volatile int *counter_p` instead
     * makes MWCC materialise the address in its own register (+2 words). */
    if (flags & 0x100) {
        changed = 1;
        counter = __builtin_allegrex_max(0, s1[0x194E] - 1);
        s1[0x194E] = counter;
    } else if (flags & 0x200) {
        changed = 1;
        counter = s1[0x194E] + 1;
        s1[0x194E] = counter;
    }

    if (changed) {
        ehsys_BE3756D0(s1[0x194E], changed);
    }

    /* single-case `switch`, not an `if`: the switch is what makes MWCC branch
     * INTO the body (`beql`, body instruction in the delay slot) and give the
     * fall-through path its own `b` to the epilogue. Every `if`/`goto` phrasing
     * folds those two into one `bnel`-skip and comes out a word short. */
    switch (*(volatile unsigned char *) ((volatile char *) s1 + 0x64F9)) {
    case 6:
        if (result == -2) {
            func_000001C8(4);
            break;
        }
        *(volatile int *) ((volatile char *) s1 + 0x6530) = result;
        func_000001C8(3);
        break;
    }
}

/* func_00000540 — per-frame update for mode 3: waits for the fade to
 * finish, sets up the fade-to-black overlay, then hands off to mode 2.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000540(void) {
    volatile int *s0 = &D_0009DB00;
    ehsys_70CB33BE(0, 1);
    goto test;
loop:
    ehsys_frame_sync();
test:
    if (!ehsys_DCA0DCE1()) {
        goto loop;
    }
    ehsys_50EA4D71(&D_00085B00, 0x18000);
    *(volatile int *) ((volatile char *) s0 + 0x6534) = 1;
    ehsys_frame_sync();
    ehsys_245526EE();
    func_000001C8(2);
}

/* func_000005C8 — per-frame update: dispatches on the current mode value
 * (D_0009DB04) to run small per-mode housekeeping. The mode==3 handler
 * reads/writes D_0009DB00+0x6530/+0x6534 through the shared state pointer
 * rather than standalone extern symbols (same field-sharing as elsewhere in
 * this file), and D_000A3F0C is similarly state+0x640C here.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0).
 *
 * Two things this function pinned down:
 *  - the dispatch is a goto chain, NOT a plain `switch` over all four cases:
 *    MWCC sorts switch cases ascending regardless of source order, which
 *    cannot produce this target's 3/2/1 test order.
 *  - the state blob is indexed as int[] (0x194D = 0x6534/4) rather than via
 *    recomputed byte offsets; writing the byte form twice makes MWCC CSE the
 *    address into a saved register, while the target folds 0x6534 into the
 *    load and store immediates off $s0.
 *  - the final test of the chain must still be a single-case `switch`; see
 *    the body for why. */
void func_000005C8(void) {
    int mode = D_0009DB04;
    volatile int *state = &D_0009DB00;
    int handle;

    if (mode == 3) {
        goto case3;
    }
    if (mode == 2) {
        goto case2;
    }
    /* the LAST test of the chain must be a single-case `switch`, not an `if`
     * (the earlier two are plain `if`s — see the target's `beq`/`beql` mix).
     * With an `if` here MWCC schedules the fall-through block's
     * `addu $a1,$a0,$zero` down into the following `jal`'s delay slot; the
     * `switch` leaves that slot as the target's `nop`. The identical call in
     * the mode==1 block DOES get its slot filled either way, which is what
     * made this look like an uncontrollable scheduling asymmetry. */
    switch (mode) {
    case 1:
        goto case1;
    }
    ehsys_942B03D0(0x10, 0x10);
    ehsys_C87BAB32(0, 0, &D_00005A7C);
    return;
case1:
    ehsys_942B03D0(0x10, 0x10);
    ehsys_C87BAB32(0, 0, &D_00005A88);
    return;
case2:
    handle = ehsys_DFCA450B(1, 0);
    ehsys_EF9B5D06(&handle, (void *) ((volatile char *) state + 0x640C));
    ehsys_1EC5342B(handle);
    return;
case3:
    /* indexed as int[] (0x194D = 0x6534/4, 0x194C = 0x6530/4) rather than via a
     * recomputed byte-offset address: writing the byte form twice makes MWCC
     * CSE the address into a saved register, while the target folds 0x6534
     * into the load and the store immediates off $s0. */
    if (state[0x194D] != 0) {
        int slot = state[0x194C];
        ehsys_B48A11FB((void *) ((volatile char *) state + (slot << 8) + 8), 0xF3F9);
        state[0x194D] = 0;
    }
}

/* func_000006B4 — empty callback (registered alongside func_000005C8 in
 * func_00000034).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000006B4(void) {
}

/* func_000006BC — walks a linked list of directory/file entries (via
 * ehsys_sceIoDopen/ehsys_sceIoDread/ehsys_sceIoDclose), recursing into
 * subdirectories and appending matched entries into the 0x100-byte-record
 * table at D_0009DB00+0x8. The ehsys_sceIoDread out-param is a 0x160-byte
 * directory-entry record: flags at +0x0, the name string at +0x58 (the
 * buffer sits at sp+0x20 and the name is read at sp+0x78; the 0x160 size is
 * fixed by the target's 0x180 frame, and is the one value that puts the
 * buffer at 0x20 rather than 0x28).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0).
 *
 * The draft was 4 words over because the slot-count bound was written as a
 * guard at the top of the body. It is really the second half of the loop
 * condition, and the value it loads is REUSED as the slot index — the
 * target's `sll $v0,$v1,8` consumes the very $v1 that the condition's
 * `lw $v1,0x6408($s0)` produced. Writing it as a guard meant three separate
 * reads of the field inside the loop, which MWCC then hoisted into its own
 * cached base register (+2 words, +1 saved register to spill and restore).
 * Folding it into the condition and indexing the blob as int[]
 * (state[0x1902] = 0x6408/4) instead of a recomputed byte offset keeps the
 * offset in each load/store immediate, exactly as the target does. This is
 * the same int[]-indexing lever that fixed func_000005C8. */
void func_000006BC(void *arg0) {
    volatile int *state = &D_0009DB00;
    char dirent[0x160];
    int handle = ehsys_sceIoDopen();
    int flags;
    int count;

    /* the slot-count bound is the SECOND half of the loop condition, not a
     * guard at the top of the body, and the value it loads is reused as the
     * slot index below (the target's `sll $v0,$v1,8` consumes the very $v1
     * the condition's `lw` produced). Hoisting the check into the body is
     * what made MWCC cache `state + 0x6408` in its own base register. */
    while (ehsys_sceIoDread(handle, dirent) > 0 &&
           (count = state[0x1902]) < 0x64) {
        flags = *(int *) dirent;
        if (flags & 0x1000) {
            if (*(signed char *) (dirent + 0x58) != 0x2E) {
                int len = ehsys_strlen(arg0);
                ehsys_strcat(arg0, dirent + 0x58);
                ehsys_strcat(arg0, &D_00005AA4);
                func_000006BC(arg0);
                ((char *) arg0)[len] = 0;
            }
        } else if (flags & 0x2000) {
            char *slot = (char *) state + (count << 8) + 8;
            ehsys_strcpy(slot, arg0);
            ehsys_strcat(slot, dirent + 0x58);
            state[0x1902] = state[0x1902] + 1;
        }
    }
    ehsys_sceIoDclose(handle);
}

/* func_000007C8 — the per-entry render callback. func_00000294 stores its
 * address into the viewer config at D_000A3F0C+0x100, and the UI layer calls
 * it back once per visible list row.
 *
 * Behaviour:
 *   - arg1 is a row index into the 0x100-byte record table that begins at
 *     D_0009DB00+0x8; the record's name string starts at +0x8 within it,
 *     hence `&D_0009DB00 + ((arg1 & 0xFFFF) << 8) + 8`;
 *   - that byte string is widened into a 0x100-entry short[] on the stack,
 *     one byte per halfword, NUL-terminated — a hand-rolled widening loop,
 *     not a library call;
 *   - it then sets the text colour/style (ehsys_942B03D0 / ehsys_60B55A50
 *     with 0xFF000000) and hands the widened label to func_00000894, which
 *     applies the <<6 fixed-point coordinate scaling and calls the draw stub;
 *   - arg2/arg3 are the row's x/y, both offset by +2 before drawing.
 *
 * MATCHING NOTES:
 *  - the copy loop is written test-first with explicit `goto`s to reproduce
 *    the target's bottom-tested layout;
 *  - arg0 looked unused and had been typed that way, which was WRONG and cost
 *    a lot of time. It IS forwarded as the first argument of ehsys_CABEA834:
 *    the target never writes $a0 before that jal and sets only $a1 = -1 in
 *    its delay slot. Because $a0 is therefore live, it is unavailable for the
 *    copy-loop pointer, which is why the target keeps that pointer in $a1 —
 *    a register-allocation difference that was really a dataflow bug. See the
 *    CAUTION paragraph in the file header.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000007C8(void *arg0, int arg1, int arg2, int arg3) {
    short label[0x100];
    signed char *src = (signed char *) &D_0009DB00 + ((arg1 & 0xFFFF) << 8) + 8;
    short *dst = label;

    goto test;
loop:
    *dst = (short) *src;
    dst++;
    src++;
test:
    if (*src != 0) {
        goto loop;
    }
    *dst = 0;

    ehsys_B8AD96EA(ehsys_E6E4401F(ehsys_CABEA834(arg0, -1), 0, 1));
    ehsys_942B03D0(0xC, 0xC);
    ehsys_60B55A50(0xFF000000);
    func_00000894(arg2 + 2, arg3 + 2,
                   (void *) (label + ehsys_strlen(
                                          (void *) (long) ehsys_7B552C34(3))));
    ehsys_A4AFF8E6();
}

/* func_00000894 — tail-calls the real draw routine with geometry derived
 * from arg0 (a "used width" the caller subtracts from a fixed 0x1E0
 * total).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000894(int arg0, int arg1, void *arg2) {
    ehsys_B89D38DC(arg0 << 6, arg1 << 6, (0x1E0 - arg0) << 6, arg2);
}

/*
 * rel_tutoriallist.prx — reconstructed code (matching decompilation)
 *
 * The "tutorial list" screen: two category tabs of tutorial entries read
 * from database/tutoriallist.ehp, a scrollable list with variable-height
 * rows, and a result (picked entry or cancel) for the caller.
 *
 * Compiler:     Metrowerks CodeWarrior for PSP — MWCC 1.3 SP7 (mwccpsp_3.0.1_219)
 * Flags:        -O4,s -sdatathreshold 0
 * Verification: scripts/mwcc_build.sh + scripts/mwcc_diff.py, one function
 *               at a time. 25 of 31 functions are "MATCH 100%"; the other six
 *               are close DRAFTS, each with its remaining diff in its comment.
 *               `make MODULE=rel_tutoriallist SRC=1` therefore still FAILS
 *               (func_00001880/func_00001C68 differ in size, which shifts
 *               every later address and the import stubs).
 *
 * Exports (libtutoriallist_rel): func_00000000 (draw), func_000026C0 (init),
 * func_000027E4 (update / state machine), func_00002950 (get result).
 * Proposed names for these and the module's data are in
 * config/symbols/rel_tutoriallist.extra.txt; this file keeps the D_/func_
 * placeholders because the generated symbol file (and so the asm the diff
 * tool compares against) still uses them.
 *
 * No Shift-JIS text in this module: every string in .data is ASCII
 * ("tab.gim", "parts.gim", "bg.ehf", "strTbl_%c.bin",
 * "database/tutoriallist.ehp", "jegfis").
 *
 * MWCC codegen levers found here (in addition to rel_movie_viewer.c's):
 *  - COMPARISON SPELLING DECIDES THE BRANCH SHAPE. The same test compiles to
 *    a fused `slt $at` + branch, or to a materialised `slt $rN` + `bnez`, and
 *    the two put the then/else blocks in opposite order:
 *      `(i >= n) ? 0x26 : tbl[i]`  -> materialised (the GetEntry ternaries),
 *      `if (k > cur) break;`       -> fused `slt at,cur,k; bnez at`,
 *      `if (cur < k) break;`       -> materialised (wrong here),
 *      `if (total <= visible) A; else B;` -> fused, B is the branch target.
 *    When a target branch looks "inverted", try the mirrored operator.
 *  - UNREACHABLE DELAY-SLOT COPIES. After filling a branch-likely delay
 *    slot, MWCC sometimes leaves the original instruction behind as dead
 *    code after a `b` (func_00000000 has three). This is not a compiler
 *    version or flag difference (checked builds 180..219, -O2..-O4, p/s,
 *    peephole/scheduling pragmas): it follows the function's control flow.
 *    func_00000000 only gets them when its state test is an early-return
 *    `switch` instead of an enclosing `if`, and func_00001C68 only LOSES
 *    them (as the target has) when written as an if/else-if chain instead of
 *    early returns.
 *  - `static inline` functions containing loops ARE inlined (ScrollDown).
 *  - Local-array initialisers (`char s[6] = "..."`) emit .data that a
 *    SRC=1 build cannot link; see func_000024B4 for the equivalent spelling.
 */

typedef unsigned short u16;
typedef unsigned char u8;

extern int ehsys_DFCA450B(int, int);
extern int ehsys_1D98951B(int, int);
extern int ehsys_42110BF9(int, int);
extern int ehsys_399AC68A(int, int);
extern int ehsys_C62F9EAC(int, int);
extern void ehsys_1EC5342B(int);
extern int ehsys_F462EE6E(int, int);
extern int ehsys_09794DF0(int, int, int);
extern int ehsys_E6E4401F(int, int, int);
extern int ehsys_5CBB9E43(int, int, int, int, int, int);
extern int ehsys_4175E3DB(int, int);
extern int ehsys_20560342(int, int, int, int, int);
extern void ehsys_878EE1D3(int *);
extern void ehsys_memset(void *, int, int);
extern int ehsys_85F89FE8(int);
extern int ehsys_7187E0F0(int, int, void *);
extern int ehsys_89812D8A(int);
extern int ehsys_9EA6989A(int, int);
extern int ehsys_CABEA834(int, int);
extern int ehsys_8EA6B1E7(int, int, int, int, int);
extern int ehsys_FDD168CA(int, int, int, int, int, int, int, int);
extern void ehsys_942B03D0(int, int);
extern void ehsys_60B55A50(int);
extern void ehsys_B8AD96EA(int);
extern void *ehsys_E58C0FDC(int, int);
extern int ehsys_A4AFF8E6(void);
extern void ehsys_B89D38DC(int, int, int, void *);
extern void ehsys_3BB2BAC6(int, int, int, int, void *, int, int);
extern int ehsys_get_language(void);
extern void ehsys_AB962AE7(int);

typedef struct {
    unsigned int pad : 3;
    unsigned int blink : 1;
} FrameBits;

typedef struct {
    FrameBits bits;
    int x04;
} FrameInfo;
extern FrameInfo *ehsys_B2B9FEFA(void);
extern int ehsys_97BB99A5(int, void *, int, int);
extern int ehsys_2FD4863E(void);
extern u16 ehsys_get_button_code(int);
extern void ehsys_5998BFD7(int, int, int);
extern void ehsys_6367AE39(int);
extern void ehsys_2AE3C0AD(int);
typedef struct {
    int x00;
    unsigned int trig;
    int x08;
    unsigned int hold;
} PadState;
extern PadState *ehsys_F5E3080C(void);
extern void ehsys_qsort(void *, int, int, int (*)(const void *, const void *));
extern void ehsys_20E340D9(int, int);
extern int ehsys_D2A768F4(int, const char *);
extern int ehsys_41AABF28(int, int);
extern void ehsys_8288EEBC(int, const char *, int, int, int (*)(int, int), int);
extern int ehsys_sprintf(char *, const char *, ...);
extern int ehsys_C6C15111(int, const char *, int);
extern void ehsys_4B0DABFA(int, int);
extern int ehsys_C07BB470(void *, int, int, int, int, int);
extern void ehsys_D979E9BF(int, int *, int);
extern int ehsys_1856E536(int, int);
extern void ehsys_sceKernelChangeCurrentThreadAttr(int, int);
extern void ehsys_C9D9E989(int);
extern void ehsys_4AA58320(int);
extern void ehsys_33ED567B(int);
extern void ehsys_D853F15C(int);
extern void ehsys_D470D0B2(int, int, int);
extern void ehsys_13A4081A(int);
extern int ehsys_450CD6C5(void);
extern void ehsys_F6414A71(int, int);
extern void ehsys_ED1410E0(int);

typedef struct {
    u8 busy : 1;
    u8 count : 7;
    u8 pad;
} LoadState;

typedef struct {
    u16 id;
    u16 pad;
    int data;
    const char *name;
} Res;

typedef struct {
    char c[6];
} LangStr;

extern LoadState D_00007C00;
extern int D_00007C04;
extern Res D_00007B90[4];
extern const char D_00007BC0[];
extern char D_00007BDC[];
extern char D_00007BE4[];

/* the list state, cleared as one 0x10C-byte block by func_000026C0 */
typedef struct {
    u8 cat : 4;  /* selected category (tab), 0 or 1 */
    u8 flag : 1; /* set when the list was cancelled */
} Bits;

typedef struct {
    u16 state;          /* 0x00: 0 load, 1 fade in, 2 input, 3 fade out, 4 done */
    u16 pad02;
    Bits bits;          /* 0x04 */
    u16 x06;            /* 0x06: selected entry id (in: initial, out: result) */
    u16 a08[2];         /* 0x08: per category, index of the first visible row */
    u16 a0C[2];         /* 0x0C: per category, cursor row relative to a08 */
    u16 a10[2];         /* 0x10: per category, number of entries */
    u16 tbl[2][38];     /* 0x14: per category, entry ids */
    u16 aAC[38];        /* 0xAC: per entry id, height in 24-px lines */
    short xF8;          /* 0xF8: set to 1 once the lists are built */
    int xFC;            /* 0xFC: heap handle */
    int x100;           /* 0x100: heap block id if we allocated it (else 0) */
    int x104;           /* 0x104: heap block address */
    int x108;           /* 0x108 */
} TutList;

extern TutList D_00007C10;
#define G D_00007C10

int func_00000454(int h);
int func_00000510(int h);
void func_00000658(int x, int y, void *p);
int func_00000678(int h, int x, int y, int w, int hgt);
void func_00000B8C(int x, int y, int w, void *p);
int func_00000B9C(int h, int x, int y, int idx);
void func_00000D58(int a, int b, int c, int d, void *p);
int func_00000D74(int h, int x, int y, int w, int hgt);
int func_00000FD8(int h, int x, int y, int w, int hgt);
int func_000014E8(int h, int x, int y, int hgt, int pos, int visible, int total);
int func_00001264(int h, int x, int y, u8 sel, unsigned int period, int show, int bg, unsigned int t, int tex);
int func_000017C0(void);
void func_000017E0(int x, int y, int n);
int func_00001C5C(int a, void *b);
int func_000024B4(u16 id);
void func_00001880(void);
void func_00001C68(void);
void func_000022B8(void);
int func_0000231C(void);
void func_000023C4(void);
void func_000023CC(void);
int func_00002404(void);
int func_000025A0(int p, int ok);
int func_00002630(const void *a, const void *b);
int func_00002640(u16 key, u16 *arr, int n, int size);
int func_00002908(void);

static inline int GetEntry(int cat, int i) {
    return (i >= G.a10[cat]) ? 0x26 : G.tbl[cat][i];
}

static inline void ScrollDown(void) {
    int cat = G.bits.cat;
    int top = G.a08[cat];
    int cur = G.a0C[cat];
    int h, sum, k, w;

    h = G.aAC[GetEntry(cat, top + cur + 1)];
    sum = 0;
    for (k = 0; k < 8; k++) {
        w = G.aAC[GetEntry(cat, top + k)];
        if (k > G.a0C[cat]) {
            break;
        }
        sum += w;
    }
    if (sum + h > 8) {
        top++;
        h = G.aAC[GetEntry(cat, top)];
        G.a08[cat]++;
        sum = 0;
        for (k = 1; k < 9; k++) {
            cat = G.bits.cat;
            sum += G.aAC[GetEntry(cat, top + k)];
            if (h <= sum) {
                G.a08[cat] += (u16) k;
                cat = G.bits.cat;
                if (G.a0C[cat] - k < 0) {
                    G.a0C[cat] = 0;
                } else {
                    G.a0C[cat] -= (u16) k;
                }
                break;
            }
        }
    } else {
        G.a0C[cat]++;
    }
}

/* func_00000000 — exported (NID 0xC1B2B589): per-frame DRAW of the tutorial
 * index (proposed name TutList_Draw). Builds the draw list: background
 * (func_00000454), header bar (func_00000510), category tabs (func_00000678),
 * scrollbar (func_000014E8), up to 8 rows of list items (func_00000B9C) and
 * the blinking cursor frame (func_00000FD8). Nothing is drawn in state 0
 * (loading) or 4 (finished).
 *
 * DRAFT — 2 words off: the loop-invariant hoist order at the head of the
 * first loop (`addiu t1,zero,0x26` and `addu t4,a0,a2` are swapped). Every
 * other word matches, including the three unreachable delay-slot copies the
 * target keeps after `b`/branch-likely pairs (see file header: they only
 * appear because the state test is an early-`return` switch rather than an
 * enclosing `if`). Declaration order, loop form, GetEntry spellings and
 * decomp-permuter (1800 iterations) did not move the two hoisted values. */
void func_00000000(void) {
    int h;
    int i, w, sum, s1, s0, idx, y;
    int cat;

    h = ehsys_DFCA450B(0, 0);
    h = ehsys_1D98951B(h, 0xFF000000);
    h = ehsys_42110BF9(h, 0);
    h = ehsys_399AC68A(h, 0);
    h = ehsys_C62F9EAC(h, 0x17);
    ehsys_1EC5342B(h);
    switch (G.state) {
    case 0:
    case 4:
        return;
    }
    h = ehsys_DFCA450B(4, 0);
    h = ehsys_F462EE6E(h, 9);
    h = ehsys_F462EE6E(h, 4);
    h = ehsys_09794DF0(h, 0, 1);
    h = ehsys_E6E4401F(h, 0, 0);
    h = ehsys_5CBB9E43(h, 0, 2, 3, 0, 0);
    h = ehsys_4175E3DB(h, 1);
    h = func_00000454(h);
    h = func_00000510(h);
    h = func_00000678(h, 0x4B, 0x27, 0x14B, 0xDB);

    cat = G.bits.cat;
    sum = 0;
    s1 = 0;
    s0 = 0;
    for (i = 0; i < G.a10[cat]; i++) {
        idx = GetEntry(cat, i);
        w = G.aAC[idx];
        sum += w;
        if (i < G.a08[cat] + G.a0C[cat]) {
            s0 += w;
        }
        if (i < G.a08[cat]) {
            s1 += w;
        }
    }
    h = func_000014E8(h, 0x18A, 0x42, 0xB3, s1, 8, sum);
    h = ehsys_20560342(h, 0, 0, 0x1E0, 0xFE);
    h = ehsys_F462EE6E(h, 2);

    for (i = 0, y = 0; y < 8; i++) {
        cat = G.bits.cat;
        w = G.a08[cat] + i;
        if (w >= G.a10[cat]) {
            break;
        }
        idx = GetEntry(cat, w);
        h = func_00000B9C(h, 0x51, y * 24 + 0x3C, idx);
        y += G.aAC[idx];
    }

    cat = G.bits.cat;
    if (G.a10[cat] > 0) {
        w = G.a08[cat] + G.a0C[cat];
        idx = GetEntry(cat, w);
        h = func_00000FD8(h, 0x6E, (s0 - s1) * 24 + 0x3E, 0x117, G.aAC[idx] * 24 - 5);
    }
    h = ehsys_4175E3DB(h, 2);
    ehsys_1EC5342B(h);
    h = ehsys_DFCA450B(5, 0);
    h = ehsys_4175E3DB(h, 1);
    ehsys_878EE1D3(&h);
    ehsys_1EC5342B(h);
}

typedef struct {
    int a;
    char rest[0x5C];
} Param60;

/* func_00000454 — draw the "bg.ehf" (res 0x1011) background layer.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000454(int h) {
    Param60 prm;
    int tex = func_000024B4(0x1011);
    if (tex == 0) {
        return h;
    }
    ehsys_memset(&prm, 0, 0x60);
    prm.a = *(int *) ehsys_B2B9FEFA();
    h = ehsys_85F89FE8(h);
    h = ehsys_7187E0F0(h, tex, &prm);
    h = ehsys_89812D8A(h);
    h = ehsys_F462EE6E(h, 4);
    h = ehsys_09794DF0(h, 0, 1);
    return ehsys_5CBB9E43(h, 0, 2, 3, 0, 0);
}

/* func_00000510 — draw the title bar from parts.gim (res 0x1003) plus its
 * caption (string 1 of the strTbl, res 0x1020).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000510(int h) {
    int tex = func_000024B4(0x1003);
    if (tex == 0) {
        return h;
    }
    h = ehsys_9EA6989A(h, tex);
    h = ehsys_CABEA834(h, -1);
    h = ehsys_E6E4401F(h, 0, 0);
    h = ehsys_8EA6B1E7(h, 0xC, 0x17, 0xC, 0x17);
    h = ehsys_FDD168CA(h, 0, 0, 0, 0, 0x14, 0, 0);
    h = ehsys_FDD168CA(h, 0x1D4, 0, 0, 0xD, 0x14, 0, 0);
    h = ehsys_8EA6B1E7(h, 1, 0x17, 0x1C8, 0x17);
    h = ehsys_FDD168CA(h, 0xC, 0, 0, 0xC, 0x14, 0, 0);
    ehsys_942B03D0(0xE, 0xE);
    ehsys_60B55A50(-1);
    ehsys_B8AD96EA(h);
    func_00000658(8, 4, ehsys_E58C0FDC(func_000024B4(0x1020), 1));
    return ehsys_A4AFF8E6();
}

/* func_00000658 — draw a text line at (x, y) spanning to x = 480 - x
 * (coordinates are passed to the engine in 1/64 px, hence the << 6).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000658(int x, int y, void *p) {
    ehsys_B89D38DC(x << 6, y << 6, (0x1E0 - x) << 6, p);
}

/* func_00000678 — draw the list window frame (9-slice from tab.gim/parts.gim)
 * and the two category tabs; the selected category (G.bits.cat) is drawn
 * lit, the other one dimmed (0x80FFFFFF / 0xFF808080) and shifted down 2 px.
 *
 * DRAFT — instruction-for-instruction identical in shape (325 words, same
 * order), only the callee-saved register numbering differs: the target keeps
 * cat*32 in $s0 and y+15 in $s1 and then reuses those two registers for the
 * tab width / loop counter, ours swaps them. Pure register-allocation. */
int func_00000678(int h, int x, int y, int w, int hgt) {
    int tex1, tex2;
    int v, y1, y2, x2;
    int i, cw, u0, u1, dy;

    tex1 = func_000024B4(0x1002);
    tex2 = func_000024B4(0x1003);
    if (tex1 == 0) {
        return h;
    }
    h = ehsys_9EA6989A(h, tex1);
    h = ehsys_CABEA834(h, -1);
    h = ehsys_E6E4401F(h, 0, 0);
    v = G.bits.cat << 5;
    h = ehsys_8EA6B1E7(h, 6, 6, 6, 6);
    y1 = y + 0xF;
    h = ehsys_FDD168CA(h, x, y1, 0, 0x40, v, 0, 0);
    y2 = y + hgt - 6;
    h = ehsys_FDD168CA(h, x, y2, 0, 0x40, v + 0xB, 0, 0);
    h = ehsys_8EA6B1E7(h, 0xE, 6, 0xE, 6);
    x2 = x + w - 0xE;
    h = ehsys_FDD168CA(h, x2, y1, 0, 0x62, v, 0, 0);
    h = ehsys_FDD168CA(h, x2, y2, 0, 0x62, v + 0x1A, 0, 0);
    h = ehsys_8EA6B1E7(h, 1, 6, w - 0x14, 6);
    h = ehsys_FDD168CA(h, x + 6, y1, 0, 0x46, v, 0, 0);
    h = ehsys_FDD168CA(h, x + 6, y2, 0, 0x46, v + 0xB, 0, 0);
    h = ehsys_8EA6B1E7(h, 6, 1, 6, hgt - 0x1B);
    h = ehsys_FDD168CA(h, x, y1 + 6, 0, 0x40, v + 6, 0, 0);
    h = ehsys_8EA6B1E7(h, 0xE, 1, 0xE, hgt - 0x1B);
    h = ehsys_FDD168CA(h, x2, y1 + 6, 0, 0x62, v + 6, 0, 0);
    h = ehsys_8EA6B1E7(h, 1, 1, w - 0x14, hgt - 0x1B);
    h = ehsys_FDD168CA(h, x + 6, y1 + 6, 0, 0x46, v + 6, 0, 0);
    ehsys_942B03D0(0xD, 0xD);

    for (i = 0; i < 2; i++) {
        cw = w / (2 - i);
        dy = 0;
        w -= cw;
        if (i == 0) {
            u0 = 0;
        } else {
            u0 = 0x10;
        }
        if (i == 1) {
            u1 = 0x30;
        } else {
            u1 = 0x20;
        }
        if (i == G.bits.cat) {
            h = ehsys_CABEA834(h, -1);
            ehsys_60B55A50(-1);
        } else {
            u0 += 8;
            u1 += 8;
            dy = 2;
            h = ehsys_CABEA834(h, 0x80FFFFFF);
            ehsys_60B55A50(0xFF808080);
        }
        h = ehsys_E6E4401F(h, 0, 0);
        h = ehsys_9EA6989A(h, tex1);
        h = ehsys_8EA6B1E7(h, 8, 0x12, 8, 0x12);
        h = ehsys_FDD168CA(h, x, y, 0, u0, i << 5, 0, 0);
        h = ehsys_FDD168CA(h, x + cw - 8, y, 0, u1, i << 5, 0, 0);
        h = ehsys_8EA6B1E7(h, 1, 0x12, cw - 0x10, 0x12);
        h = ehsys_FDD168CA(h, x + 8, y, 0, u0 + 4, i << 5, 0, 0);
        if (i != G.bits.cat) {
            h = ehsys_20560342(h, x, y + dy, cw - 8, 0xD);
            h = ehsys_F462EE6E(h, 2);
        }
        if (tex2 != 0) {
            h = ehsys_9EA6989A(h, tex2);
            h = ehsys_8EA6B1E7(h, 0x10, 0x10, 0x10, 0x10);
            h = ehsys_FDD168CA(h, x + 4, y + 2 + dy, 0, i << 4, 0, 0, 0);
        }
        ehsys_B8AD96EA(h);
        func_00000B8C(x + 0x14, y + 4 + dy, cw - 8, ehsys_E58C0FDC(func_000024B4(0x1020), i + 2));
        h = ehsys_4175E3DB(ehsys_A4AFF8E6(), 2);
        x += cw;
    }
    return h;
}

/* func_00000B8C — draw a text line of width w at (x, y), 1/64 px units.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000B8C(int x, int y, int w, void *p) {
    ehsys_B89D38DC(x << 6, y << 6, w << 6, p);
}

/* func_00000B9C — draw one list row: number badge, frame (func_00000D74),
 * the entry number (func_000017E0; entries 8+ are numbered from 1 again) and
 * the entry title (strTbl string idx + 4). Height = aAC[idx] lines of 24 px.
 * The `(idx >= 8) ? idx - 8 : idx` form matters: `>=` makes MWCC materialise
 * the compare (`slti v0` + `bnez`), `idx < 8 ? ..` gives a fused `slt at`.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000B9C(int h, int x, int y, int idx) {
    int tex;
    int hgt;
    int h2;

    tex = func_000024B4(0x1003);
    if (tex == 0) {
        return h;
    }
    h = ehsys_9EA6989A(h, tex);
    h = ehsys_CABEA834(h, -1);
    h = ehsys_E6E4401F(h, 0, 0);
    h = ehsys_8EA6B1E7(h, 0x19, 0x15, 0x19, 0x15);
    h = ehsys_FDD168CA(h, x, y + 1, 0, 0, 0x2B, 0, 0);
    hgt = G.aAC[idx] * 24;
    h2 = func_00000D74(h, x + 0x1B, y, 0x11B, hgt - 1);
    if (ehsys_get_language() == 0) {
        ehsys_AB962AE7(0x280);
    } else {
        ehsys_AB962AE7(0x180);
    }
    ehsys_B8AD96EA(h2);
    ehsys_942B03D0(0xC, 0xE);
    ehsys_60B55A50(-1);
    func_000017E0(x + 4, y + 4, ((idx >= 8) ? idx - 8 : idx) + 1);
    ehsys_942B03D0(0xE, 0xE);
    ehsys_60B55A50(0xFF000000);
    func_00000D58(x + 0x20, y + 5, 0x110, hgt - 7, ehsys_E58C0FDC(func_000024B4(0x1020), idx + 4));
    ehsys_AB962AE7(0);
    return ehsys_A4AFF8E6();
}

/* func_00000D58 — draw a word-wrapped text box (1/64 px units).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000D58(int a, int b, int c, int d, void *p) {
    ehsys_3BB2BAC6(a << 6, b << 6, c << 6, d << 6, p, -1, 0);
}

/* func_00000D74 — draw a 9-slice frame (6 px corners) from parts.gim.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000D74(int h, int x, int y, int w, int hgt) {
    int tex;
    int y2, x2;

    tex = func_000024B4(0x1003);
    if (tex == 0) {
        return h;
    }
    h = ehsys_9EA6989A(h, tex);
    h = ehsys_CABEA834(h, -1);
    h = ehsys_E6E4401F(h, 0, 0);
    h = ehsys_8EA6B1E7(h, 6, 6, 6, 6);
    h = ehsys_FDD168CA(h, x, y, 0, 0x19, 0x2F, 0, 0);
    y2 = y + hgt - 6;
    h = ehsys_FDD168CA(h, x, y2, 0, 0x19, 0x3A, 0, 0);
    h = ehsys_8EA6B1E7(h, 6, 6, 6, 6);
    x2 = x + w - 6;
    h = ehsys_FDD168CA(h, x2, y, 0, 0x24, 0x2F, 0, 0);
    h = ehsys_FDD168CA(h, x2, y2, 0, 0x24, 0x3A, 0, 0);
    h = ehsys_8EA6B1E7(h, 1, 6, w - 0xC, 6);
    h = ehsys_FDD168CA(h, x + 6, y, 0, 0x1F, 0x2F, 0, 0);
    h = ehsys_FDD168CA(h, x + 6, y2, 0, 0x1F, 0x3A, 0, 0);
    h = ehsys_8EA6B1E7(h, 6, 1, 6, hgt - 0xC);
    h = ehsys_FDD168CA(h, x, y + 6, 0, 0x19, 0x35, 0, 0);
    h = ehsys_FDD168CA(h, x2, y + 6, 0, 0x24, 0x35, 0, 0);
    h = ehsys_8EA6B1E7(h, 1, 1, w - 0xC, hgt - 0xC);
    return ehsys_FDD168CA(h, x + 6, y + 6, 0, 0x1F, 0x35, 0, 0);
}

/* func_00000FD8 — draw the cursor frame around the selected row; it
 * "breathes" by 2 px every 8 frames (bit 3 of the engine frame counter,
 * compiled to an Allegrex `ext`).
 *
 * DRAFT — same 163 words in the same order; register allocation differs:
 * the target spills `w` (a3) to its home slot and keeps `hgt` in $fp, ours
 * does the opposite, and the x/y corner pairs swap $s2/$s3 with $s6/$s7.
 * Local declaration order was brute-forced (all 720 orders): no effect. */
int func_00000FD8(int h, int x, int y, int w, int hgt) {
    int tex;
    int y1, x1, x0, y0, half, d;

    tex = func_000024B4(0x1003);
    if (tex == 0) {
        return h;
    }
    d = ((*(unsigned int *) ehsys_B2B9FEFA() >> 3) & 1) * 2;
    h = ehsys_CABEA834(h, -1);
    h = ehsys_E6E4401F(h, 0, 0);
    h = ehsys_9EA6989A(h, tex);
    h = ehsys_8EA6B1E7(h, 6, 6, 6, 6);
    half = d / 2;
    y0 = y - 4 - half;
    x0 = x - 4 - half;
    h = ehsys_FDD168CA(h, x0, y0, 0, 0x19, 0x14, 0, 0);
    x1 = half + (x + w - 2);
    h = ehsys_FDD168CA(h, x1, y0, 0, 0x28, 0x14, 0, 0);
    y1 = half + (y + hgt - 2);
    h = ehsys_FDD168CA(h, x0, y1, 0, 0x19, 0x29, 0, 0);
    h = ehsys_FDD168CA(h, x1, y1, 0, 0x28, 0x29, 0, 0);
    h = ehsys_8EA6B1E7(h, 6, 1, 6, hgt + d - 4);
    y = y + 2 - half;
    h = ehsys_FDD168CA(h, x0, y, 0, 0x19, 0x1A, 0, 0);
    h = ehsys_FDD168CA(h, x1, y, 0, 0x28, 0x1A, 0, 0);
    h = ehsys_8EA6B1E7(h, 1, 6, w + d - 4, 6);
    x = x + 2 - half;
    h = ehsys_FDD168CA(h, x, y0, 0, 0x1F, 0x14, 0, 0);
    return ehsys_FDD168CA(h, x, y1, 0, 0x1F, 0x29, 0, 0);
}

/* func_00001264 — draw one scrollbar arrow (sel 0 = up, 1 = down); when
 * `show` it pulses: alpha = 255 * triangle(t % period) (float math,
 * unsigned->float conversions are MWCC's).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00001264(int h, int x, int y, u8 sel, unsigned int period, int show, int bg, unsigned int t, int tex) {
    unsigned int half;
    int u;
    unsigned int r;
    float f;

    half = period >> 1;
    u = (sel == 1) ? 8 : 0;
    h = ehsys_9EA6989A(h, tex);
    h = ehsys_E6E4401F(h, 0, 0);
    if (bg) {
        h = ehsys_CABEA834(h, -1);
        h = ehsys_8EA6B1E7(h, 8, 8, 8, 8);
        h = ehsys_FDD168CA(h, x, y, 0, 0x2E, u + 0x30, 0, 0);
    }
    if (!show) {
        return h;
    }
    r = t % period;
    if (r < half) {
        f = (float) r / (float) half;
    } else {
        f = (float) (period - r) / (float) half;
    }
    if (f < 0.0f) {
        f = 0.0f;
    }
    if (f > 1.0f) {
        f = 1.0f;
    }
    h = ehsys_CABEA834(h, ((unsigned int) (255.0f * f) << 24) | 0xFFFFFF);
    h = ehsys_8EA6B1E7(h, 8, 8, 8, 8);
    return ehsys_FDD168CA(h, x, y, 0, 0x2E, u, 0, 0);
}

/* func_000014E8 — draw the scrollbar: track, thumb (size and offset from
 * pos/visible/total, clamped with the Allegrex min/max intrinsics) and the
 * two arrows (func_00001264), lit only when scrolling that way is possible.
 *
 * DRAFT — branch structure now matches (`total <= visible` is what gives the
 * target's fused `slt at` + branch to the computed case). Left: register
 * allocation, and the target recomputes `len - thumb` as `move` + `subu`
 * for the min() bound instead of CSE-ing it with the multiply operand. */
int func_000014E8(int h, int x, int y, int hgt, int pos, int visible, int total) {
    int len, thumb, tex, off, y2;

    len = hgt - 2;
    tex = func_000024B4(0x1003);
    if (tex == 0) {
        return h;
    }
    if (total <= visible) {
        thumb = len;
        len = 0;
    } else {
        thumb = __builtin_allegrex_max(visible * len / total, 10);
        off = pos * (len - thumb) / (total - visible);
        len -= thumb;
        len = __builtin_allegrex_min(__builtin_allegrex_max(off, 0), len);
    }
    h = ehsys_E6E4401F(h, 0, 0);
    h = ehsys_CABEA834(h, -1);
    h = ehsys_9EA6989A(h, tex);
    h = ehsys_8EA6B1E7(h, 8, 5, 8, 5);
    h = ehsys_FDD168CA(h, x, y, 0, 0x37, 0, 0, 0);
    y2 = y + hgt;
    h = ehsys_FDD168CA(h, x, y2 - 5, 0, 0x37, 0xF, 0, 0);
    h = ehsys_8EA6B1E7(h, 8, 5, 8, hgt - 0xA);
    h = ehsys_FDD168CA(h, x, y + 5, 0, 0x37, 5, 0, 0);
    h = ehsys_8EA6B1E7(h, 4, 5, 4, 5);
    off = y + len;
    h = ehsys_FDD168CA(h, x + 2, off + 2, 0, 0x37, 0x28, 0, 0);
    h = ehsys_FDD168CA(h, x + 2, thumb + (off - 3), 0, 0x37, 0x37, 0, 0);
    h = ehsys_8EA6B1E7(h, 4, 5, 4, thumb - 0xA);
    h = ehsys_FDD168CA(h, x + 2, off + 7, 0, 0x37, 0x2D, 0, 0);
    h = func_00001264(h, x, y - 7, 0, func_000017C0(), 0 < pos, 1, *(unsigned int *) ehsys_B2B9FEFA(), tex);
    return func_00001264(h, x, y2 + 2, 1, func_000017C0(), pos < total - visible, 1, *(unsigned int *) ehsys_B2B9FEFA(), tex);
}

/* func_000017C0 — engine frame-info word 1 (used as the arrow pulse period).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000017C0(void) {
    return ehsys_B2B9FEFA()->x04;
}

/* byte-wise clear; the NULL test and the count-down do/while are what the
 * target's inlined loop does (an `if (p)` guard on a stack address) */
static inline void ZeroMem(void *dst, int n) {
    char *p = (char *) dst;
    int i = n;
    if (p != 0) {
        do {
            *p++ = 0;
        } while (--i);
    }
}

/* func_000017E0 — draw a decimal number right-aligned at (x, y), one u16
 * glyph at a time (buf is a 2-u16 string). The byte-clear loop with the NULL
 * test is an inlined helper (ZeroMem); a plain memset or `= {0}` differ.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000017E0(int x, int y, int n) {
    u16 buf[2];

    ZeroMem(buf, sizeof(buf));
    if (n < 10) {
        x += 4;
    } else {
        x += 8;
    }
    while (n > 0) {
        buf[0] = n % 10 + '0';
        func_00000658(x, y, buf);
        n /= 10;
        x -= 8;
    }
}

/* func_00001880 — build the two category lists (proposed TutList_BuildList):
 * entries 0..4 go to category 1, 8..23 to category 0 (5..7 are skipped), each
 * entry's height in lines is measured from its title text; then the window is
 * scrolled down until the entry selected last time (G.x06) is under the cursor.
 *
 * DRAFT — 243 vs 247 words. The list-build half matches up to register
 * names; the scroll loop is the same algorithm as the down-key path of
 * func_00001C68, but the target CSEs the outer `while` test values (cat, top,
 * pos, &a08[cat], &a0C[cat]) into the body differently. Using the ScrollDown
 * inline here (as func_00001C68 does) makes it worse (257 words). */
void func_00001880(void) {
    int cat;
    int sel;
    int i, n, lineh;
    int top, cur, pos, k, sum, h;

    cat = 0;
    sel = 0;
    ehsys_942B03D0(0xE, 0xE);
    lineh = ehsys_2FD4863E();
    for (i = 0; i < 0x26; i++) {
        if (i > 4 && i < 8) {
            i = 8;
        } else if (i > 0x17) {
            break;
        }
        if (i >= 8) {
            n = G.a10[0]++;
            G.tbl[0][n] = i;
            if (i == G.x06) {
                cat = 0;
                sel = G.a10[0] - 1;
            }
        } else {
            n = G.a10[1]++;
            G.tbl[1][n] = i;
            if (i == G.x06) {
                cat = 1;
                sel = G.a10[1] - 1;
            }
        }
        G.aAC[i] = (func_00001C5C(0x4400, ehsys_E58C0FDC(func_000024B4(0x1020), i + 4)) + 0x40) / lineh;
    }
    G.xF8 = 1;
    G.bits.cat = cat;

    while (cat = G.bits.cat, top = G.a08[cat], cur = G.a0C[cat], sel != top + cur) {
        n = G.a10[cat];
        if (top + cur >= n - 1) {
            return;
        }
        h = G.aAC[GetEntry(cat, top + cur + 1)];
        sum = 0;
        for (k = 0; k < 8; k++) {
            i = G.aAC[GetEntry(cat, top + k)];
            if (k > G.a0C[cat]) {
                break;
            }
            sum += i;
        }
        if (sum + h > 8) {
            top++;
            h = G.aAC[GetEntry(cat, top)];
            G.a08[cat]++;
            sum = 0;
            for (k = 1; k < 9; k++) {
                cat = G.bits.cat;
                sum += G.aAC[GetEntry(cat, top + k)];
                if (h <= sum) {
                    G.a08[cat] += (u16) k;
                    cat = G.bits.cat;
                    if (G.a0C[cat] - k < 0) {
                        G.a0C[cat] = 0;
                    } else {
                        G.a0C[cat] -= (u16) k;
                    }
                    break;
                }
            }
        } else {
            G.a0C[cat]++;
        }
    }
}

/* func_00001C5C — measure a text string (returns its height in 1/64 px).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00001C5C(int a, void *b) {
    return ehsys_97BB99A5(a, b, 0, 0);
}

/* func_00001C68 — input handler for state 2 (proposed TutList_HandleInput):
 * cancel -> leave with result -1, confirm -> pick the entry (entries >= 8 also
 * trigger ehsys_6367AE39(600)), L/R -> switch category, up/down -> move the
 * cursor, scrolling by item heights, wrapping at both ends on a fresh press.
 *
 * DRAFT — 402 vs 404 words; the confirm/LR/down paths match up to register
 * names. Left: register choice in the up-wrap loop (target keeps the
 * counter in $a1 and the count in $v0, ours the reverse) and the
 * `if (k < 0) k = 0` clamp, which our build folds away. */
void func_00001C68(void) {
    PadState *pad;
    unsigned int trig;
    int cat, top, cur, k, sum, i, last;

    pad = ehsys_F5E3080C();
    trig = pad->trig;
    if (trig & ehsys_get_button_code(1)) {
        G.bits.flag = 1;
        ehsys_5998BFD7(1, 0xFF000000, 0x10);
        ehsys_6367AE39(600);
        G.state = 3;
        ehsys_2AE3C0AD(0x94C5);
    } else if (trig & ehsys_get_button_code(0)) {
        cat = G.bits.cat;
        i = G.a08[cat] + G.a0C[cat];
        if (GetEntry(cat, i) < 0x26) {
            G.x06 = GetEntry(cat, i);
            ehsys_5998BFD7(1, 0xFF000000, 0x10);
            if (G.x06 >= 8) {
                ehsys_6367AE39(600);
            }
            G.state = 3;
            ehsys_2AE3C0AD(0x94C4);
        }
    } else if (trig & 0x83A0) {
        G.bits.cat = (G.bits.cat + 1) & 1;
        ehsys_2AE3C0AD(0x94CC);
    } else if (pad->hold & 0x10) {
        cat = G.bits.cat;
        cur = G.a0C[cat];
        if (cur > 0) {
            G.a0C[cat] = cur - 1;
            ehsys_2AE3C0AD(0x94C3);
        } else if ((top = G.a08[cat]) > 0) {
            G.a08[cat] = top - 1;
            ehsys_2AE3C0AD(0x94C3);
        } else if ((trig & 0x10) && top == 0 && cur == 0) {
            last = G.a10[cat] - 1;
            sum = 0;
            k = 0;
            for (i = last; i > 0; i--) {
                sum += G.aAC[GetEntry(cat, i)];
                if (sum > 7) {
                    break;
                }
                k++;
            }
            top = last - k;
            if (top < 0) {
                top = 0;
            }
            if (k < 0) {
                k = 0;
            }
            G.a08[cat] = top;
            G.a0C[G.bits.cat] = k;
            ehsys_2AE3C0AD(0x94C3);
        }
    } else if (pad->hold & 0x40) {
        cat = G.bits.cat;
        if (G.a08[cat] + G.a0C[cat] < G.a10[cat] - 1) {
            ScrollDown();
            ehsys_2AE3C0AD(0x94C3);
        } else if (trig & 0x40) {
            G.a08[cat] = 0;
            G.a0C[G.bits.cat] = 0;
            ehsys_2AE3C0AD(0x94C3);
        }
    }
}

/* func_000022B8 — reset the archive load state and sort the resource table
 * by id (ehsys_qsort + func_00002630) so func_00002640 can binary-search it.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000022B8(void) {
    ehsys_memset(&D_00007C00, 0, 2);
    D_00007C00.busy = 1;
    ehsys_qsort(D_00007B90, 4, 0xC, func_00002630);
}

/* func_0000231C — poll the archive load: returns 1 once, when the async
 * load completed. The middle test is a `switch` on the counter: an if/else
 * puts the blocks in the opposite order.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_0000231C(void) {
    if (D_00007C00.busy) {
        switch (D_00007C00.count) {
        case 0:
            if (func_00002404() != 0) {
                D_00007C00.count++;
            }
            break;
        default:
            D_00007C00.count = 0;
            return 1;
        }
    }
    return 0;
}

/* func_000023C4 — tail-calls func_000023CC.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000023C4(void) {
    func_000023CC();
}

/* func_000023CC — free the loaded archive buffer.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000023CC(void) {
    if (D_00007C04 != 0) {
        ehsys_20E340D9(G.xFC, D_00007C04);
        D_00007C04 = 0;
    }
}

/* func_00002404 — return the archive buffer, starting the async load of
 * "database/tutoriallist.ehp" (callback func_000025A0) if not loaded yet.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00002404(void) {
    int size;
    int buf;

    if (D_00007C04 != 0) {
        return D_00007C04;
    }
    if (D_00007C00.busy) {
        D_00007C00.busy = 0;
        size = ehsys_D2A768F4(3, D_00007BC0);
        buf = ehsys_41AABF28(G.xFC, size);
        D_00007C04 = buf;
        ehsys_8288EEBC(3, D_00007BC0, buf, size, func_000025A0, buf);
    }
    return 0;
}

/* func_000024B4 — get (and cache) a resource of the archive by id: 0x1002
 * tab.gim, 0x1003 parts.gim, 0x1011 bg.ehf, 0x1020 strTbl_<lang>.bin where
 * <lang> is a letter of "jegfis" (Japanese, English, German, French,
 * Italian, Spanish) indexed by ehsys_get_language().
 * The original source had `char langs[6] = "jegfis";` — MWCC puts that
 * initialiser in .data (it IS D_00007BDC) and copies it with this 3x2-byte
 * loop. We cannot emit .data from this object (the Makefile links the
 * shipped .data blob verbatim), so the copy is written as a struct copy from
 * the extern, which compiles to the identical loop and relocation.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000024B4(u16 id) {
    Res *e;
    int ehp;
    char name[0x100];
    LangStr langs;
    int i;

    i = func_00002640(id, (u16 *) D_00007B90, 4, 0xC);
    if (i < 0) {
        return 0;
    }
    e = &D_00007B90[i];
    if (e->data != 0) {
        return e->data;
    }
    ehp = func_00002404();
    /* original: char langs[6] = "jegfis"; (initializer lives in .data) */
    langs = *(LangStr *) D_00007BDC;
    if (ehp == 0) {
        return 0;
    }
    ehsys_sprintf(name, e->name, langs.c[ehsys_get_language()]);
    return e->data = ehsys_C6C15111(ehp, name, 0);
}

/* func_000025A0 — archive load-complete callback.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000025A0(int p, int ok) {
    int r = 0 < ok;
    if (r) {
        if (p != 0) {
            ehsys_4B0DABFA(0, p);
        }
        D_00007C00.count++;
    }
    D_00007C00.busy = 1;
    return r;
}

/* func_00002630 — qsort comparator on the u16 resource id.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00002630(const void *a, const void *b) {
    return *(const u16 *) a - *(const u16 *) b;
}

/* func_00002640 — binary search for a u16 key in a table of `size`-byte
 * records whose first field is the key; returns the index or -1.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00002640(u16 key, u16 *arr, int n, int size) {
    int lo, hi, mid;
    int stride = size / 2;

    hi = n - 1;
    lo = 0;
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        if (key == arr[mid * stride]) {
            return mid;
        }
        if (arr[mid * stride] < key) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return -1;
}

/* func_000026B8 — empty stub returning 0.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000026B8(void) {
    return 0;
}

/* func_000026C0 — exported (NID 0x41015B0F), proposed TutList_Init(sel,
 * heap): clear the state, allocate a 2 MB heap unless one is passed in,
 * set up the resource table and the initially selected entry (8 if sel < 0).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000026C0(int sel, int heap) {
    ehsys_memset(&G, 0, 0x10C);
    if (heap == 0) {
        G.x100 = ehsys_C07BB470(D_00007BE4, 2, 0, 0x200000, 1, 0);
        ehsys_D979E9BF(G.x100, &G.x104, 0);
        G.xFC = ehsys_1856E536(G.x104, 0x200000);
    } else {
        G.xFC = heap;
    }
    ehsys_sceKernelChangeCurrentThreadAttr(0, 0x4000);
    func_000022B8();
    if (sel >= 0) {
        G.x06 = sel;
    } else {
        G.x06 = 8;
    }
    ehsys_C9D9E989(1);
    ehsys_4AA58320(1);
    ehsys_AB962AE7(0);
    if (ehsys_get_language() == 0) {
        ehsys_33ED567B(1);
        ehsys_D853F15C(0);
    } else {
        ehsys_33ED567B(0);
        ehsys_D853F15C(1);
    }
}

/* func_000027E4 — exported (NID 0xCF2830FF), proposed TutList_Update: the
 * state machine 0 load -> 1 fade in -> 2 input -> 3 fade out -> 4 shutdown;
 * returns 0 while running, 1 when an entry was picked, 2 when cancelled.
 * (A `default: return 0;` case adds a stray `b`; the target has none.)
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000027E4(void) {
    switch (G.state) {
    case 0:
        if (func_0000231C()) {
            func_00001880();
            G.state = 1;
            ehsys_D470D0B2(1, 0xFF000000, 0x10);
            ehsys_13A4081A(0);
        }
        break;
    case 1:
        if (ehsys_450CD6C5()) {
            G.state = 2;
        }
        break;
    case 2:
        func_00001C68();
        break;
    case 3:
        if (ehsys_450CD6C5()) {
            G.state = 4;
        }
        break;
    case 4:
        if (func_00002908()) {
            return G.bits.flag ? 2 : 1;
        }
        break;
    }
    return 0;
}

/* func_00002908 — shutdown: free the archive and, if we allocated it, the heap.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00002908(void) {
    func_000023C4();
    if (G.x100 > 0) {
        ehsys_F6414A71(G.x100, G.x104);
        ehsys_ED1410E0(G.x100);
    }
    return 1;
}

/* func_00002950 — exported (NID 0xF498562A), proposed TutList_GetResult:
 * -1 if the list was cancelled, else the picked entry id.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00002950(void) {
    if (G.bits.flag) {
        return -1;
    }
    return G.x06;
}

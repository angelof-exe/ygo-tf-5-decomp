/*
 * rel_conv_machine.prx — reconstructed code (matching decompilation)
 * Card Convert Machine ("labo/conv_machine.ehp"): the screen where a card is
 * picked through the select_card module, shown on a rotating 3D model, and
 * explained by a scrolling text list built from the OCG pack tables.
 *
 * Compiler:     Metrowerks CodeWarrior for PSP — MWCC 1.3 SP7 (mwccpsp_3.0.1_219)
 * Flags:        -O4,s -sdatathreshold 0
 * Verification: scripts/mwcc_build.sh + scripts/mwcc_diff.py, and
 *               `make MODULE=rel_conv_machine SRC=1` for the whole module.
 *               Functions tagged "MATCH 100%" compile byte-identical (checked
 *               per function AND after linking, where every instruction of
 *               the function equals the shipped PRX).
 *
 * See src/rel_movie_viewer.c for the general conventions and MWCC levers.
 *
 * MODULE LAYOUT
 *  - exports (libconv_machine_rel, nids/modules.csv): func_00000810 init,
 *    func_000008EC per-frame update (returns 1 to leave), func_00000A10 draw.
 *  - main state machine D_0007D040.state: 0 load (func_00000114), 1 browse
 *    (func_00000A60), 2 card select (func_00000CE8, select_card_* imports),
 *    3 model zoom-in (func_00000E68), 4 convert + text list (func_0000111C),
 *    5 teardown (func_000014A8), 6 = done.
 *  - resource tables in .data are qsorted by id at init (func_00000078) and
 *    bsearched (func_00000010 / func_00000790); paths are sprintf formats
 *    whose %c is a language letter from "jegfis" (D_000088F0).
 *  - 0x47D460 is a scrolling text list widget (func_000025D4..func_00003594)
 *    that wraps a wide-char (u16) string into rows.
 *
 * GLOBALS. Everything past the end of .data (0x9030) is bss that the shipped
 * PRX addresses with BAKED absolute immediates (no relocation entry) when an
 * instruction only loads/stores a field, and with a normal HI16/LO16
 * relocation when the code takes an object's ADDRESS into a register. Both
 * are written here as members of one struct per area (Work at 0x7D040,
 * G47D410/List at 0x47D410/0x47D460); the linker resolves the addends.
 * scripts/mwcc_diff.py does not yet understand a relocation ADDEND (it
 * compares the candidate's symbol address, not symbol+addend, against the
 * target's baked immediate), so for this file it reports false "baked-const"
 * and "RELOC MISMATCH" diffs on struct members; see the module doc for the
 * one-line fix. The link-level check (`make ... SRC=1`) is unaffected.
 *
 * MWCC LEVERS CONFIRMED HERE (each was the last blocker on a function):
 *  1. Address-of-global copy propagation is LOCAL to a basic block. A
 *     pointer `T *p = &G;` whose uses sit in the same block as its
 *     definition is folded back into a re-materialised lui/addiu per use;
 *     if a branch separates them, p stays in a register. Three ways the
 *     target shows this:
 *       - `&G.member` with a NONZERO offset is never folded
 *         (func_00000A60, func_000018D8, func_00001590, func_00002480 ...);
 *       - a `?:` creates the block boundary and is if-converted later into
 *         sltu/branch-free code, so it costs nothing (func_00002E1C:
 *         `r->r2C = a1 ? 1 : 0;` is what keeps &D_0047D460 in $s0);
 *       - a definition before a `switch`/`if` (func_00000E68, func_0000111C).
 *  2. Stack slots: the FIRST declared local gets the HIGHEST address
 *     (func_00000E68's four matrices, func_00003278's four out-ints).
 *     Declaration order also picks saved registers (func_000003E4,
 *     func_00002B14, func_00003594: swap two declarations to swap $s regs).
 *  3. A `static` callee that does not write an argument register lets the
 *     caller keep a value there across the call: func_00000520 reuses $a1
 *     (the table base) after `jal func_00000010`. Only func_00000010 is
 *     static; making the others static breaks func_000030C4.
 *  4. MWCC never hoists a load above an earlier store through a pointer
 *     into the same object, so a field-copy init must be written in the
 *     order the target LOADS its sources (func_00002FB0).
 *  5. A loop that is fully known (0 < 1, 0 < 2) is peeled/rotated by MWCC
 *     when written as for/while; the target kept the plain "b test" shape,
 *     which only a goto-built loop reproduces (func_00000338).
 *  6. `char *` arithmetic `p + 0x160000` is emitted as a macro addiu with
 *     $at, `int` arithmetic as lui into a temp (func_00001F98).
 *  7. An `int` prototype parameter fed from a `short` global changes the
 *     argument evaluation order versus a `short` parameter (func_00001F98).
 *  8. `if (a && b)` / nested-if / `if (x == 0) return 0; else return ...`
 *     each give a different branch layout; see func_00002EC4/func_00002F14.
 *  9. Local aggregates with initializers become anonymous .data objects;
 *     this file must not emit any .data (the shipped .data blob is linked
 *     verbatim), so such copies are written as copies from the D_ symbol of
 *     that anonymous object (func_00000520 "jabgfisb", func_00002154,
 *     func_00002370), which reproduces MWCC's copy code exactly.
 *
 * Japanese text: none. Every string in .data is ASCII (file names).
 */

/* ---- imports -------------------------------------------------------- */
extern void ehsys_memset(void *, int, int);
extern void ehsys_qsort(void *, int, int, int (*)(const void *, const void *));
extern int ehsys_sprintf(char *, const char *, ...);
extern int ehsys_get_language(void);
extern void ehsys_sceKernelChangeCurrentThreadAttr(int, int);
extern int ehsys_291D6262(void *, int, int);
extern int ehsys_2717C28C(int);
extern int ehsys_8C5B148E(int, int, int);
extern void ehsys_96BC2DC8(int);
extern int ehsys_4E8B47D4(void);
extern void ehsys_60BA0722(int);
extern void ehsys_20E340D9(int, int);
extern int ehsys_D2A768F4(int, char *);
extern int ehsys_41AABF28(int, int);
extern void ehsys_8288EEBC(int, char *, int, int, int (*)(int, int), int);
extern int ehsys_C6C15111(int, char *, int);
extern void ehsys_4B0DABFA(int, int);
extern int ehsys_1856E536(void *, int);
extern void ehsys_6E42DDCD(void);
extern void ehsys_C9D9E989(int);
extern void ehsys_4AA58320(int);
extern void ehsys_33ED567B(int);
extern void ehsys_D853F15C(int);
extern void ehsys_D470D0B2(int, int, int);
extern void ehsys_BE3756D0(int);
extern void ehsys_13A4081A(int);
extern short ehsys_6533F67D(void);
extern unsigned short ehsys_6FE21799(void);
extern short ehsys_F352F222(void);
extern int ehsys_450CD6C5(void);
extern void ehsys_2AE3C0AD(int);
extern void ehsys_65DA0419(int);
extern unsigned char *ehsys_3BC985D1(int);
extern void ehsys_39ADDA9F(void *);
extern void ehsys_5998BFD7(int, int, int);
extern void ehsys_6367AE39(int);
extern int select_card_5B395307(void);
extern void select_card_BF2B46D7(int, void *, void *, void *);
extern unsigned char *ehsys_53636C1D(void);
extern void ehsys_13A603EB(void *);
extern void ehsys_D697947E(void *, void *, void *);
extern void ehsys_8B0A8B96(float, void *, void *);
extern int ehsys_E58C0FDC(int, int);
extern int ehsys_AD0E1E03(short);
extern void ehsys_E50B9594(void *, int, int);
extern int *ehsys_F5E3080C(void);
extern void ehsys_50D6F57B(unsigned short, int, int);
extern void ehsys_D1A604C7(int);
extern void ehsys_C4FA973E(unsigned short, int);
extern int ehsys_B779C0DF(void);
extern int ehsys_06A1D1D8(void);
extern void ehsys_0ED40BD9(unsigned short);
extern void ehsys_99E9314A(unsigned short);
extern void ehsys_3191B26A(unsigned short);
extern void ehsys_5DBE93DB(int);
extern int ehsys_101103B3(int);
extern void ehsys_D767C29F(int);
extern void ehsys_1078C73B(int);
extern void ehsys_025FE238(char *, int);
extern void ehsys_E1139F1A(void *);
extern void ehsys_2012884C(void *);
extern void ehsys_B8AD96EA(int);
extern void ehsys_942B03D0(int, int);
extern void ehsys_60B55A50(int);
extern int ehsys_A4AFF8E6(void);
extern void ehsys_BC8E65D7(int, int, int, int, int);
extern int ehsys_DFCA450B(int, int);
extern int ehsys_459B889D(int);
extern void ehsys_EBD1986B(int (*)(int), void (*)(int));
extern void ehsys_1EC5342B(int);
extern void select_card_CFA64443(void);
extern int ehsys_1D98951B(int, int);
extern int ehsys_42110BF9(int, int);
extern int ehsys_399AC68A(int, int);
extern int ehsys_C62F9EAC(int, int);
extern void ehsys_C1F72EF5(void);
extern int ehsys_F462EE6E(int, int);
extern int ehsys_09794DF0(int, int, int);
extern int ehsys_E6E4401F(int, int, int);
extern int ehsys_5CBB9E43(int, int, int, int, int, int);
extern int ehsys_4175E3DB(int, int);
extern int ehsys_9EA6989A(int, int);
extern int ehsys_CABEA834(int, int);
extern int ehsys_8EA6B1E7(int, int, int, int, int);
extern int ehsys_FDD168CA(int, int, int, int, int, int, int, int);
extern int ehsys_28633E5D(int);
extern void ehsys_CE8B1945(int, int);
extern int ehsys_85F89FE8(int);
extern int ehsys_7187E0F0(int, int, void *);
extern int ehsys_89812D8A(int);
extern void ehsys_4F22C9AA(int);
extern void ehsys_9EFDC9B9(void);
extern void ehsys_F2BD0694(void);
extern void ehsys_8CA1052F(float, float, float, float);
extern void ehsys_9490E557(int, int, int (*)(int *, int, int));
extern void ehsys_EF9740B0(void *);
extern void ehsys_9F25AE2F(int, int);
extern void ehsys_BB22B13B(int, void *);
extern void ehsys_F8CDBCAB(void);
extern void ehsys_EF9B5D06(int *, void *);
extern void ehsys_878EE1D3(int *);
extern int ehsys_3765ED01(int, int);
extern void ehsys_1CC5878C(void *);
extern void ehsys_BE792119(void *);
extern int ehsys_B4F2178B(int);
extern int ehsys_5DE4869F(int, int, int, int, int, int);
extern int ehsys_AD222020(int, int, int);
extern int ehsys_FD84C4D3(int, int, int, int);
extern int ehsys_B9064026(int);
extern int ehsys_572A577B(int, int, int, int, int);
extern char *ehsys_E47E40E4(void);
extern int ehsys_3DE5C8DC(int, int, int, int, int, void *);
extern int ehsys_F17C5C28(int, int, int);
extern int ehsys_7B1755CF(int, float, float);
extern int ehsys_3E24ACE1(int, float, float);
extern int ehsys_B56890DA(int, int, int);
extern int ehsys_0939B6BE(int, int, int, int, int);
extern int ehsys_758DE09D(int, int, int);
extern int ehsys_20560342(int, int, int, int, int);
extern int ehsys_C8DF61CD(int, int, int, int);
extern int ehsys_348F1A33(int, int);
extern int ehsys_A36635DA(int, int);
extern int ehsys_5C1AEE8E(int, int);
extern int ehsys_23E69878(int, int);
extern int ehsys_917CC4D5(int, int, float, float);
extern void ehsys_8EA5CCE1(float, float, float, float, float, float);
extern void ehsys_42C464D5(float *);
extern void ehsys_E2A5CE8D(float *, float *, float *);
extern void ehsys_D1C6F209(void);
extern int ehsys_1C2A8EDD(int, int, void *);
extern int ehsys_D6E0AA6A(int, int);
extern int ehsys_D71FA940(int);
extern int ehsys_94982653(int, int);
extern int ehsys_442C88C6(void);
extern void ehsys_006C8EE8(float, void *, void *, void *);
extern int ehsys_7002278E(void);
extern int ehsys_get_button_code(int);
extern unsigned int *ehsys_B2B9FEFA(void);
extern void ehsys_C792FA38(int);
extern void ehsys_14BFFB01(int *, void *);
extern int ehsys_2FD4863E(void);
extern void ehsys_6B9DF7E8(void *, void *, int *, int *, int *, int *);
extern int ehsys_3BB2BAC6(int, int, int, int, unsigned short *, int, int);
extern void ehsys_30FECAEF(void *, int);
extern void ehsys_50B8DF28(void *, void *);
extern void ehsys_5B08FB0D(void);
extern int ehsys_66F8E113(unsigned short *);
extern unsigned int ehsys_82E739A4(unsigned short *);
extern unsigned short ehsys_0D961646(void);
extern void ehsys_AB962AE7(int);
extern void ehsys_06380DFA(unsigned short);

/* ---- module data ---------------------------------------------------- */

/* resource table entry: id, loaded handle, sprintf format for the path */
typedef struct {
    unsigned short id;
    unsigned short pad;
    int handle;
    char *fmt;
} Res12;

typedef struct {
    unsigned short id;
    unsigned short pad;
    int handle;
    char *fmt;
    int unkC;
} Res16;

typedef struct {
    unsigned short id;
    unsigned short pad;
    int handle;
    int unk8;
    int unkC;
    int unk10;
} Res20;

typedef struct {
    unsigned char flag : 1;
    unsigned char step : 7;
} LoadState;

typedef struct {
    char c[8];
} Chars8;

typedef struct {
    float m[16];
} Mtx;

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    float x, y, z, w;
} Vec4;

/* one-byte bitfield groups (see the MWCC bitfield note in docs/06) */
typedef struct {
    unsigned char b0 : 1;
    unsigned char b1 : 1;
    unsigned char b2 : 1;
    unsigned char b3 : 1;
    unsigned char b4 : 1;
    unsigned char b5 : 1;
    unsigned char b6 : 1;
    unsigned char b7 : 1;
} Bits8;

typedef struct {
    unsigned char lo : 4;
    unsigned char hi : 4;
} Nib8;

/* engine message-window descriptor (0x124 bytes; same layout as the one
 * rel_movie_viewer's func_00000294 builds). Field names are offsets. */
typedef struct {
    int r00;            /* +0x4C */
    int r04;
    signed char r08;    /* +0x54 */
    signed char r09;
    short pad0A;
    short r0C;          /* +0x58 */
    short r0E;
    int r10;
    int r14;            /* +0x60 */
    int r18;
    int r1C;
    short r20;          /* +0x6C */
    short r22;
    int r24;            /* +0x70 */
    int r28;
    int r2C;            /* +0x78 */
    int r30;
    int r34;
    int r38;            /* +0x84: text */
    int r3C;
    int r40;
    int r44;
    short r48;          /* +0x94 */
    short r4A;
    short r4C;
    short r4E;
    unsigned char r50;  /* +0x9C */
    char pad51[3];
    int r54;            /* +0xA0 */
    int r58;
    signed char r5C;    /* +0xA8 */
    char pad5D[3];
    int r60;            /* +0xAC */
    int r64;
    int r68;            /* +0xB4 */
    char pad6C[0x10];
} MsgRec;

typedef struct {
    char data[0x4C];
} MsgHdr;

typedef struct {
    MsgHdr hdr;         /* +0x00 */
    MsgRec rec;         /* +0x4C */
    int heap;           /* +0xC8 */
    Bits8 fCC;          /* +0xCC */
    unsigned char fCD;
    Nib8 fCE;           /* +0xCE */
    char padCF[8];
    Nib8 fD7;           /* +0xD7 */
    unsigned char fD8;  /* +0xD8 */
    char padD9[0x14];
    unsigned char state; /* +0xED */
    char padEE[0x12];
    void (*draw)(int, int, int, int); /* +0x100 */
    char pad104[0x20];
} MsgWin;

typedef struct {
    int obj;           /* +0x00 */
    char pad4[0xC];
    Mtx mtx;           /* +0x10 */
    unsigned int cnt;  /* +0x50 */
    int tex[9];        /* +0x54 */
} Obj380;

extern unsigned char D_000088F0[];      /* "jegfis": language letters */
extern Res12 D_000089C8[2];    /* archives: deck/all_%c.ehp, labo/conv_machine.ehp */
extern Res12 D_00008C80[0x29]; /* files inside the archives, id 0x80xx */
extern Res16 D_00008F28[0xC];  /* files, id 0x10xx */
extern Res20 D_00008FE8[1];
extern char D_00008FFC[];      /* "jabgfisb" — a local char[8] initializer in the original */
extern LoadState D_00009030;
extern char D_00009034[];
extern float D_00008948;
extern float D_0000894C; /* pi */
extern float D_00008950; /* 1.8f */
extern float D_00008978;
extern float D_0000897C;
extern float D_00008960;
extern float D_00008964;
extern float D_00008968;
extern float D_00008970;
extern float D_00008974;
extern unsigned int D_00008980[3]; /* list scroll speeds (10, 20, 480) */
extern int D_00008988;
extern short D_0000898C;
extern short D_0000898E;
extern short D_00008990;
extern short D_00008992;
extern int D_00008994;
extern int D_00008998;
extern volatile float D_0000899C;
extern float D_00009004;
extern float D_00009008;
extern float D_0000900C;
extern float D_00009010;
extern float D_00009014;
extern float D_00009018;
extern float D_0000901C;

/* main work area at 0x7D040 (0x3D0 bytes, cleared by func_00000810).
 * Direct field accesses compile to a baked absolute address in the shipped
 * PRX (no relocation entry); taking a member's ADDRESS keeps a relocation.
 * Writing those as `&D_0007D040.member` (nonzero offset) is also what stops
 * MWCC from constant-propagating the address into every use (see header). */
typedef struct {
    unsigned char state;  /* +0x000 main state (func_000008EC) */
    unsigned char sub;    /* +0x001 sub state */
    unsigned char next;   /* +0x002 state to enter after a fade */
    unsigned char pad003;
    short card;           /* +0x004 */
    short pad006;
    int x008;             /* +0x008 */
    short x00C;           /* +0x00C */
    short pad00E;
    int x010;             /* +0x010 */
    int x014;             /* +0x014 */
    unsigned short x018;  /* +0x018 */
    unsigned short pad01A;
    MsgWin win;           /* +0x01C */
    char x140[0x200];     /* +0x140 */
    Obj380 model;         /* +0x340 */
    char pad3B8[0x8];
    int x3C0;             /* +0x3C0 */
    int heap;             /* +0x3C4 heap handle */
    int x3C8;             /* +0x3C8 */
    int pad3CC;
} Work;

extern Work D_0007D040;
extern MsgWin D_0007D05C; /* == D_0007D040.win */
extern MsgRec D_0007D0A8; /* == D_0007D040.win.rec */
typedef struct {
    float x, y;
} Vec2;

/* 0x47D410 (0x50 bytes, cleared by func_00001854) */
typedef struct {
    Vec4 light;           /* +0x00 */
    char x10[0x3C];       /* +0x10 */
    int pad4C;
} G47D410;

typedef struct {
    int x0;
    int x4;
    int val;              /* +0x08 */
    int xC;
    int x10;
    int x14;
} ListRow;


/* 0x47D460: the scrolling list (0x184 bytes, cleared by func_000025D4) */
typedef struct {
    int mode;             /* +0x000 0..3 */
    float t;              /* +0x004 */
    MsgHdr hdr;           /* +0x008 */
    MsgRec rec;           /* +0x054 */
    int xD0;              /* +0x0D0 */
    int xD4;
    int xD8;
    int xDC;
    int xE0;              /* +0x0E0 */
    int xE4;
    unsigned short *text; /* +0x0E8 */
    int xEC;              /* +0x0EC */
    int xF0;
    int xF4;
    ListRow rows[3];      /* +0x0F8 */
    int x140;             /* +0x140 */
    int x144;
    int x148;
    int x14C;
    int x150;             /* +0x150 */
    int x154;
    int x158;
    int x15C;
    int x160;             /* +0x160 */
    int x164;
    int x168;
    Vec2 v16C;            /* +0x16C */
    Vec2 v174;            /* +0x174 */
    Vec2 v17C;            /* +0x17C */
} List;

extern G47D410 D_0047D410;
extern MsgRec D_0047D4B4; /* == D_0047D460.rec */
extern List D_0047D460;
extern char D_0007D410[];

/* ---- forward declarations ------------------------------------------- */
int func_00000000(const void *a, const void *b);
static int func_00000010(unsigned short key, Res12 *tbl, int n);
void func_00000078(void);
int func_00000114(void);
void func_00000330(void);
void func_00000338(void);
int func_000003E4(unsigned short id);
int func_00000520(unsigned short id);
int func_000006F0(int arg0, int arg1);
int func_00000780(const void *a, const void *b);
int func_00000790(unsigned short key, unsigned short *tbl, int n, int size);
int func_00000808(void);
void func_00000810(void);
int func_000008EC(void);
void func_00000A10(void);
void func_00000A18(void);
void func_00000A20(void);
void func_00000A60(void);
void func_00000CE8(void);
void func_00000E68(void);
Mtx *func_00001094(Mtx *dst, Mtx *src);
void func_0000111C(void);
int func_000014A8(void);
void func_00001590(unsigned char mode);
void func_000017DC(int a0, int a1, int a2, int a3);
void func_00001840(int x, int y, int w, int text);
void func_00001854(void);
int func_000018B8(int size);
void func_000018C8(int ptr);
void func_000018D8(void);
void func_00001EE8(void);
int func_00001F98(int *pp, int a1, int mode);
int func_00002154(int p);
int func_00002370(int p);
int func_00002480(int p);
int func_000027E0(void);
void func_00002B14(int *pp);
int func_00002DC8(int x, int y, int w, int h, unsigned short *text);
int func_00002DE4(void);
MsgRec *func_00002DFC(void);
int func_00002EC4(void);
int func_00002F14(void);
int func_00002F7C(void);
void func_00003444(int *pp, int x, int y, int a3, int t0, int t1, int t2, int t3, int blink);
unsigned short func_000034FC(void);
void func_00003570(unsigned short sel);
Vec4 *func_00002458(Vec4 *dst, Vec4 *src);
Vec2 *func_000027C8(Vec2 *dst, Vec2 *src);
void func_00002E08(int);
void func_00002FB0(void);
void func_000030C4(void);
void func_00003100(void);
void func_00003278(void);
void func_00003358(void);
void func_00003594(void);
void func_000024F4(void);
void func_000025D4(int a0, int a1, int a2, int a3, int a4, int a5);
void func_00002DF0(int a0);
void func_00002EA4(void);
void func_00002EB4(void);
void func_00002E1C(void *a0, int a1);
void func_00002678(int a0, int a1, int a2, int a3, int a4);

/* func_00000000 — qsort comparator on a leading u16 id (used for g_files_8000).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000000(const void *a, const void *b) {
    return *(unsigned short *) a - *(unsigned short *) b;
}

/* func_00000010 — binary search of a 12-byte {id, handle, name} table by id; -1 if absent.
 * `static` is load-bearing: it lets func_00000520 keep the table base in $a1
 * across the call (lever 3 in the header).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
static int func_00000010(unsigned short key, Res12 *tbl, int n) {
    int lo = 0;
    int hi = n - 1;
    int mid;

    while (lo <= hi) {
        mid = (lo + hi) / 2;
        if (key == tbl[mid].id) {
            return mid;
        }
        if (tbl[mid].id < key) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return -1;
}

/* func_00000078 — reset the loader state and qsort the three resource tables by id.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000078(void) {
    ehsys_memset(&D_00009030, 0, 2);
    D_00009030.flag = 1;
    ehsys_qsort(D_00008C80, 0x29, 0xC, func_00000000);
    ehsys_qsort(D_00008F28, 0xC, 0x10, func_00000780);
    ehsys_qsort(D_00008FE8, 1, 0x14, func_00000780);
}

/* func_00000114 — async loader, one step per frame. step 0 sets up the 0x74000-byte
 * buffer and opens archive 2, 1 opens archive 1, 2 waits for it, 3/4 finish,
 * 100 = alternative path when a card is already selected. Returns 1 when done.
 * Written as `if (flag) { switch } return 0;` — an early `return 0` for the
 * flag test inverts the first branch.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000114(void) {
    if (D_00009030.flag) {
        switch (D_00009030.step) {
        case 0:
            D_0007D040.x3C8 = ehsys_291D6262(D_00009034, 0x74000, 0);
            func_000003E4(2);
            break;
        case 1:
            func_000003E4(1);
            break;
        case 2:
            if (ehsys_2717C28C(D_0007D040.x3C8)) {
                if (D_0007D040.card >= 0) {
                    D_00009030.step = 100;
                } else {
                    D_00009030.step++;
                }
            }
            break;
        case 3:
            ehsys_96BC2DC8(0x1F);
            D_00009030.step++;
            break;
        case 4:
            if (ehsys_4E8B47D4()) {
                D_0007D040.x010 = 0x1F;
                D_00009030.step++;
            }
            break;
        case 100:
            if (ehsys_8C5B148E(D_0007D040.x3C8, D_0007D040.card, 0)) {
                D_00009030.step = 3;
            }
            break;
        default:
            D_00009030.step = 0;
            return 1;
        }
    }
    return 0;
}

/* func_00000330 — tail call: release everything (func_00000338).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000330(void) {
    func_00000338();
}

/* func_00000338 — release every loaded file and archive. Both loops are goto-built:
 * MWCC peels/rotates a `for (i = 0; i < 1; i++)` (and `< 2`) whose trip count
 * it can prove, while the target keeps the plain "b test / body / test" shape
 * with the entry test (lever 5 in the header).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000338(void) {
    unsigned int i;
    unsigned int j;
    Res20 *r;
    Res12 *a;

    i = 0;
    goto test_i;
loop_i:
    r = &D_00008FE8[i];
    if (r->handle) {
        ehsys_60BA0722(r->handle);
        r->handle = 0;
    }
    i++;
test_i:
    if (i < 1) {
        goto loop_i;
    }

    j = 0;
    goto test_j;
loop_j:
    a = &D_000089C8[j];
    if (a->handle) {
        ehsys_20E340D9(D_0007D040.heap, a->handle);
        a->handle = 0;
    }
    j++;
test_j:
    if (j < 2) {
        goto loop_j;
    }
}

/* func_000003E4 — get archive `id` (1 = deck/all_%c.ehp, 2 = labo/conv_machine.ehp),
 * starting an async load (callback func_000006F0) if it is not resident and the
 * loader is idle. Declaring `idx` before `i` is what puts them in $a2/$a1.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000003E4(unsigned short id) {
    char path[0x100];
    int idx = -1;
    unsigned int i;
    Res12 *a;
    int size;
    int h;

    for (i = 0; i < 2; i++) {
        if (id == D_000089C8[i].id) {
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        return 0;
    }
    a = &D_000089C8[idx];
    if (a->handle) {
        return a->handle;
    }
    if (D_00009030.flag) {
        ehsys_sprintf(path, a->fmt, D_000088F0[ehsys_get_language()]);
        D_00009030.flag = 0;
        size = ehsys_D2A768F4(3, path);
        a->handle = h = ehsys_41AABF28(D_0007D040.heap, size);
        ehsys_8288EEBC(3, path, h, size, func_000006F0, h);
    }
    return 0;
}

/* func_00000520 — get file `id`, loading it from its archive on demand: ids with
 * bit 15 come from g_files_8000 (archive 1), ids with bit 12 from g_files_1000
 * (archive 2). ocg_pack{name,list}_%c.bin (0x8036/0x8037) always use letter
 * "jabgfisb"[2]; the copy of that local char[8] is written as a struct copy
 * from its anonymous .data object (lever 9).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000520(unsigned short id) {
    int idx;
    int arc;

    if (id & 0x8000) {
        Res12 *r;
        idx = func_00000010(id, D_00008C80, 0x29);
        if (idx < 0) {
            return 0;
        }
        r = &D_00008C80[idx];
        if (r->handle) {
            return r->handle;
        }
        arc = func_000003E4(1);
        if (!arc) {
            return 0;
        }
        {
            char path[0x100];
            if ((unsigned int) (id - 0x8000 - 0x36) < 2) {
                Chars8 langs = *(Chars8 *) D_00008FFC;
                ehsys_sprintf(path, r->fmt, langs.c[2]);
            } else {
                ehsys_sprintf(path, r->fmt, D_000088F0[ehsys_get_language()]);
            }
            return r->handle = ehsys_C6C15111(arc, path, 0);
        }
    }
    if (id & 0x1000) {
        Res16 *r;
        char path[0x100];
        idx = func_00000790(id, (unsigned short *) D_00008F28, 0xC, 0x10);
        if (idx < 0) {
            return 0;
        }
        r = &D_00008F28[idx];
        if (r->handle) {
            return r->handle;
        }
        arc = func_000003E4(2);
        if (!arc) {
            return 0;
        }
        ehsys_sprintf(path, r->fmt, D_000088F0[ehsys_get_language()]);
        return r->handle = ehsys_C6C15111(arc, path, 0);
    }
    return 0;
}

/* func_000006F0 — archive load completion callback: on success hand the data to
 * ehsys_4B0DABFA and advance the loader step; always re-arm the loader.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000006F0(int arg0, int arg1) {
    int ok = arg1 > 0;

    if (ok) {
        if (arg0) {
            ehsys_4B0DABFA(0, arg0);
        }
        D_00009030.step++;
    }
    D_00009030.flag = 1;
    return ok;
}

/* func_00000780 — qsort comparator on a leading u16 id.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000780(const void *a, const void *b) {
    return *(unsigned short *) a - *(unsigned short *) b;
}

/* func_00000790 — binary search of a table of `size`-byte records whose first field is a
 * u16 id.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000790(unsigned short key, unsigned short *tbl, int n, int size) {
    int step = size / 2;
    int lo = 0;
    int hi = n - 1;
    int mid;

    while (lo <= hi) {
        mid = (lo + hi) / 2;
        if (key == tbl[mid * step]) {
            return mid;
        }
        if (tbl[mid * step] < key) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return -1;
}

/* func_00000808 — returns 0.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000808(void) {
    return 0;
}

/* func_00000810 — exported module init: clear the work area, create the 4 MB module
 * heap, set up allocators, sort tables, start the text list, pick the font for
 * Japanese vs. other languages.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000810(void) {
    ehsys_memset(&D_0007D040, 0, 0x3D0);
    D_0007D040.heap = ehsys_1856E536(D_0007D410, 0x400000);
    ehsys_sceKernelChangeCurrentThreadAttr(0, 0x4000);
    func_00001854();
    func_00000078();
    ehsys_6E42DDCD();
    func_00000A20();
    func_000025D4(D_0007D040.heap, 0x27, 0x190, 0xE, 1, 3);
    ehsys_C9D9E989(1);
    ehsys_4AA58320(1);
    if (ehsys_get_language() == 0) {
        ehsys_33ED567B(1);
        ehsys_D853F15C(0);
    } else {
        ehsys_33ED567B(0);
        ehsys_D853F15C(1);
    }
    D_0007D040.state = 0;
}

/* func_000008EC — exported per-frame update; dispatches on g_work.state, returns 1 when
 * the screen is finished.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000008EC(void) {
    switch (D_0007D040.state) {
    case 0:
        if (func_00000114()) {
            func_00002DF0(func_00000520(0x1002));
            D_0007D040.x3C0 = func_00000520(0x1010);
            func_00001EE8();
            ehsys_D470D0B2(1, 0xFF000000, 0x10);
            ehsys_BE3756D0(0x13);
            ehsys_13A4081A(0);
            D_0007D040.state = 1;
            D_0007D040.sub = 0;
        }
        break;
    case 1:
        func_00000A60();
        break;
    case 2:
        func_00000CE8();
        break;
    case 3:
        func_00000E68();
        break;
    case 4:
        func_0000111C();
        break;
    case 5:
        if (!func_000014A8()) {
            D_0007D040.state = 6;
        }
        break;
    default:
        func_00000A18();
        return 1;
    }
    return 0;
}

/* func_00000A10 — exported draw entry: tail call to func_000018D8.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000A10(void) {
    func_000018D8();
}

/* func_00000A18 — tail call to ehsys_6E42DDCD (state 6: leave).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000A18(void) {
    ehsys_6E42DDCD();
}

/* func_00000A20 — read the current card / counters from the engine.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000A20(void) {
    D_0007D040.card = ehsys_6533F67D();
    D_0007D040.x008 = ehsys_6FE21799();
    D_0007D040.x00C = ehsys_F352F222();
}

/* func_00000A60 — state 1 (browse). Fixed by two findings: the window pointer must be
 * `&D_0007D040.win` (a member address is not constant-propagated, lever 1),
 * and ehsys_3BC985D1 takes the card id (the target keeps it in $a0).
 * ehsys_3BC985D1(card)[1] & 0x7F is compared with 9 (likely the level).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000A60(void) {
    MsgWin *w;

    switch (D_0007D040.sub) {
    case 0:
        if (ehsys_450CD6C5()) {
            D_0007D040.sub = 1;
            ehsys_2AE3C0AD(0x9558);
        }
        break;
    case 1:
        D_0007D040.x00C += 10;
        if (D_0007D040.x00C >= D_0007D040.x008) {
            D_0007D040.x00C = D_0007D040.x008;
            ehsys_65DA0419(0x9558);
            if (D_0007D040.card > 0 && (ehsys_3BC985D1(D_0007D040.card)[1] & 0x7F) >= 9) {
                D_0007D040.card = -1;
                func_00001590(2);
                D_0007D040.sub = 4;
            } else if (D_0007D040.card >= 0 && D_0007D040.x008 >= 500) {
                D_0007D040.model.cnt = 0;
                D_0007D040.x008 = 0;
                D_0007D040.state = 4;
                D_0007D040.sub = 0;
            } else if (D_0007D040.card < 0) {
                func_00001590(0);
                D_0007D040.sub = 2;
            } else {
                func_00001590(1);
                D_0007D040.sub = 3;
            }
        }
        break;
    case 2:
    case 3:
    case 4:
        w = &D_0007D040.win;
        switch (w->state) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            ehsys_39ADDA9F(w);
            break;
        case 6:
            if (D_0007D040.sub == 4) {
                func_00001590(0);
                D_0007D040.sub = 2;
            } else {
                ehsys_5998BFD7(1, 0xFF000000, 0x10);
                if (w->fD7.lo == 2) {
                    D_0007D040.next = 2;
                    D_0007D040.sub = 5;
                } else {
                    D_0007D040.next = 5;
                    D_0007D040.sub = 5;
                    ehsys_6367AE39(0x190);
                }
            }
            break;
        }
        break;
    case 5:
        if (ehsys_450CD6C5()) {
            D_0007D040.state = D_0007D040.next;
            D_0007D040.sub = 0;
        }
        break;
    }
    if (D_0007D040.card >= 0) {
        func_000024F4();
    }
}

/* func_00000CE8 — state 2: run the select_card module and act on the picked card.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000CE8(void) {
    int p[4];

    switch (D_0007D040.sub) {
    case 0:
        p[0] = D_0007D040.heap;
        p[1] = D_0007D040.x3C8;
        p[2] = func_000003E4(1);
        p[3] = func_000003E4(2);
        D_0007D040.x014 = 0;
        D_0007D040.x018 = 0xFFFF;
        select_card_BF2B46D7(1, p, &D_0007D040.x014, &D_0007D040.x018);
        D_0007D040.sub = 1;
        break;
    case 1:
        if (select_card_5B395307() == 1) {
            ehsys_13A4081A(0);
            if ((short) D_0007D040.x018 < 0) {
                ehsys_D470D0B2(1, 0xFF000000, 0x10);
                D_0007D040.state = 1;
            } else if (D_0007D040.x008 >= 500) {
                ehsys_D470D0B2(1, 0xFF000000, 0x10);
                D_0007D040.card = D_0007D040.x018;
                D_0007D040.model.cnt = 0;
                D_0007D040.x008 = 0;
                D_0007D040.state = 4;
            } else if (D_0007D040.card != D_0007D040.x018) {
                D_0007D040.state = 3;
            } else {
                ehsys_D470D0B2(1, 0xFF000000, 0x10);
                D_0007D040.state = 1;
            }
            D_0007D040.sub = 0;
        }
        break;
    }
}

/* func_00000E68 — state 3: animate the model in (24 frames). The four matrices are
 * declared in reverse address order (lever 2).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000E68(void) {
    Mtx cam;
    Mtx trans;
    Mtx res;
    Mtx rot;
    float t;
    Obj380 *w = &D_0007D040.model;

    switch (D_0007D040.sub) {
    case 0:
        if (ehsys_8C5B148E(D_0007D040.x3C8, D_0007D040.x018, 0)) {
            D_0007D040.card = D_0007D040.x018;
            D_0007D040.sub = 1;
            ehsys_D470D0B2(1, 0xFF000000, 0x10);
        }
        break;
    case 1: {
        unsigned char *c = ehsys_53636C1D();
        cam = *(Mtx *) (c + 0x80);
        cam.m[12] = ((float *) c)[4];
        cam.m[13] = ((float *) c)[5];
        cam.m[14] = ((float *) c)[6];
        ehsys_13A603EB(&trans);
        t = (float) w->cnt / 24.0f;
        trans.m[13] = -3.0f + D_00008950 * t;
        trans.m[14] = -0.5f + -5.0f * t;
        ehsys_13A603EB(&res);
        ehsys_D697947E(&res, &res, &cam);
        ehsys_D697947E(&res, &res, &trans);
        ehsys_13A603EB(&rot);
        ehsys_8B0A8B96(D_0000894C, &rot, &rot);
        ehsys_D697947E(&res, &res, &rot);
        if (w->cnt < 24) {
            w->cnt++;
        } else {
            w->cnt = 0;
            D_0007D040.sub = 2;
        }
        func_00001094(&w->mtx, &res);
        break;
    }
    case 2:
        if (w->cnt < 24) {
            w->cnt++;
        } else {
            w->cnt = 0;
            func_00001590(1);
            D_0007D040.state = 1;
            D_0007D040.sub = 3;
        }
        break;
    }
}

/* func_00001094 — 4x4 matrix copy, one row (Vec4 struct copy) at a time.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
Mtx *func_00001094(Mtx *dst, Mtx *src) {
    ((Vec4 *) dst)[0] = ((Vec4 *) src)[0];
    ((Vec4 *) dst)[1] = ((Vec4 *) src)[1];
    ((Vec4 *) dst)[2] = ((Vec4 *) src)[2];
    ((Vec4 *) dst)[3] = ((Vec4 *) src)[3];
    return dst;
}

/* func_0000111C — state 4: conversion screen + text list. `D_0007D040.sub < 2U`
 * (unsigned) is what gives the target's sltiu.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_0000111C(void) {
    Mtx cam;
    Mtx trans;
    Mtx res;
    Mtx rot;
    float a;
    Obj380 *w = &D_0007D040.model;
    short *s1;
    int h;

    switch (D_0007D040.sub) {
    case 0:
        if (w->cnt < 24) {
            w->cnt++;
            D_0007D040.x00C = 500.0f * (1.0f - (float) w->cnt / 24.0f);
        } else {
            D_0007D040.x00C = 0;
            s1 = (short *) func_00002DFC();
            func_00002678(0x27, 0x190, 0xE, 1, 3);
            s1[0x48 / 2] = 4;
            s1[0x4C / 2] = 2;
            h = ehsys_E58C0FDC(func_00000520(0x1030), 8);
            ehsys_E50B9594(D_0007D040.x140, h, ehsys_AD0E1E03(D_0007D040.card));
            func_00002E1C(D_0007D040.x140, 0);
            D_0007D040.sub = 1;
        }
        break;
    case 1:
        if (ehsys_F5E3080C()[1] & 0x1000) {
            h = func_00000520(0x1008);
            ehsys_50D6F57B(D_0007D040.card, h, func_00000520(0x1009));
            ehsys_D1A604C7(func_00000520(0x8015));
            D_0007D040.sub = 2;
        } else {
            if (func_00002DE4()) {
                func_00002EB4();
                D_0007D040.sub = 3;
                ehsys_C4FA973E(D_0007D040.card, 1);
            }
            func_000024F4();
        }
        break;
    case 2:
        if (ehsys_B779C0DF()) {
            ehsys_D470D0B2(1, 0xFF000000, 1);
            D_0007D040.sub = 1;
        }
        break;
    case 3:
        func_00001590(0);
        D_0007D040.state = 1;
        D_0007D040.sub = 2;
        w->cnt = 0;
        D_0007D040.card = -1;
        break;
    }
    if (D_0007D040.sub < 2U) {
        unsigned char *c = ehsys_53636C1D();
        cam = *(Mtx *) (c + 0x80);
        cam.m[12] = ((float *) c)[4];
        cam.m[13] = ((float *) c)[5];
        cam.m[14] = ((float *) c)[6];
        ehsys_13A603EB(&trans);
        trans.m[13] = D_00008948;
        trans.m[14] = -0.5f;
        ehsys_13A603EB(&res);
        ehsys_D697947E(&res, &res, &cam);
        ehsys_D697947E(&res, &res, &trans);
        a = D_0000894C * ((float) w->cnt / 24.0f);
        ehsys_13A603EB(&rot);
        ehsys_8B0A8B96(a, &rot, &rot);
        ehsys_D697947E(&res, &res, &rot);
        func_00001094(&w->mtx, &res);
    }
}

/* func_000014A8 — state 5: save/teardown; returns 0 when finished. Case 0 is
 * `if (!busy) {...} break;` (decomp-permuter): an explicit `return 1` in the
 * busy path turns the later `beqz` into a `beqzl`.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000014A8(void) {
    switch (D_0007D040.sub) {
    case 0:
        if (!ehsys_06A1D1D8()) {
            ehsys_0ED40BD9(D_0007D040.card);
            ehsys_99E9314A(D_0007D040.x008);
            ehsys_3191B26A(D_0007D040.x008);
            ehsys_5DBE93DB(D_0007D040.x3C8);
            D_0007D040.sub = 1;
        }
        break;
    case 1:
        if (!ehsys_101103B3(D_0007D040.x3C8)) {
            break;
        }
        func_00000330();
        if (D_0007D040.x010 == 0x1F) {
            ehsys_D767C29F(D_0007D040.x010);
        }
        D_0007D040.sub++;
        break;
    default:
        ehsys_1078C73B(D_0007D040.heap);
        return 0;
    }
    return 1;
}

/* func_00001590 — open the message window with text `mode` (0..2), message ids
 * 10/7/9 from file 0x1030. Same 0x124-byte window type as rel_movie_viewer's
 * func_00000294 / rel_umd_replace's MsgBox.
 * NONMATCHING (147/147 words, scheduling only): every instruction is present,
 * but MWCC stores `r->r38 = text` right after the call that produces it,
 * wherever the statement is written (verified by moving it to each position
 * and by deleting the statements after it), so $v0 is free early and the
 * heap load / bitfield inserts that follow get different registers. The
 * target keeps $v0 live and stores r38 after the +0xCC bit-5 insert. Not
 * reached: `int flag` for the bitfields, `&w->rec` (costs a word: the target
 * materialises the record with its own lui/addiu), separate D_0007D05C/
 * D_0007D0A8 symbols, a statement-order permuter run (PERM_LINESWAP over the
 * 26 tail statements, ~5k iterations, best score 2100). */
void func_00001590(unsigned char mode) {
    char buf[0x200];
    MsgWin *w = &D_0007D040.win;
    MsgRec *r = &D_0007D040.win.rec;
    int msg = 0;
    int text;
    int flag = 0;

    switch (mode) {
    case 0:
        msg = 10;
        break;
    case 1:
        msg = 7;
        break;
    case 2:
        msg = 9;
        break;
    }
    ehsys_025FE238(buf, ehsys_E58C0FDC(func_00000520(0x1030), msg));
    ehsys_E1139F1A(w);
    ehsys_memset(w, 0, 0x124);
    r->r00 = 1;
    r->r14 = 1;
    r->r0E = 0x20;
    r->r20 = 0x1E0;
    r->r22 = 0x80;
    r->r24 = -1;
    r->r08 = 0;
    r->r09 = 0;
    r->r0C = 0;
    text = ehsys_E58C0FDC(func_00000520(0x1030), msg);
    w->heap = D_0007D040.heap;
    w->fCC.b0 = flag;
    w->fCC.b1 = 1;
    w->fCC.b2 = flag;
    w->fCC.b3 = flag;
    w->fCC.b4 = flag;
    w->fD8 = 3;
    w->fCC.b5 = 1;
    r->r38 = text;
    w->fCE.lo = 3;
    r->r48 = 0xC;
    r->r4A = 0xC;
    r->r4C = 0xC;
    r->r50 = 0xC;
    r->r5C = 0xC;
    r->r54 = 0xFF000000;
    r->r60 = -1;
    w->draw = func_000017DC;
    r->r3C = 0;
    w->fD7.lo = 3;
    r->r4E = 0;
    r->r58 = 0;
    r->r64 = 1;
    r->r2C = 1;
    r->r30 = 1;
    r->r68 = 2;
    if (mode == 2) {
        w->fCE.lo = 1;
    }
    ehsys_E1139F1A(w);
    ehsys_2012884C(w);
}

/* func_000017DC — message window draw callback.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000017DC(int a0, int a1, int a2, int a3) {
    ehsys_B8AD96EA(a0);
    ehsys_942B03D0(0x18, 0x18);
    ehsys_60B55A50(0xFF000000);
    func_00001840(a2, a3, 0x180, D_0007D040.win.rec.r38);
    ehsys_A4AFF8E6();
}

/* func_00001840 — draw text at (x, y) with width w, in pixels (engine wants 1/64).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001840(int x, int y, int w, int text) {
    ehsys_BC8E65D7(x << 6, y << 6, w << 6, text, -1);
}

/* func_00001854 — clear render state and register the module-heap allocators.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001854(void) {
    int h;

    ehsys_memset(&D_0047D410, 0, 0x50);
    h = ehsys_459B889D(ehsys_DFCA450B(0, 0));
    ehsys_EBD1986B(func_000018B8, func_000018C8);
    ehsys_1EC5342B(h);
}

/* func_000018B8 — allocator callback: allocate from the module heap.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000018B8(int size) {
    return ehsys_41AABF28(D_0007D040.heap, size);
}

/* func_000018C8 — allocator callback: free into the module heap.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000018C8(int ptr) {
    ehsys_20E340D9(D_0007D040.heap, ptr);
}

/* func_000018D8 — per-frame draw (display-list chain `p = f(p, ...)`).
 * NONMATCHING (387/388 words): identical except that the target keeps a dead
 * `addu $a0,$zero,$zero` at 0x1914. MWCC copied that instruction into the
 * `bnel` delay slot of the state-2 test and, unlike here, left the original
 * in place, i.e. the else block had a second predecessor when delay slots were
 * filled. if/else, early return, switch, goto, do/while(0), for(;;)/break and
 * an inline helper for the body all drop it. rel_cardalbum func_00000000 has
 * the same artefact before the same ehsys_DFCA450B call, so it is a shared
 * source idiom. The two pointers below are written as member addresses
 * (`&D_0007D040.model`, `&D_0007D040.win.rec`) so they are not
 * constant-propagated (lever 1); with them the rest of the function matches. */
void func_000018D8(void) {
    int p;
    int st2[0x34 / 4];
    int st[0x60 / 4];
    float f;
    int i;
    int j;

    if (D_0007D040.state == 2) {
        select_card_CFA64443();
    } else {
        p = ehsys_DFCA450B(0, 0);
        p = ehsys_1D98951B(p, 0xFF000000);
        p = ehsys_42110BF9(p, 0);
        p = ehsys_399AC68A(p, 0);
        p = ehsys_C62F9EAC(p, 0x17);
        ehsys_1EC5342B(p);
        if (D_0007D040.state == 0) {
            return;
        }
        if (D_0007D040.state == 4 && D_0007D040.sub == 2) {
            ehsys_C1F72EF5();
            return;
        }
        p = ehsys_DFCA450B(1, 0);
        p = ehsys_F462EE6E(p, 9);
        p = ehsys_F462EE6E(p, 4);
        p = ehsys_09794DF0(p, 0, 1);
        p = ehsys_E6E4401F(p, 0, 0);
        p = ehsys_5CBB9E43(p, 0, 2, 3, 0, 0);
        p = ehsys_4175E3DB(p, 1);
        p = ehsys_9EA6989A(p, func_00000520(0x1001));
        p = ehsys_CABEA834(p, -1);
        p = ehsys_E6E4401F(p, 0, 0);
        p = ehsys_8EA6B1E7(p, 0x40, 0x100, 0x40, 0x100);
        for (i = 0; i < 7; i++) {
            p = ehsys_FDD168CA(p, i << 6, 0, 0, i << 6, 0, 0, 0);
        }
        p = ehsys_8EA6B1E7(p, 0x20, 0x100, 0x20, 0x100);
        p = ehsys_FDD168CA(p, i << 6, 0, 0, i << 6, 0, 0, 0);
        p = ehsys_8EA6B1E7(p, 0x10, 0x40, 0x40, 0x10);
        for (j = 0; j < 4; j++) {
            p = ehsys_FDD168CA(p, 0x100 - ((j + 1) << 6), 0x100, 0, 0x1E0, j << 6, 0, 3);
            p = ehsys_FDD168CA(p, 0x200 - ((j + 1) << 6), 0x100, 0, 0x1F0, j << 6, 0, 3);
        }
        ehsys_memset(st, 0, 0x60);
        f = (float) D_0007D040.x00C / 500.0f;
        if (f > 1.0f) {
            f = 1.0f;
        }
        ehsys_CE8B1945(D_0007D040.x3C0, (float) (ehsys_28633E5D(D_0007D040.x3C0) - 1) * f);
        st[3] = 0x10000;
        st[0] = 0;
        st[5] = 0;
        st[6] = 0;
        p = ehsys_85F89FE8(p);
        p = ehsys_7187E0F0(p, D_0007D040.x3C0, st);
        p = ehsys_89812D8A(p);
        ehsys_1EC5342B(p);
        if (D_0007D040.card >= 0 && (D_0007D040.state != 3 || D_0007D040.sub != 0)) {
            p = ehsys_DFCA450B(0, 0);
            p = func_00002154(p);
            p = func_00002370(p);
            p = func_00002480(p);
            ehsys_1EC5342B(p);
            ehsys_4F22C9AA(2);
            ehsys_9EFDC9B9();
            {
            Obj380 *o = &D_0007D040.model;
            int obj = o->obj;
            ehsys_memset(st2, 0, 0x34);
            ehsys_4F22C9AA(0);
            ehsys_F2BD0694();
            ehsys_9EFDC9B9();
            ehsys_8CA1052F(D_0000897C, D_00008978, 5.0f, 400.0f);
            ehsys_9490E557(obj, 0, func_00001F98);
            ehsys_4F22C9AA(2);
            ehsys_F2BD0694();
            ehsys_9EFDC9B9();
            ehsys_EF9740B0(&o->mtx);
            st2[0] = 4;
            st2[1] = 4;
            ehsys_9F25AE2F(obj, 0);
            ehsys_BB22B13B(obj, st2);
            ehsys_4F22C9AA(2);
            ehsys_F8CDBCAB();
            ehsys_4F22C9AA(0);
            ehsys_F8CDBCAB();
            }
        }
        p = ehsys_DFCA450B(4, 0);
        p = ehsys_F462EE6E(p, 9);
        p = ehsys_F462EE6E(p, 4);
        p = ehsys_09794DF0(p, 0, 1);
        p = ehsys_E6E4401F(p, 0, 0);
        p = ehsys_5CBB9E43(p, 0, 2, 3, 0, 0);
        p = ehsys_4175E3DB(p, 1);
        func_000027E0();
        func_00002B14(&p);
        {
            MsgRec *r = &D_0007D040.win.rec;
            r->r0C = (0x1E0 - r->r20) / 2;
            r->r0E = (0x110 - r->r22) / 2;
        }
        ehsys_EF9B5D06(&p, &D_0007D040.win);
        ehsys_1EC5342B(p);
        p = ehsys_DFCA450B(5, 0);
        p = ehsys_4175E3DB(p, 1);
        ehsys_878EE1D3(&p);
        ehsys_1EC5342B(p);
    }
}

/* func_00001EE8 — load the 3D model (file 0x1031) and its textures.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001EE8(void) {
    Obj380 *o = &D_0007D040.model;
    int *t;

    o->obj = ehsys_3765ED01(func_00000520(0x1031), 0);
    t = o->tex;
    ehsys_memset(t, 0, 0x24);
    t[0] = func_00000520(0x1004);
    t[1] = func_00000520(0x1005);
    t[3] = func_00000520(0x1006);
    t[4] = func_00000520(0x1007);
    t[5] = func_00000520(0x1003);
    t[7] = D_0007D040.x3C8;
    ehsys_1CC5878C(t);
    ehsys_BE792119(&o->mtx);
}

/* func_00001F98 — model draw callback (passed to ehsys_9490E557). Three levers: the
 * `mode != 0 && mode != 1` early return, an `int` prototype for
 * ehsys_FD84C4D3's 4th parameter, and `char *` arithmetic for
 * `ehsys_E47E40E4() + 0x160000` (levers 6/7).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00001F98(int *pp, int a1, int mode) {
    int p = *pp;

    if (mode != 0 && mode != 1) {
        return 0;
    }
    p = ehsys_F462EE6E(p, 9);
    p = ehsys_F462EE6E(p, 4);
    p = ehsys_09794DF0(p, 3, 1);
    p = ehsys_5CBB9E43(p, 0, 2, 3, 0, 0);
    p = ehsys_4175E3DB(p, 1);
    p = ehsys_CABEA834(p, -1);
    p = ehsys_B4F2178B(p);
    p = ehsys_5DE4869F(p, 0x160000, -1, 0x80, 0x100, 3);
    if (mode == 0) {
        p = ehsys_AD222020(p, 0, 0);
    } else {
        p = ehsys_FD84C4D3(p, 0, 0, D_0007D040.card);
    }
    p = ehsys_B9064026(p);
    p = ehsys_B4F2178B(p);
    p = ehsys_F462EE6E(p, 9);
    p = ehsys_572A577B(p, 3, 0, 0, 0);
    p = ehsys_3DE5C8DC(p, 0, 0x80, 0x100, 0x80, ehsys_E47E40E4() + 0x160000);
    p = ehsys_09794DF0(p, 3, 0);
    p = ehsys_F17C5C28(p, 1, 1);
    p = ehsys_7B1755CF(p, 0.0f, 0.6015625f);
    p = ehsys_3E24ACE1(p, 0.84375f, -0.6015625f);
    p = ehsys_F462EE6E(p, 1);
    *pp = p;
    return 1;
}

/* func_00002154 — background / projection setup pass.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00002154(int p) {
    float v[3];
    int q;

    p = ehsys_B56890DA(p, 0x710, 0x778);
    p = ehsys_0939B6BE(p, 0x800, 0x800, 0x1E0, 0x110);
    p = ehsys_758DE09D(p, 0xFFFF, 0);
    p = ehsys_5CBB9E43(p, 0, 2, 3, 0, 0);
    p = ehsys_4175E3DB(p, 4);
    p = ehsys_20560342(p, 0, 0, 0x1E0, 0x110);
    p = ehsys_F462EE6E(p, 2);
    p = ehsys_C8DF61CD(p, 6, 0, 0xFF);
    p = ehsys_4175E3DB(p, 0);
    p = ehsys_348F1A33(p, 7);
    p = ehsys_F462EE6E(p, 1);
    p = ehsys_A36635DA(p, 1);
    p = ehsys_F462EE6E(p, 5);
    p = ehsys_4175E3DB(p, 9);
    p = ehsys_5C1AEE8E(p, 1);
    p = ehsys_F462EE6E(p, 8);
    p = ehsys_23E69878(p, 0xFF0000FF);
    p = ehsys_917CC4D5(p, 0x404040, 50.0f, 100.0f);
    q = ehsys_4175E3DB(p, 7);
    ehsys_4F22C9AA(0);
    ehsys_9EFDC9B9();
    ehsys_8EA5CCE1(-240.0f, 240.0f, -136.0f, 136.0f, -10000, 10000);
    v[0] = D_00009004;
    v[1] = D_00009008;
    v[2] = D_0000900C;
    ehsys_42C464D5(v);
    ehsys_4F22C9AA(1);
    ehsys_9EFDC9B9();
    ehsys_4F22C9AA(2);
    ehsys_9EFDC9B9();
    q = ehsys_1D98951B(q, 0);
    q = ehsys_399AC68A(q, 0);
    q = ehsys_42110BF9(q, 0);
    return ehsys_E6E4401F(q, 1, 1);
}

/* func_00002370 — view setup (eye/at/up). The zero vector is written to the stack
 * first and the eye added to it, in that order; any other order moves stores.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00002370(int p) {
    float eye[4];
    float at[4];
    float up[4];

    up[0] = D_00009010;
    up[1] = D_00009014;
    up[2] = D_00009018;
    up[3] = D_0000901C;
    ehsys_4F22C9AA(1);
    at[0] = 0.0f;
    at[1] = 0.0f;
    at[2] = 0.0f;
    eye[0] = at[0] + D_00008960;
    eye[1] = at[1] + D_00008964;
    eye[2] = at[2] + D_00008968;
    eye[3] = 1.0f;
    at[3] = 1.0f;
    ehsys_E2A5CE8D(eye, at, up);
    ehsys_D1C6F209();
    func_00002458(&D_0047D410.light, (Vec4 *) (ehsys_53636C1D() + 0x30));
    ehsys_4F22C9AA(2);
    ehsys_9EFDC9B9();
    return p;
}

/* func_00002458 — Vec4 copy.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
Vec4 *func_00002458(Vec4 *dst, Vec4 *src) {
    dst->x = src->x;
    dst->y = src->y;
    dst->z = src->z;
    dst->w = src->w;
    return dst;
}

/* func_00002480 — lighting setup pass.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00002480(int p) {
    char *g = D_0047D410.x10;

    ehsys_memset(g, 0, 0x3C);
    p = ehsys_1C2A8EDD(p, 0, g);
    p = ehsys_D6E0AA6A(p, -1);
    p = ehsys_D71FA940(p);
    return ehsys_94982653(p, 0);
}

/* func_000024F4 — idle model pose.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000024F4(void) {
    Mtx cam;
    Mtx trans;
    Mtx res;
    Mtx rot;
    unsigned char *c = ehsys_53636C1D();

    cam = *(Mtx *) (c + 0x80);
    cam.m[12] = ((float *) c)[4];
    cam.m[13] = ((float *) c)[5];
    cam.m[14] = ((float *) c)[6];
    ehsys_13A603EB(&trans);
    trans.m[13] = D_00008974;
    trans.m[14] = -5.5f;
    ehsys_13A603EB(&res);
    ehsys_D697947E(&res, &res, &cam);
    ehsys_D697947E(&res, &res, &trans);
    ehsys_13A603EB(&rot);
    ehsys_8B0A8B96(D_00008970, &rot, &rot);
    ehsys_D697947E(&res, &res, &rot);
    func_00001094(&D_0007D040.model.mtx, &res);
}

/* func_000025D4 — text list init (heap, x, width, rows, ...).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000025D4(int a0, int a1, int a2, int a3, int a4, int a5) {
    ehsys_memset(&D_0047D460, 0, 0x184);
    D_0047D460.xD0 = a0;
    D_0047D460.xD4 = a1;
    D_0047D460.xD8 = a2;
    D_0047D460.xDC = a3;
    D_0047D460.xF0 = a4;
    D_0047D460.xF4 = a5;
    func_00002FB0();
}

/* func_00002678 — text list reset.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002678(int a0, int a1, int a2, int a3, int a4) {
    MsgRec *s;
    float f;

    D_0047D460.xE0 = 0;
    D_0047D460.text = 0;
    s = &D_0047D460.rec;
    D_0047D460.xEC = ehsys_442C88C6();
    D_0047D460.xF0 = a3;
    D_0047D460.xF4 = a4;
    ehsys_memset(D_0047D460.rows, 0, 0x48);
    D_0047D460.x140 = 0;
    D_0047D460.x144 = 0;
    D_0047D460.x14C = 0;
    D_0047D460.x150 = 0;
    D_0047D460.x154 = 0;
    D_0047D460.x158 = 0;
    D_0047D460.x15C = 0;
    f = a0;
    D_0047D460.v174.x = f;
    D_0047D460.v174.y = 300.0f;
    D_0047D460.v17C.x = f;
    D_0047D460.v17C.y = 210.0f;
    func_000027C8(&D_0047D460.v16C, &D_0047D460.v174);
    D_0047D460.xE0 = 3;
    D_0047D460.x15C = 0;
    s->r0C = a0;
    s->r44 = 0;
    func_00002E08(a2);
    func_00002E1C(0, 0);
    func_00003358();
    D_0047D460.mode = 0;
}

/* func_000027C8 — Vec2 copy.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
Vec2 *func_000027C8(Vec2 *dst, Vec2 *src) {
    dst->x = src->x;
    dst->y = src->y;
    return dst;
}

/* func_000027E0 — text list update (scroll in, advance rows, scroll out); returns the
 * "finished" flag. D_0000899C is read twice per expression, so it is
 * declared volatile (otherwise MWCC loads it once).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000027E0(void) {
    List *l = &D_0047D460;
    int held = ehsys_F5E3080C()[1];
    int i;
    int rep;

    l->xEC = ehsys_442C88C6();
    switch (l->mode) {
    case 0:
        break;
    case 1:
        ehsys_006C8EE8(l->t, &l->v16C, &l->v174, &l->v17C);
        if (l->t >= 1.0f) {
            l->mode = 2;
        } else {
            l->t += D_0000899C + D_0000899C * l->t;
            if (l->t > 1.0f) {
                l->t = 1.0f;
            }
        }
        break;
    case 2:
        if (l->xEC == 2) {
            for (i = 0; i < l->xE0; i++) {
                func_00003100();
            }
        } else {
            func_00003100();
        }
        func_000030C4();
        if (l->xF0) {
            if (ehsys_7002278E() && (ehsys_F5E3080C()[0] & 0x200) &&
                (ehsys_F5E3080C()[0] & (unsigned short) ehsys_get_button_code(0)) &&
                ehsys_B2B9FEFA()[0] % 3 == 0) {
                rep = 1;
            } else {
                rep = 0;
            }
            if (l->x168 == 0 && ((held & (unsigned short) ehsys_get_button_code(0)) || rep)) {
                if (l->x14C) {
                    if (!l->x150) {
                        l->x150 = 1;
                        ehsys_2AE3C0AD(0x94CA);
                    }
                } else if (l->x154) {
                    func_00003278();
                    l->x144 = l->x158;
                    func_00003594();
                    l->x154 = 0;
                    l->x148++;
                    ehsys_2AE3C0AD(0x94CA);
                } else {
                    int step = D_00008988;
                    for (i = 0; i < l->xE0; i++) {
                        l->rows[l->x140].val += step;
                        func_00003100();
                        if (l->x14C || l->x154) {
                            break;
                        }
                    }
                }
            }
        }
        if (!l->text) {
            l->x150 = 1;
        }
        break;
    case 3:
        ehsys_006C8EE8(l->t, &l->v16C, &l->v174, &l->v17C);
        if (l->t <= 0.0f) {
            l->mode = 0;
        } else {
            l->t -= D_0000899C + D_0000899C * l->t;
            if (l->t < 0.0f) {
                l->t = 0.0f;
            }
        }
        break;
    }
    if (l->mode) {
        func_00003358();
    }
    return l->x150;
}

/* func_00002B14 — text list draw. `float gap = 3.0f;` (decomp-permuter) is what gives
 * the add.s operand order; with a literal MWCC puts the constant first.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002B14(int *pp) {
    List *l = &D_0047D460;
    MsgRec *r = &D_0047D460.rec;
    int a;
    int b;
    int c;
    int d;
    int i;
    unsigned short sel;
    int y;
    int h;
    ListRow *row;
    int x;

    if (D_0047D460.mode == 0) {
        return;
    }
    ehsys_C792FA38(r->r44);
    ehsys_14BFFB01(pp, &l->hdr);
    ehsys_C792FA38(0);
    if (l->text) {
        sel = func_000034FC();
        for (i = 0; i <= l->x140; i++) {
            row = &l->rows[i];
            y = row->x4 - 2;
            h = row->xC + 4;
            if (row->val <= 0) {
                continue;
            }
            if (y < 0x110) {
                h = (y + h < 0x110) ? h : 0x110 - y;
                *pp = ehsys_20560342(*pp, row->x0, y, row->val + 1, h);
                *pp = ehsys_F462EE6E(*pp, 2);
            }
            if (y < 0x110) {
                float f = (float) ehsys_2FD4863E() / 64.0f;
                float gap = 3.0f;
                y = -(int) ((float) l->x148 * ((float) l->xE0 * (f + gap)));
                ehsys_6B9DF7E8(&l->hdr, &l->rec, &a, &b, &c, &d);
                ehsys_B8AD96EA(*pp);
                func_00002DC8(a, b + y, c, 0x110, l->text);
                *pp = ehsys_A4AFF8E6();
            }
            *pp = ehsys_4175E3DB(*pp, 2);
        }
        func_00003570(sel);
    }
    if (l->xF0 && !func_00002F14() && !func_00002F7C() && l->x160) {
        x = r->r0C + r->r20;
        y = r->r0E + r->r22;
        *pp = ehsys_CABEA834(*pp, -1);
        *pp = ehsys_9EA6989A(*pp, l->x160);
        func_00003444(pp, x + D_00008994, y + D_00008998, 0, D_0000898C, D_0000898E, D_00008990,
                      D_00008992, (l->x164 / 10U) & 1);
    }
}

/* func_00002DC8 — draw wrapped text in a box, pixel coordinates; returns chars drawn.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00002DC8(int x, int y, int w, int h, unsigned short *text) {
    return ehsys_3BB2BAC6(x << 6, y << 6, w << 6, h << 6, text, -1, 0);
}

/* func_00002DE4 — list: finished flag.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00002DE4(void) {
    return D_0047D460.x150;
}

/* func_00002DF0 — list: set the icon file.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002DF0(int a0) {
    D_0047D460.x160 = a0;
}

/* func_00002DFC — list: window record.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
MsgRec *func_00002DFC(void) {
    return &D_0047D460.rec;
}

/* func_00002E08 — list: set the font size.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002E08(int a0) {
    MsgRec *r = &D_0047D460.rec;

    r->r50 = a0;
    r->r5C = a0;
}

/* func_00002E1C — list: set the u16 text and restart. `a1 ? 1 : 0` (not `a1 != 0`) is
 * what keeps &D_0047D460 in $s0 (lever 1).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002E1C(void *a0, int a1) {
    MsgRec *r = &D_0047D460.rec;
    List *l = &D_0047D460;

    r->r2C = a1 ? 1 : 0;
    D_0047D460.x14C = 0;
    D_0047D460.x150 = 0;
    D_0047D460.x154 = 0;
    D_0047D460.x158 = 0;
    D_0047D460.x148 = 0;
    r->r58 = a1;
    D_0047D460.text = a0;
    func_00003278();
    func_00003594();
    if (l->mode == 0) {
        func_00002EA4();
    }
}

/* func_00002EA4 — list: start scrolling in.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002EA4(void) {
    D_0047D460.mode = 1;
}

/* func_00002EB4 — list: start scrolling out.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002EB4(void) {
    D_0047D460.mode = 3;
}

/* func_00002EC4 — list: is the current page still being typed?
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00002EC4(void) {
    List *l = &D_0047D460;

    if (D_0047D460.x14C) {
        return 0;
    }
    if (l->x154) {
        return 0;
    }
    if (l->text == 0) {
        return 0;
    } else {
        return l->mode == 2;
    }
}

/* func_00002F14 — list: page finished (or waiting) check.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00002F14(void) {
    List *l = &D_0047D460;

    if (D_0047D460.x150 == 0) {
        if (l->x14C) {
            return 0;
        }
        if (l->x154) {
            return 0;
        }
        if (l->text == 0) {
            return 0;
        } else {
            switch (l->mode) {
            case 2:
                return 1;
            }
            return 0;
        }
    }
    return 1;
}

/* func_00002F7C — list: is it scrolling?
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00002F7C(void) {
    switch (D_0047D460.mode) {
    case 1:
    case 3:
        return 1;
    }
    return 0;
}

/* func_00002FB0 — list window setup. Field copies are written in the order their
 * sources are LOADED (lever 4).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002FB0(void) {
    MsgRec *r = &D_0047D460.rec;

    ehsys_30FECAEF(&D_0047D460.hdr, D_0047D460.xD0);
    ehsys_memset(r, 0, 0x7C);
    r->r00 = 1;
    r->r30 = 1;
    r->r3C = 1;
    r->r08 = D_0047D460.xF4;
    r->r09 = 2;
    r->r4C = 2;
    r->r0C = D_0047D460.xD4;
    r->r2C = 0;
    r->r0E = 0xD2;
    r->r20 = D_0047D460.xD8;
    r->r38 = 0;
    r->r58 = 0;
    r->r22 = 0x3C;
    r->r24 = -1;
    r->r60 = -1;
    r->r50 = D_0047D460.xDC;
    r->r5C = D_0047D460.xDC;
    r->r54 = 0xFF000000;
    r->r48 = 4;
    r->r4A = 0x14;
    ehsys_50B8DF28(&D_0047D460.hdr, r);
    func_00002678(D_0047D460.xD4, D_0047D460.xD8, D_0047D460.xDC, D_0047D460.xF0, D_0047D460.xF4);
}

/* func_000030C4 — list: count frames while idle.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000030C4(void) {
    List *l = &D_0047D460;

    if (!func_00002EC4()) {
        l->x164++;
    }
}

/* func_00003100 — list: advance the current row; D_00008980 is the per-speed step
 * table and is unsigned (decomp-permuter).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003100(void) {
    List *l = &D_0047D460;
    ListRow *row;
    int lim;
    unsigned short sel;

    if (D_0047D460.text == 0) {
        l->x14C = 1;
        return;
    }
    if (l->x14C) {
        return;
    }
    if (l->x154) {
        return;
    }
    row = &l->rows[l->x140];
    ehsys_5B08FB0D();
    sel = func_000034FC();
    lim = (ehsys_66F8E113(l->text + row->x10) >> 6) + 1;
    lim = lim < l->xE4 ? lim : l->xE4;
    func_00003570(sel);
    row->val += D_00008980[l->xEC];
    if (row->val >= lim) {
        row->val = lim;
        if (l->mode == 2) {
            l->x158 += l->rows[l->x140].x14;
            if (l->x158 - 1 == ehsys_82E739A4(l->text)) {
                l->x164 = 0;
                l->x14C = 1;
            } else if (l->x140 == l->xE0 - 1) {
                l->x164 = 0;
                l->x154 = 1;
            } else {
                l->x140++;
            }
        }
    }
}

/* func_00003278 — list: lay the rows out.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003278(void) {
    List *l = &D_0047D460;
    int a;
    int b;
    int c;
    int d;
    unsigned short sel = func_000034FC();
    int h;
    int i;

    D_0047D460.x140 = 0;
    D_0047D460.x144 = 0;
    ehsys_6B9DF7E8(&D_0047D460.hdr, &D_0047D460.rec, &a, &b, &c, &d);
    D_0047D460.xE4 = c;
    h = ehsys_2FD4863E() >> 6;
    for (i = 0; i < l->xE0; i++) {
        ListRow *row = &l->rows[i];
        row->x0 = a;
        row->x4 = b + h * i;
        row->val = 0;
        row->xC = h;
        row->x10 = 0;
        row->x4 += i * 3;
    }
    func_00003570(sel);
}

/* func_00003358 — list: move the rows with the window.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003358(void) {
    List *l = &D_0047D460;
    int a;
    int b;
    int c;
    int d;
    unsigned short sel = func_000034FC();
    int h;
    int i;

    D_0047D460.rec.r0C = D_0047D460.v16C.x;
    D_0047D460.rec.r0E = D_0047D460.v16C.y;
    ehsys_6B9DF7E8(&D_0047D460.hdr, &D_0047D460.rec, &a, &b, &c, &d);
    h = (ehsys_2FD4863E() >> 6) + 1;
    for (i = 0; i < l->xE0; i++) {
        ListRow *row = &l->rows[i];
        row->x0 = a;
        row->x4 = b + h * i;
        row->x4 += i * 3;
    }
    func_00003570(sel);
}

/* func_00003444 — draw one icon from a tile sheet.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003444(int *pp, int x, int y, int a3, int t0, int t1, int t2, int t3, int blink) {
    *pp = ehsys_8EA6B1E7(*pp, t2, t3, t2, t3);
    *pp = ehsys_FDD168CA(*pp, x, y, a3, t0 + t2 * blink, t1, 0, 0);
}

/* func_000034FC — list: push the text style; returns the previous font.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
unsigned short func_000034FC(void) {
    MsgRec *r = &D_0047D460.rec;
    unsigned short old = ehsys_0D961646();

    ehsys_D853F15C(1);
    ehsys_C792FA38(0);
    ehsys_4AA58320(1);
    ehsys_942B03D0(r->r50, r->r50);
    ehsys_C9D9E989(r->r3C);
    ehsys_60B55A50(r->r54);
    ehsys_AB962AE7(0xC0);
    return old;
}

/* func_00003570 — list: pop the text style.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003570(unsigned short sel) {
    ehsys_06380DFA(sel);
    ehsys_AB962AE7(0);
}

/* func_00003594 — list: break the text into rows.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003594(void) {
    List *l = &D_0047D460;
    int pos;
    int i;
    unsigned short sel;
    unsigned int end;
    ListRow *row;

    if (D_0047D460.text == 0) {
        return;
    }
    sel = func_000034FC();
    ehsys_60B55A50(0);
    pos = l->x144;
    for (i = 0; i < l->xE0; i++) {
        row = &l->rows[i];
        row->x10 = pos;
        row->x14 = func_00002DC8(row->x0, row->x4, l->xE4, row->xC + 2, l->text + pos);
        end = pos + row->x14;
        if (end < ehsys_82E739A4(l->text)) {
            if (l->text[end] == 0x20) {
                row->x14++;
            }
        }
        pos += row->x14;
    }
    func_00003570(sel);
}

/*
 * rel_cardalbum.prx — reconstructed code (matching decompilation)
 *
 * The card album ("database" menu): every collectible card in name order,
 * 18 per page, as a 3D book. Pages turn with L/R or the side arrows (holding
 * the button accelerates), triangle opens a letter-jump dialog (kana grid in
 * Japanese, A-Z otherwise), O zooms into a card's full picture (decoded from
 * JPEG with sceJpeg) where it can be panned/zoomed/rotated, X leaves.
 * Module `modcardalbum`, exports `libcardalbum_rel`: func_00000000 = draw,
 * func_00006A8C = init(arg, heap), func_00006BB4 = per-frame update.
 *
 * Compiler:     Metrowerks CodeWarrior for PSP — MWCC 1.3 SP7 (mwccpsp_3.0.1_219)
 * Flags:        -O4,s -sdatathreshold 0
 * Verification: 81 of 82 functions "MATCH 100%" with scripts/mwcc_diff.py;
 *               func_00004D64 has the right size (309 words) but a
 *               saved-register permutation (see its comment), so
 *               `make MODULE=rel_cardalbum SRC=1` still differs in that
 *               function only. Everything else — .text layout, all three jump
 *               tables — links byte-identical.
 *
 * Conventions are those of src/rel_password.c / src/rel_debug_menu.c:
 * D_/func_ placeholder names (proposed names in
 * config/symbols/rel_cardalbum.extra.txt), no .data/.bss in the object
 * (every table/string is an extern at its address; local initialised arrays
 * are struct copies from them), functions in address order.
 *
 * Data: all module state is AlbumState D_0000CB10 (0x3004 bytes, memset by
 * the init), which embeds the card-zoom/page-turn state Fx at +0x257C
 * (0xA88 bytes, memset by func_00005330). Smaller blocks: the letter-jump
 * dialog (D_0000CA70), the light (D_0000CA88), the resource loader and
 * picture request queue (D_0000CAD0), three buffer pointers (D_0000CAF8..).
 *
 * What was new in this module (also in docs/13-mwcc-cookbook.md):
 *  - 16-BYTE-ALIGNED TYPES. Vec4/Mtx (VFPU) and the 0x60-byte sprite block
 *    are __attribute__((aligned(16))): that alone explains the stack holes
 *    in func_000008B8, func_00002044 and func_000023E8.
 *  - `float p[3] = {0}` zeroes at its declaration; when the target computes
 *    values first, the array lives in an inner block (func_00000EF4).
 *  - an `int` with `(unsigned short)` casts at each assignment is masked once;
 *    an `unsigned short` local is re-masked before each use (func_00000AA0).
 *  - offsets kept apart from a constant (`2 - i % 3` + 9 slots) come from
 *    `(pg->cards + 9)[...]`, not `pg->cards[... + 9]` (func_00002754).
 *  - a state switch with `case 0: case 8: return;` (func_00000000) is what
 *    keeps the dead delay-slot copy after the early return.
 *  - returning a call's truth value folds into `sltu` unless the earlier
 *    branch falls through to the shared `return 0` (func_00005330,
 *    func_0000541C); `(unsigned)(id - K) < 2` gives the target's range test
 *    where a `case K ... K+1:` gives two compares (func_0000608C).
 *  - two original bugs are kept as compiled: a scan loop in func_00004D64
 *    that never advances and one that counts the wrong way.
 *
 * No Shift-JIS text in this module (scripts/sjis_strings.py finds none):
 * the UI strings come from strTbl_<lang>.bin in database/cardalbum.ehp.
 */

/* ---- types ---- */

typedef struct {
    float x, y, z;
} Vec3;


typedef struct {
    short c[2];
} Str2;

typedef struct {
    unsigned char c[10];
} Anim10;

typedef struct {
    char c[6];
} LangCodes6;

typedef struct {
    char c[7];
} LangCodes7;

/* sprite parameter block for ehsys_7187E0F0 (0x60 bytes) */
typedef struct {
    int f00;
    char pad04[0x5C];
} SprParam __attribute__((aligned(16)));

/* model instance (0x68 bytes) returned by func_000061F4 */
typedef struct {
    char pad00[0x20];
    float frame;                   /* 0x20 current animation frame */
    char pad24[0x18];
    int flags;                     /* 0x3C */
    char pad40[0x28];
} Model;

/* one album page (0xC0 bytes, heap-allocated, doubly linked) */
typedef struct Page {
    unsigned char anim : 4;        /* 0x00 model/animation kind (D_0000C2E0 row) */
    unsigned char slot : 4;        /*      position in the stack (D_0000C9F8 row) */
    unsigned char live : 1;        /* 0x01 */
    unsigned short page : 15;      /* 0x02 page number */
    unsigned short f2_hi : 1;
    unsigned short cards[18];      /* 0x04 the 18 card ids on the page */
    float x, y, z;                 /* 0x28 */
    char pad34[0xC];
    Model mdl;                     /* 0x40 private copy of the page model */
    float speed;                   /* 0xA8 */
    struct Page *prev;             /* 0xAC */
    struct Page *next;             /* 0xB0 */
    char padB4[0xC];
} Page;

/* one vertex of the page-turn mesh (16 bytes) */
typedef struct {
    unsigned short u, v;
    float x, y, z;
} Vtx;

/* full-screen page-turn effect, D_0000CB10.fx = D_0000F08C */
typedef struct {
    unsigned char on;              /* 0x000 card zoom state, 0..5 (func_00005744) */
    unsigned char jpeg : 1;        /* 0x001 JPEG decoder is set up */
    unsigned char active : 1;      /*       the card picture is decoded */
    unsigned char buf : 1;         /*       double-buffered mesh */
    void *tex;                     /* 0x004 decoded picture (0x40000 bytes) */
    int dx;                        /* 0x008 */
    int dy;                        /* 0x00C */
    int size;                      /* 0x010 */
    float angle;                   /* 0x014 */
    int sdx;                       /* 0x018 saved by the zoom-out */
    int sdy;                       /* 0x01C */
    int ssize;                     /* 0x020 */
    float sangle;                  /* 0x024 */
    Vtx vtx[2][9][9];              /* 0x028 */
    unsigned char idx[0x12];       /* 0xA48 strip index list */
    int handle;                    /* 0xA5C card-picture archive handle */
    void *jbuf;                    /* 0xA60 its 0x45000-byte buffer */
    int card;                      /* 0xA64 card being shown */
    int card_sub;                  /* 0xA68 ehsys_B401BB25(card) */
    int fade;                      /* 0xA6C 0..10 */
    int fade2;                     /* 0xA70 0..10 zoom progress */
    unsigned short step;           /* 0xA74 decoder step */
    unsigned short fA76;
    void *ycc;                     /* 0xA78 64-byte aligned YCbCr buffer */
    void *ycc_raw;                 /* 0xA7C */
    int ycc_size;                  /* 0xA80 */
    int info;                      /* 0xA84 */
} Fx;

/* scroll state for ehsys_680BA5D6, D_0000CB10.scroll = D_0000F054 */
typedef struct {
    int f00;
    int f04;
    int pos;                       /* 0x8 */
    unsigned int max;              /* 0xC */
} Scroll;

/* the module's whole state, D_0000CB10 (0x3004 bytes; memset by func_00006A8C) */
typedef struct {
    unsigned short mode;           /* 0x0000 func_00006BB4's state, 0..8 */
    unsigned short step;           /* 0x0002 */
    unsigned char arg : 2;         /* 0x0004 init argument */
    unsigned char f4b : 2;
    Page *pages;                   /* 0x0008 page list */
    unsigned short n_main;         /* 0x000C slots of the regular part */
    unsigned short n_total;        /* 0x000E */
    unsigned short cards[0x1277];  /* 0x0010 album slots: 0 = not owned */
    char pad24FE[0x24];
    unsigned short permille;       /* 0x2522 album completion, 0..1000 */
    float f2524;                   /* 0x2524 */
    unsigned char view : 2;        /* 0x2528 */
    unsigned char idx : 6;
    unsigned char f2529_lo : 4;    /* 0x2529 */
    unsigned char f2529_hi : 2;
    int f252C;                     /* 0x252C */
    float f2530;                   /* 0x2530 */
    float f2534;                   /* 0x2534 */
    unsigned char jumping : 1;     /* 0x2538 multi-page jump in progress */
    unsigned char jump_idx : 6;    /*        cursor position after the jump */
    unsigned short jump_page;      /* 0x253A target page */
    float jump_speed;              /* 0x253C */
    unsigned char f2540 : 1;       /* 0x2540 */
    Scroll scroll;                 /* 0x2544 */
    char pad2554[0x10];
    int heap;                      /* 0x2564 */
    int handle;                    /* 0x2568 */
    int handle2;                   /* 0x256C */
    int block;                     /* 0x2570 memory block id (own heap only) */
    void *block_addr;              /* 0x2574 */
    unsigned short timer;          /* 0x2578 */
    Fx fx;                         /* 0x257C (0xA88 bytes, memset by func_00005330) */
} AlbumState;

/* ---- module data ---- */
extern AlbumState D_0000CB10;

/* letter-jump dialog state, D_0000CA70 (0x18 bytes) */
typedef struct {
    unsigned char col : 4;
    unsigned char row : 4;
} JumpCursor;

typedef struct {
    unsigned char bottom : 1;
} JumpFlags;

typedef struct {
    unsigned char f0b0 : 1;
} JumpFlags2;

typedef struct {
    unsigned short f00;            /* 0x00 */
    JumpCursor cur;                /* 0x02 */
    JumpFlags flags;               /* 0x03 */
    unsigned char *has_group;      /* 0x04 one flag per name-initial group */
    JumpFlags2 flags2;             /* 0x08 */
    unsigned char *layout;         /* 0x0C cols x rows grid of group indices */
    unsigned char cols;            /* 0x10 */
    unsigned char rows;            /* 0x11 */
    struct Widget *win;            /* 0x14 */
} JumpState;

/* box part of an engine dialog window (at Widget+0x4C) */
typedef struct {
    int f00;                       /* 0x00 */
    int f04;
    unsigned char f08;             /* 0x08 */
    unsigned char f09;             /* 0x09 */
    char pad0A[2];
    short x;                       /* 0x0C */
    short y;                       /* 0x0E */
    char pad10[0x10];
    short w;                       /* 0x20 */
    short h;                       /* 0x22 */
    int f24;                       /* 0x24 */
    int f28;
    int f2C;                       /* 0x2C */
    int f30;                       /* 0x30 */
    char pad34[0x14];
    short f48;                     /* 0x48 */
    short f4A;                     /* 0x4A */
    short f4C;                     /* 0x4C */
    short f4E;                     /* 0x4E */
    char pad50[0x8];
    void *title;                   /* 0x58 */
    unsigned char f5C;             /* 0x5C */
    int f60;                       /* 0x60 */
    int f64;                       /* 0x64 */
    int f68;                       /* 0x68 */
    char pad6C[0x10];
} WinBox;

typedef struct {
    unsigned char b0 : 1;
    unsigned char b1 : 1;
} WinFlags;

typedef struct {
    unsigned char cancelled : 1;
} WinFlags2;

/* engine dialog window (0x124 bytes; ehsys_2012884C opens it) */
typedef struct Widget {
    char pad00[0x4C];
    WinBox box;                    /* 0x4C */
    int heap;                      /* 0xC8 */
    WinFlags flags;                /* 0xCC */
    char padCD[0x37];
    void (*cb)(int, int, int, int); /* 0x104 draw callback */
    char pad108[0x1C];
} Widget;

extern char D_0000C240[]; /* {'/', 0}: page counter separator */
extern char D_0000C244[]; /* {'0', 0}: digit template */
extern char D_0000C234[]; /* hand cursor animation frames (10 bytes) */
extern unsigned short D_0000C228[]; /* hand x per column */
typedef struct {
    unsigned short x;
    unsigned short pad;
    unsigned short y;
    unsigned short pad2;
} Pos8;
extern Pos8 D_0000C198[];

extern JumpState D_0000CA70;

/* resource loader, D_0000CAD0 (0x26 bytes) */
typedef struct {
    unsigned char ready : 1;       /* no request in flight */
    unsigned char step : 7;
    unsigned char pad;
    unsigned short queue[18];      /* 0x02 card pictures waiting to be requested */
} Loader;

/* one entry of the 33-entry resource table at D_0000C694 (16 bytes) */
typedef struct {
    unsigned short id;             /* 0x1001..0x1032, qsorted */
    void *data;
    char *fmt;                     /* file name in database/cardalbum.ehp, may hold %c */
    char *aux;                     /* second file (.tmf animation) */
} ResEntry;

/* one entry of the 5-entry model table at D_0000C96C (20 bytes) */
typedef struct {
    unsigned short id;             /* 0x1020..0x1024 */
    Model *data;
    char *tms;
    char *tmt;
    char *extra;
} MdlEntry;

typedef struct {
    void *res[5];
    int pad[2];
    int handle;
    int pad2;
} ResParam;

extern Loader D_0000CAD0;
extern void *D_0000CAF8;           /* database/cardalbum.ehp, once loaded */
extern void *D_0000CAFC;           /* 0x74000-byte work buffer */
extern void *D_0000CB00;           /* 0x3F8000-byte card picture buffer */
extern ResEntry D_0000C694[];
extern MdlEntry D_0000C96C[];
extern char D_0000C9D0[];          /* "database/cardalbum.ehp" */
extern char D_0000C9E8[];          /* "jegfis" */
extern char D_0000C9F0[];          /* "jagfisb" */
extern Vec3 D_0000C9F8[];          /* page position per stack slot */
extern char D_0000CA58[];          /* memory block name */
extern unsigned char D_0000C248[]; /* 10x5 kana group grid (Japanese) */
extern unsigned char D_0000C27C[]; /* 7x4 letter group grid */

/* pad state returned by ehsys_F5E3080C */
typedef struct {
    int f0;
    int press;  /* 0x4 newly pressed buttons */
    int f8;
    int repeat; /* 0xC auto-repeat (d-pad) */
    int f10;
    int stick_y; /* 0x14 analog stick, 0..0xFF */
} Pad;

/* light setup for ehsys_1C2A8EDD, D_0000CA88 (0x3C bytes) */
typedef struct {
    int f00;
    int color;
    int f08;
    int f0C;
    Vec3 dir;
    char pad1C[0x20];
} Light;

typedef struct {
    float m[16];
} Mtx __attribute__((aligned(16)));

typedef struct {
    float x, y, z, w;
} Vec4 __attribute__((aligned(16)));

extern Light D_0000CA88;

/* page model animations, D_0000C2E0 (16 bytes per row) */
typedef struct {
    unsigned short id;             /* model resource */
    float start;                   /* first frame */
    float end;                     /* last frame */
    unsigned char next;            /* row to chain to at the end */
} AnimDef;

/* page cursor positions, D_0000C360 (x, z per card slot) */
typedef struct {
    float x, z;
} Pos2;
extern Pos2 D_0000C360[];
extern float D_0000C110;
extern float D_0000C154;   /* pi/2 */
extern float D_0000C158;   /* pi */
extern float D_0000C15C;   /* 2 pi */
extern float D_0000C160[2]; /* -pi, 2 degrees */
extern int D_0000C3F0[];  /* cards outside the regular album, -1 terminated */
extern float D_0000C168;
extern float D_0000C16C;

/* draw parameters for ehsys_BB22B13B (0x34 bytes); `pg` reaches the mesh
 * callback func_00002E44 */
typedef struct {
    int f00;
    int f04;
    char pad08[0x28];
    struct Page *pg;               /* 0x30 */
} DrawParam;
extern AnimDef D_0000C2E0[];
extern float D_0000C114;
extern float D_0000C118;
extern float D_0000C298[3];  /* light direction */
extern float D_0000C2A4[3];  /* model position */
extern float D_0000C2B0[4];  /* camera eye */
extern float D_0000C2C0[4];  /* camera target */
extern float D_0000C2D0[4];  /* camera up */
extern float D_0000C11C;
extern float D_0000C120;

/* ---- imports ---- */
extern int ehsys_09794DF0(int, int, int);
extern int ehsys_1D98951B(int, int);
extern void ehsys_1EC5342B(int);
extern int ehsys_399AC68A(int, int);
extern int ehsys_4175E3DB(int, int);
extern int ehsys_42110BF9(int, int);
extern void ehsys_4F22C9AA(int);
extern int ehsys_5CBB9E43(int, int, int, int, int, int);
extern void ehsys_60B55A50(int);
extern int ehsys_66F8E113(void *);
extern int ehsys_680BA5D6(int, Scroll *);
extern int ehsys_7187E0F0(int, void *, void *);
extern void ehsys_878EE1D3(int *);
extern int ehsys_85F89FE8(int);
extern int ehsys_89812D8A(int);
extern int ehsys_8EA6B1E7(int, int, int, int, int);
extern void ehsys_942B03D0(int, int);
extern int ehsys_9EA6989A(int, void *);
extern int ehsys_A4AFF8E6(void);
extern unsigned int *ehsys_B2B9FEFA(void);
extern void ehsys_B89D38DC(int, int, int, void *);
extern void ehsys_B8AD96EA(int);
extern void ehsys_C1F72EF5(void);
extern int ehsys_C62F9EAC(int, int);
extern int ehsys_CABEA834(int, int);
extern int ehsys_DFCA450B(int, int);
extern void *ehsys_E58C0FDC(void *, int);
extern int ehsys_E6E4401F(int, int, int);
extern int ehsys_F462EE6E(int, int);
extern int ehsys_FDD168CA(int, int, int, int, int, int, int, int);
extern void ehsys_memset(void *, int, int);

extern void ehsys_2012884C(void *);
extern void ehsys_qsort(void *, int, int, int (*)(unsigned short *, unsigned short *));
extern int ehsys_291D6262(void *, int, int);
extern int ehsys_47719795(void *, int, int);
extern void ehsys_1CC5878C(void *);
extern void ehsys_60BA0722(void *);
extern int ehsys_D2A768F4(int, char *);              /* file size */
extern void ehsys_8288EEBC(int, char *, void *, int, int (*)(void *, int), void *); /* async load */
extern void ehsys_sprintf(char *, char *, ...);
extern void *ehsys_C6C15111(void *, char *, int);    /* find archive member */
extern void ehsys_FDAFCF3A(void *, void *);
extern Model *ehsys_3765ED01(void *, void *);
extern void ehsys_D0E95BD8(Model *, void *);
extern void ehsys_CB9CCF0B(Model *, void *);
extern void ehsys_4B0DABFA(int, void *);
extern int ehsys_C07BB470(char *, int, int, int, int, int);
extern void ehsys_D979E9BF(int, void **, int);
extern int ehsys_1856E536(void *, int);              /* create heap */
extern void ehsys_sceKernelChangeCurrentThreadAttr(int, int);
extern void ehsys_C9D9E989(int);
extern void ehsys_4AA58320(int);
extern void ehsys_33ED567B(int);
extern void ehsys_D853F15C(int);
extern void ehsys_13A4081A(int);
extern void ehsys_BFFBA174(void);
extern void ehsys_F6414A71(int, void *);
extern void ehsys_ED1410E0(int);
extern int ehsys_06A1D1D8(void);
extern int ehsys_101103B3(int);
extern int ehsys_2717C28C(int);
extern int ehsys_3E4FC8A6(void);
extern int ehsys_57724B02(void);
extern void ehsys_5DBE93DB(int);
extern void *ehsys_8C5B148E(int, int, int);
extern float ehsys_A38AA0E9(float);
extern void ehsys_B46025DF(int, int *, int *);
extern int ehsys_C01F381E(int, int, int);
extern int ehsys_D80C7D36(void *, int, int);
extern void ehsys_F6905EFA(int, int);
extern void sceJpegInitMJpeg(void);
extern void sceJpegCreateMJpeg(int, int);
extern void sceJpegDeleteMJpeg(void);
extern void sceJpegFinishMJpeg(void);
extern int sceJpegGetOutputInfo(void *, int, int *, int);
extern void sceJpegDecodeMJpegYCbCr(void *, int, void *, int, int);
extern void sceJpegCsc(void *, void *, int, int, int);
extern void ehsys_831C5769(void *, int, int);       /* animation: play frame range */
extern void ehsys_B8770B92(void *, int *, int *);   /* animation: frame range */
extern int ehsys_558B2DCB(void *);                  /* animation: current frame */
extern int ehsys_28F7224B(int);              /* card id is valid */
extern int ehsys_450CD6C5(void);             /* fade finished */
extern void ehsys_50D6F57B(unsigned short, void *, void *);
extern void ehsys_5998BFD7(int, int, int);   /* fade out */
extern int ehsys_67312719(int);
extern void ehsys_6D6C9B94(void);
extern int ehsys_7B47DFE9(int);              /* card is owned */
extern int ehsys_93EACD81(int);              /* card position in name order */
extern int ehsys_B779C0DF(void);
extern void ehsys_D1A604C7(void *);
extern void ehsys_D470D0B2(int, int, int);   /* fade in */
extern void ehsys_memcpy(void *, void *, int);
extern int ehsys_B401BB25(unsigned short);
extern int ehsys_B8DDA661(int, unsigned short, int);
extern void ehsys_FF360587(void *, int);
extern void ehsys_9490E557(void *, int, int (*)(int *, int, int, int, DrawParam *));
extern int ehsys_F96953BE(int, int, unsigned short, int, int *);
extern void ehsys_9F25AE2F(void *, float *);
extern void ehsys_BB22B13B(void *, void *);
extern void ehsys_0089B865(Vec3 *);
extern int ehsys_0939B6BE(int, int, int, int, int);
extern int ehsys_11EBE80C(int, int, int, int, int);
extern void ehsys_13A603EB(Mtx *);
extern int ehsys_1C2A8EDD(int, int, void *);
extern int ehsys_20560342(int, int, int, int, int);
extern int ehsys_23E69878(int, int);
extern int ehsys_348F1A33(int, int);
extern int ehsys_3E24ACE1(int, float, float);
extern int ehsys_459B889D(int);
extern int ehsys_5C1AEE8E(int, int);
extern int ehsys_758DE09D(int, int, int);
extern int ehsys_7B1755CF(int, float, float);
extern int ehsys_890D2BD0(int, float);
extern void ehsys_8CA1052F(float, float, float, float);
extern int ehsys_917CC4D5(int, int, float, float);
extern void ehsys_9EFDC9B9(void);
extern int ehsys_A22C8F51(int, int);
extern int ehsys_A36635DA(int, int);
extern int ehsys_B56890DA(int, int, int);
extern int ehsys_C8DF61CD(int, int, int, int);
extern void ehsys_D1C6F209(void);
extern int ehsys_D6E0AA6A(int, int);
extern int ehsys_D71FA940(int);
extern void ehsys_E2A5CE8D(Vec4 *, Vec4 *, Vec4 *);
extern void ehsys_EBD1986B(void *(*)(int), void (*)(void *));
extern int ehsys_F0243623(int, int);
extern int ehsys_F48E70FB(int, int, Mtx *);
extern int ehsys_35D67398(unsigned short);
extern int ehsys_40D6B29D(int, void *, int, int, int, int);
extern int ehsys_4E722BBD(int);
extern void ehsys_20E340D9(int, void *);
extern int ehsys_247C7989(int, int, int, int, void *, void *);
extern void ehsys_29D1B9D5(float *);
extern void ehsys_2AE3C0AD(int);
extern void ehsys_30CEF747(void *, int);
extern void ehsys_39ADDA9F(void *);
extern int ehsys_3DE5C8DC(int, int, int, int, int, int);
extern void *ehsys_41AABF28(int, int);
extern int ehsys_5471DB90(int);
extern int ehsys_572A577B(int, int, int, int, int);
extern void ehsys_73918516(float, float *, float *);
extern void ehsys_89EE81BB(float *, float *, float *);
extern int ehsys_B4F2178B(int);
extern void ehsys_B0F320E6(void *);
extern void ehsys_E1139F1A(void *);
extern unsigned short ehsys_E60D8E10(void);
extern void ehsys_EF9B5D06(int *, void *);
extern Pad *ehsys_F5E3080C(void);
extern unsigned short ehsys_get_button_code(int);
extern int ehsys_get_language(void);

/* ---- forward declarations ---- */
int func_00000294(int h);
int func_00000414(int h);
int func_000005DC(int h);
void func_00000888(int x, int y, void *str);
void func_000008A8(int x, int y, int w, void *str);
int func_000008B8(int h);
int func_00000AA0(int h, int x0, short y, int val, int step);
int func_00000B4C(int h);
void func_00000EF4(void);
void func_000013AC(void);
void func_00001708(void);
void func_00001774(void);
void func_00001C74(int h, int x, int y, int width);
void func_00001FB0(int x, int y, void *str);
int func_00001FD0(int group);
int func_00004690(int idx);
int func_00001C48(int h);
void func_00002044(void);
Vec3 *func_000023C8(Vec3 *dst, Vec3 *src);
void func_000023E8(void);
void func_000026E0(void);
void func_00002754(Page *pg, int anim, float speed);
void func_000029D8(void);
void func_00002B68(void);
void func_00002BE4(Page *pg);
void func_00002D54(int idx);
void func_00002FD0(void);
void func_00003010(void);
void func_00003EE4(void);
int func_000043E8(unsigned short id);
int func_0000442C(unsigned short *out, int id, int n);
void func_000048BC(void);
int func_00005330(void);
int func_0000541C(void);
void func_000054B8(void);
void func_0000555C(void);
int func_000055B8(void);
int func_00005744(void);
int func_00005AB8(int in);
void func_00005C90(void);
int func_00005D10(void);
void func_00005EFC(void);
void func_00005F04(void);
void *func_00005FDC(void);
int func_00006354(void *buf, int size);
int func_000063E4(unsigned short *a, unsigned short *b);
int func_000063F4(unsigned short id, void *tbl, int n, int stride);
void func_000069B8(void);
int func_00006D78(void);
void func_000031C4(void);
void func_00003434(void);
void func_00003678(void);
void func_000038C8(void);
int func_00003AE8(void);
int func_00003C00(void);
unsigned short func_00003B6C(void);
void func_00003C5C(float f);
void func_00003D90(float f);
void func_00004A54(void);
void func_00004C0C(void);
int func_00004D64(int dir);
void func_000052C4(void);
void func_0000646C(void);
void func_00006480(unsigned short card);
void func_000064C0(void);
Page *func_00006A48(unsigned short slot);
void func_00004488(void);
void func_00005238(int a);
void func_00006848(void);
void func_00006900(void);
int func_00002E44(int *ph, int a1, int mesh, int a3, DrawParam *ctx);
void *func_00002E24(int size);
void func_00002E34(void *p);
Model *func_000061F4(unsigned short id);
void func_00006800(void);
Page *func_000065DC(int type, int a1, int page);
void func_000029D8(void);
void func_00002B68(void);
Page *func_00003AB4(void);
void *func_0000608C(unsigned short id);

/* func_00000000 — exported draw callback.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000000(void) {
    int h;

    h = ehsys_DFCA450B(0, 0);
    h = ehsys_1D98951B(h, 0xFF000000);
    h = ehsys_42110BF9(h, 0);
    h = ehsys_399AC68A(h, 0);
    h = ehsys_C62F9EAC(h, 0x17);
    ehsys_1EC5342B(h);
    switch (D_0000CB10.mode) {
    case 0:
    case 8:
        return;
    }
    if (D_0000CB10.f2540) {
        ehsys_C1F72EF5();
        func_00000EF4();
    } else {
    h = ehsys_DFCA450B(0, 0);
    h = ehsys_F462EE6E(h, 9);
    h = ehsys_F462EE6E(h, 4);
    h = ehsys_09794DF0(h, 0, 1);
    h = ehsys_E6E4401F(h, 0, 0);
    h = ehsys_5CBB9E43(h, 0, 2, 3, 0, 0);
    h = ehsys_4175E3DB(h, 1);
    h = func_00000294(h);
    ehsys_1EC5342B(h);
    func_000023E8();
    ehsys_4F22C9AA(2);
    func_000029D8();
    func_00002B68();
    h = ehsys_DFCA450B(4, 0);
    h = ehsys_F462EE6E(h, 9);
    h = ehsys_F462EE6E(h, 4);
    h = ehsys_09794DF0(h, 0, 1);
    h = ehsys_E6E4401F(h, 0, 0);
    h = ehsys_5CBB9E43(h, 0, 2, 3, 0, 0);
    h = ehsys_4175E3DB(h, 1);
    h = func_000005DC(h);
    h = func_000008B8(h);
    if (D_0000CB10.mode == 3) {
        h = func_00000414(h);
    }
    h = ehsys_680BA5D6(h, &D_0000CB10.scroll);
    if (D_0000CB10.mode == 5) {
        h = func_00001C48(h);
    }
    ehsys_1EC5342B(h);
    h = ehsys_DFCA450B(5, 0);
    h = ehsys_4175E3DB(h, 1);
    ehsys_878EE1D3(&h);
    ehsys_1EC5342B(h);
    }
}

/* func_00000294 — draw the background (resource 0x1001, bg.gim); the same
 * code as rel_password's func_00000150.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000294(int h) {
    void *res = func_0000608C(0x1001);
    int i;

    if (res == 0) {
        return h;
    }
    h = ehsys_9EA6989A(h, res);
    h = ehsys_CABEA834(h, -1);
    h = ehsys_E6E4401F(h, 0, 0);
    h = ehsys_8EA6B1E7(h, 0x40, 0x100, 0x40, 0x100);
    for (i = 0; i < 7; i++) {
        h = ehsys_FDD168CA(h, i << 6, 0, 0, i << 6, 0, 0, 0);
    }
    h = ehsys_8EA6B1E7(h, 0x20, 0x100, 0x20, 0x100);
    h = ehsys_FDD168CA(h, i << 6, 0, 0, i << 6, 0, 0, 0);
    h = ehsys_8EA6B1E7(h, 0x10, 0x40, 0x40, 0x10);
    for (i = 0; i < 4; i++) {
        h = ehsys_FDD168CA(h, 0x100 - ((i + 1) << 6), 0x100, 0, 0x1E0, i << 6, 0, 3);
        h = ehsys_FDD168CA(h, 0x200 - ((i + 1) << 6), 0x100, 0, 0x1F0, i << 6, 0, 3);
    }
    return h;
}

/* func_00000414 — draw the pointing hand (resource 0x1003, hand.gim).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000414(int h) {
    void *res = func_0000608C(0x1003);
    unsigned short x;
    unsigned short y;
    unsigned short size;
    int flip;
    Anim10 anim;

    flip = 0;
    if (res == 0) {
        return h;
    }
    size = 0x80;
    switch (D_0000CB10.view) {
    case 0:
        x = D_0000C198[D_0000CB10.idx].x;
        y = D_0000C198[D_0000CB10.idx].y;
        break;
    case 1:
        if (D_0000CB10.f2529_hi == 0) {
            flip = 1;
            x = 0x96;
        } else {
            x = 0x1B8;
        }
        y = 0xC8;
        size = 0xA0;
        break;
    case 2:
        y = 0xF0;
        x = D_0000C228[D_0000CB10.f2529_lo];
        break;
    }
    h = ehsys_9EA6989A(h, res);
    h = ehsys_CABEA834(h, -1);
    h = ehsys_E6E4401F(h, 0, 0);
    h = ehsys_8EA6B1E7(h, 0x20, 0x20, 0x20, 0x20);
    anim = *(Anim10 *) D_0000C234;
    return ehsys_FDD168CA(h, x, y, 0, anim.c[(*ehsys_B2B9FEFA() >> 2) % 10] * 32 + 0x40, size, flip, 0);
}

/* func_000005DC — draw the title bar (resource 0x1002) with the album
 * title (string 7), the "completion" label (string 2) and the percentage.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000005DC(int h) {
    unsigned short s[2] = {0};
    void *res = func_0000608C(0x1002);
    int w;

    if (res != 0) {
        h = ehsys_9EA6989A(h, res);
        h = ehsys_CABEA834(h, -1);
        h = ehsys_E6E4401F(h, 0, 0);
        h = ehsys_8EA6B1E7(h, 0xC, 0x19, 0xC, 0x19);
        h = ehsys_FDD168CA(h, 0x91, 0, 0, 0, 0x14, 0, 0);
        h = ehsys_8EA6B1E7(h, 1, 0x19, 0x137, 0x19);
        h = ehsys_FDD168CA(h, 0x9D, 0, 0, 6, 0x14, 0, 0);
        h = ehsys_8EA6B1E7(h, 0xC, 0x19, 0xC, 0x19);
        h = ehsys_FDD168CA(h, 0x1D4, 0, 0, 0xD, 0x14, 0, 0);
    }
    ehsys_942B03D0(0xE, 0xE);
    ehsys_60B55A50(-1);
    ehsys_B8AD96EA(h);
    func_000008A8(0x96, 4, 0xB4, ehsys_E58C0FDC(func_0000608C(0x1030), 7));
    w = ehsys_66F8E113(ehsys_E58C0FDC(func_0000608C(0x1030), 2));
    ehsys_B89D38DC(0x68C0 - w, 0x100, w + 0x40, ehsys_E58C0FDC(func_0000608C(0x1030), 2));
    s[0] = 0x25;
    func_00000888(0x1CE, 4, s);
    if (D_0000CB10.permille > 999) {
        h = ehsys_A4AFF8E6();
        ehsys_942B03D0(0xC, 0xE);
        return func_00000AA0(h, 0x1CC, 4, 100, 9);
    }
    s[0] = 0x2E;
    func_00000888(0x1BF, 4, s);
    h = ehsys_A4AFF8E6();
    ehsys_942B03D0(0xC, 0xE);
    h = func_00000AA0(h, 0x1BF, 4, (D_0000CB10.permille / 10) % 100, 9);
    return func_00000AA0(h, 0x1CC, 4, D_0000CB10.permille % 10, 9);
}

/* func_00000888 — draw a string from x to the right edge (0x1E0), pixels
 * -> 26.6 fixed point.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000888(int x, int y, void *str) {
    ehsys_B89D38DC(x << 6, y << 6, (0x1E0 - x) << 6, str);
}

/* func_000008A8 — draw a string (pixels -> 26.6 fixed point).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000008A8(int x, int y, int w, void *str) {
    ehsys_B89D38DC(x << 6, y << 6, w << 6, str);
}

/* func_000008B8 — draw the album frame (resource 0x1010) and the page
 * counter "n/m" (or string 3 past the last page).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000008B8(int h) {
    Str2 s;
    SprParam p;
    void *res = func_0000608C(0x1010);
    Page *pg;
    int w;

    if (res != 0) {
        ehsys_memset(&p, 0, sizeof(p));
        p.f00 = *ehsys_B2B9FEFA();
        h = ehsys_85F89FE8(h);
        h = ehsys_89812D8A(ehsys_7187E0F0(h, res, &p));
    }
    ehsys_942B03D0(0xC, 0xE);
    ehsys_60B55A50(-1);
    pg = func_00003AB4();
    if (pg != 0) {
        if (pg->page >= (D_0000CB10.n_main + 0x11) / 0x12) {
            w = ehsys_66F8E113(ehsys_E58C0FDC(func_0000608C(0x1030), 3));
            ehsys_B8AD96EA(h);
            ehsys_B89D38DC(0x4E40 - w / 2, 0x3DC0, 0xF00, ehsys_E58C0FDC(func_0000608C(0x1030), 3));
            h = ehsys_A4AFF8E6();
        } else {
            s = *(Str2 *) D_0000C240;
            h = func_00000AA0(h, 0x135, 0xF7, (pg->page + 1) % 1000, 9);
            h = func_00000AA0(h, 0x157, 0xF7, ((D_0000CB10.n_main + 0x11) / 0x12) % 1000, 9);
            ehsys_B8AD96EA(h);
            func_00000888(0x136, 0xF7, &s);
            h = ehsys_A4AFF8E6();
        }
    }
    return h;
}

/* func_00000AA0 — draw `val` in decimal, right-aligned so that its last
 * digit ends at x, `step` pixels per digit.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000AA0(int h, int x0, short y, int val, int step) {
    Str2 s = *(Str2 *) D_0000C244;
    int x = (unsigned short) x0;

    ehsys_B8AD96EA(h);
    do {
        x = (unsigned short) (x - step);
        s.c[0] = val % 10 + 0x30;
        func_00000888(x, y, &s);
        val /= 10;
    } while (val > 0);
    return ehsys_A4AFF8E6();
}

/* func_00000B4C — draw the help bar (resource 0x100E) and its button
 * labels (strings 4, 5, 6, 9), faded by D_0000CB10.fx.fade (0..10).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000B4C(int h) {
    int a = (D_0000CB10.fx.fade * 255) / 10;
    void *res = func_0000608C(0x100E);
    int w;
    int x;

    h = ehsys_F462EE6E(h, 9);
    h = ehsys_09794DF0(h, 0, 1);
    h = ehsys_9EA6989A(h, res);
    a = (a << 24) | 0xFFFFFF;
    h = ehsys_CABEA834(h, a);
    h = ehsys_E6E4401F(h, 0, 0);
    h = ehsys_8EA6B1E7(h, 0x16, 0xA, 0x16, 0xA);
    h = ehsys_FDD168CA(h, 0, 0, 0, 0, 0x2A, 0, 0);
    h = ehsys_FDD168CA(h, 0x1CA, 0, 0, 0x18, 0x2A, 0, 0);
    h = ehsys_8EA6B1E7(h, 0x26, 0x26, 0x26, 0x26);
    h = ehsys_FDD168CA(h, 0xB, 0xB0, 0, 2, 2, 0, 0);
    h = ehsys_8EA6B1E7(h, 0x13, 0x13, 0x13, 0x13);
    h = ehsys_FDD168CA(h, 0x15, 0xEA, 0, 0x5F, 0xB, 0, 0);
    h = ehsys_8EA6B1E7(h, 0x10, 0x10, 0x10, 0x10);
    h = ehsys_FDD168CA(h, 0x1BA, 0xBB, 0, 0x30, 0x2A, 0, 0);
    ehsys_942B03D0(0xC, 0xC);
    ehsys_60B55A50(a);
    ehsys_B8AD96EA(h);
    w = ehsys_66F8E113(ehsys_E58C0FDC(func_0000608C(0x1030), 6)) >> 6;
    func_000008A8(0x16, 0, 0x64, ehsys_E58C0FDC(func_0000608C(0x1030), 6));
    func_000008A8(0x1CA - w, 0, 0x64, ehsys_E58C0FDC(func_0000608C(0x1030), 6));
    x = 0x1D - (ehsys_66F8E113(ehsys_E58C0FDC(func_0000608C(0x1030), 4)) >> 6) / 2;
    if (x < 0) {
        x = 0;
    }
    func_000008A8(x, 0xD8, 0x64, ehsys_E58C0FDC(func_0000608C(0x1030), 4));
    w = ehsys_66F8E113(ehsys_E58C0FDC(func_0000608C(0x1030), 9)) >> 6;
    func_000008A8(0x1D - w / 2, 0xFF, 0x64, ehsys_E58C0FDC(func_0000608C(0x1030), 9));
    w = ehsys_66F8E113(ehsys_E58C0FDC(func_0000608C(0x1030), 5)) >> 6;
    x = 0x1C2 - w / 2;
    if (x + w > 0x1E0) {
        x = 0x1E0 - w;
    }
    func_000008A8(x, 0xAE, 0x64, ehsys_E58C0FDC(func_0000608C(0x1030), 5));
    return ehsys_A4AFF8E6();
}

/* func_00000EF4 — draw the page-turn effect: a 9x9 mesh, rotated by
 * fx.angle and scaled by fx.size, drawn as eight triangle strips over the
 * captured screen, then the help bar on top.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000EF4(void) {
    Fx *fx = &D_0000CB10.fx;
    float m[9];
    int h;
    int i, j;
    int v;
    float t, y, s;

    if (fx->on == 0) {
        return;
    }
    h = ehsys_DFCA450B(4, 0);
    h = ehsys_4175E3DB(h, 9);
    h = ehsys_F462EE6E(h, 4);
    h = ehsys_09794DF0(h, 0, 0);
    h = ehsys_E6E4401F(h, 1, 1);
    h = ehsys_5CBB9E43(h, 0, 2, 3, 0, 0);
    h = ehsys_4175E3DB(h, 1);
    h = ehsys_CABEA834(h, (fx->fade * 200 / 10) << 24);
    h = ehsys_8EA6B1E7(h, 0, 0, 0x1E0, 0x110);
    h = ehsys_FDD168CA(h, 0, 0, 0, 0, 0, 0, 0);
    if (fx->active == 1) {
        h = ehsys_B4F2178B(h);
        h = ehsys_572A577B(h, 3, 0, 0, 0);
        h = ehsys_3DE5C8DC(h, 0, 0x100, 0x100, 0x100, (int) fx->tex);
        h = ehsys_F462EE6E(h, 9);
        h = ehsys_CABEA834(h, ((fx->fade2 * 155) / 10 + 100) << 24 | 0xFFFFFF);
        for (i = 0; i < 9; i++) {
            t = (float) i / 8.0f;
            v = (int) (256.0f * t);
            y = (float) (fx->size + 0x100) * (t - 0.5f);
            for (j = 0; j < 9; j++) {
                float x;

                s = (float) j / 8.0f;
                x = (float) (fx->size + 0x100) * (s - 0.5f);
                {
                float pos[3] = {0};

                pos[0] = x;
                pos[1] = y;
                ehsys_29D1B9D5(m);
                ehsys_73918516(fx->angle, m, m);
                ehsys_89EE81BB(pos, m, pos);
                fx->vtx[fx->buf][i][j].u = (int) (256.0f * s);
                fx->vtx[fx->buf][i][j].v = v;
                fx->vtx[fx->buf][i][j].x = 240.0f + (pos[0] + fx->dx);
                fx->vtx[fx->buf][i][j].y = 136.0f + (pos[1] + fx->dy);
                fx->vtx[fx->buf][i][j].z = 0.0f;
                }
            }
        }
        for (i = 0; i < 8; i++) {
            h = ehsys_247C7989(h, 4, 0x800982, 0x12, fx->idx, fx->vtx[fx->buf][i]);
        }
        fx->buf = (fx->buf != 0) ^ 1;
    }
    h = func_00000B4C(h);
    ehsys_1EC5342B(h);
}

/* func_000013AC — open the letter-jump dialog: flag every name-initial
 * group that has at least one card in the album, pick the kana (10x5) or
 * Latin (7x4) grid by language, and open an engine window sized for it
 * (callback func_00001C74). Mode 5.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000013AC(void) {
    int i;
    int cols;
    WinBox *b;
    Widget *w;

    ehsys_memset(&D_0000CA70, 0, sizeof(D_0000CA70));
    D_0000CA70.has_group = ehsys_41AABF28(D_0000CB10.heap, ehsys_E60D8E10());
    if (D_0000CA70.has_group == 0) {
        func_00001708();
        return;
    }
    ehsys_memset(D_0000CA70.has_group, 0, ehsys_E60D8E10());
    for (i = 0; i < D_0000CB10.n_main; i++) {
        if (D_0000CB10.cards[i] != 0) {
            D_0000CA70.has_group[ehsys_5471DB90(D_0000CB10.cards[i])] = 1;
        }
    }
    if (ehsys_get_language() == 0) {
        D_0000CA70.cols = 10;
        D_0000CA70.rows = 5;
        D_0000CA70.layout = D_0000C248;
    } else {
        D_0000CA70.cols = 7;
        D_0000CA70.rows = 4;
        D_0000CA70.layout = D_0000C27C;
    }
    if ((D_0000CB10.n_total + 0x11) / 0x12 - (D_0000CB10.n_main + 0x11) / 0x12 > 0) {
        D_0000CA70.flags2.f0b0 = 1;
    }
    w = D_0000CA70.win = ehsys_41AABF28(D_0000CB10.heap, sizeof(Widget));
    if (w == 0) {
        func_00001708();
        return;
    }
    cols = D_0000CA70.cols;
    b = &w->box;
    if (ehsys_get_language() == 0) {
        cols++;
    }
    ehsys_memset(w, 0, sizeof(Widget));
    b->f00 = 1;
    b->f08 = 0;
    b->w = cols * 22 + 0x11;
    b->h = D_0000CA70.rows * 22 + 0x11;
    b->x = (0x1E0 - b->w) / 2;
    b->y = (0x110 - b->h) / 2;
    if (D_0000CA70.flags2.f0b0) {
        b->h += 0x16;
    }
    b->f24 = -1;
    b->f48 = 8;
    b->f4C = 8;
    b->f4A = 8;
    b->f4E = 8;
    b->f2C = 1;
    b->f30 = 1;
    b->title = ehsys_E58C0FDC(func_0000608C(0x1030), 8);
    b->f09 = 2;
    b->f5C = 0xC;
    b->f60 = -1;
    b->f64 = 2;
    b->f68 = 2;
    w->heap = D_0000CB10.heap;
    w->flags.b1 = 1;
    w->flags.b0 = 1;
    w->cb = func_00001C74;
    ehsys_2012884C(w);
    D_0000CB10.mode = 5;
}

/* func_00001708 — free the letter-jump dialog.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001708(void) {
    if (D_0000CA70.has_group != 0) {
        ehsys_20E340D9(D_0000CB10.heap, D_0000CA70.has_group);
        D_0000CA70.has_group = 0;
    }
    if (D_0000CA70.win != 0) {
        ehsys_E1139F1A(D_0000CA70.win);
        ehsys_20E340D9(D_0000CB10.heap, D_0000CA70.win);
        D_0000CA70.win = 0;
    }
}

/* func_00001774 — mode 5: letter-jump dialog input. The d-pad moves the
 * cursor over the grid (and onto the extra bottom button when there are
 * unowned pages), O picks a group that has cards, X cancels. When the
 * window has closed (state 6), jump to the picked group and go back to
 * mode 3.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001774(void) {
    Widget *w = D_0000CA70.win;
    Pad *pad = ehsys_F5E3080C();
    int c;
    int idx;

    ehsys_39ADDA9F(w);
    switch (((unsigned char *) w)[0xED]) {
    case 3:
        if (pad->repeat & 0x10) {
            if (D_0000CA70.flags.bottom) {
                D_0000CA70.flags.bottom = 0;
                D_0000CA70.cur.row = D_0000CA70.rows - 1;
            } else if (D_0000CA70.cur.row > 0) {
                D_0000CA70.cur.row--;
            } else if (D_0000CA70.flags2.f0b0) {
                D_0000CA70.flags.bottom = 1;
            } else {
                D_0000CA70.cur.row = D_0000CA70.rows - 1;
            }
            ehsys_2AE3C0AD(0x94C3);
        }
        if (pad->repeat & 0x40) {
            if (D_0000CA70.flags.bottom) {
                D_0000CA70.flags.bottom = 0;
                D_0000CA70.cur.row = 0;
            } else if (D_0000CA70.cur.row < D_0000CA70.rows - 1) {
                D_0000CA70.cur.row++;
            } else if (D_0000CA70.flags2.f0b0) {
                D_0000CA70.flags.bottom = 1;
            } else {
                D_0000CA70.cur.row = 0;
            }
            ehsys_2AE3C0AD(0x94C3);
        }
        if (pad->repeat & 0x80) {
            if (D_0000CA70.cur.col > 0) {
                D_0000CA70.cur.col--;
            } else {
                D_0000CA70.cur.col = D_0000CA70.cols - 1;
            }
            ehsys_2AE3C0AD(0x94C3);
        }
        if (pad->repeat & 0x20) {
            if (D_0000CA70.cur.col < D_0000CA70.cols - 1) {
                D_0000CA70.cur.col++;
            } else {
                D_0000CA70.cur.col = 0;
            }
            ehsys_2AE3C0AD(0x94C3);
        }
        if (pad->press & ehsys_get_button_code(1)) {
            ((WinFlags2 *) ((unsigned char *) w + 0xEC))->cancelled = 1;
            ehsys_B0F320E6(w);
            ehsys_2AE3C0AD(0x94C5);
        } else if (pad->press & ehsys_get_button_code(0)) {
            idx = D_0000CA70.cur.col + D_0000CA70.cur.row * D_0000CA70.cols;
            c = D_0000CA70.layout[idx];
            if (D_0000CA70.flags.bottom || (c != 0xFF && D_0000CA70.has_group[c] != 0)) {
                ehsys_30CEF747(w, 0);
            }
            ehsys_2AE3C0AD(0x94C4);
        }
        break;
    case 6:
        D_0000CB10.mode = 3;
        if (!((WinFlags2 *) ((unsigned char *) w + 0xEC))->cancelled) {
            if (D_0000CA70.flags.bottom) {
                idx = (D_0000CB10.n_main + 0x11) / 0x12 * 0x12;
            } else {
                idx = D_0000CA70.cur.col + D_0000CA70.cur.row * D_0000CA70.cols;
                idx = func_00001FD0(D_0000CA70.layout[idx]);
            }
            if (idx >= 0) {
                func_00004690(idx);
            }
        }
        func_00001708();
        break;
    }
}

/* func_00001C48 — draw the letter-jump dialog into display list h.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00001C48(int h) {
    ehsys_EF9B5D06(&h, D_0000CA70.win);
    return h;
}

/* func_00001C74 — letter-jump dialog draw callback (Widget.cb): the
 * cursor box, then every group initial of the grid (dimmed when the album
 * has no card in that group); the Japanese grid leaves a gap column in its
 * middle. The optional bottom button shows string 3.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001C74(int h, int x, int y, int width) {
    unsigned short s[2] = {0};
    int row;
    int col;
    int i;
    int cx;
    unsigned int w;

    if (D_0000CA70.flags.bottom) {
        h = ehsys_40D6B29D(h, D_0000CA70.win, x + (width - 0x4E) / 2, y + D_0000CA70.rows * 22 - 4, 0x4E, 0x16);
    } else {
        cx = D_0000CA70.cur.col;
        if (ehsys_get_language() == 0 && cx >= D_0000CA70.cols / 2) {
            cx++;
        }
        h = ehsys_40D6B29D(h, D_0000CA70.win, x + cx * 22 - 4, y + D_0000CA70.cur.row * 22 - 4, 0x16, 0x16);
    }
    ehsys_942B03D0(0xE, 0xE);
    ehsys_B8AD96EA(h);
    for (i = 0, row = 0; row < D_0000CA70.rows; row++) {
        for (col = 0; col < D_0000CA70.cols; i++, col++) {
            w = D_0000CA70.layout[i];
            cx = col;
            if (D_0000CA70.has_group[w]) {
                ehsys_60B55A50(0xFF000000);
            } else {
                ehsys_60B55A50(0x80000000);
            }
            s[0] = ehsys_35D67398(w);
            w = ehsys_66F8E113(s) >> 6;
            if (ehsys_get_language() == 0 && col >= D_0000CA70.cols / 2) {
                cx++;
            }
            func_00001FB0(x + cx * 22 + (14 - w) / 2, y + row * 22, s);
        }
    }
    if (D_0000CA70.flags2.f0b0) {
        ehsys_60B55A50(0xFF000000);
        w = ehsys_66F8E113(ehsys_E58C0FDC(func_0000608C(0x1030), 3)) >> 6;
        func_00001FB0(x + (width - w) / 2, y + D_0000CA70.rows * 22, ehsys_E58C0FDC(func_0000608C(0x1030), 3));
    }
    ehsys_A4AFF8E6();
}

/* func_00001FB0 — draw a string from x to the right edge (0x1E0); a
 * second copy of func_00000888 (static helper of another source file).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001FB0(int x, int y, void *str) {
    ehsys_B89D38DC(x << 6, y << 6, (0x1E0 - x) << 6, str);
}

/* func_00001FD0 — index of the first album card whose name-initial group
 * is `group` (n_main if none).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00001FD0(int group) {
    int i;

    for (i = 0; i < D_0000CB10.n_main; i++) {
        if (D_0000CB10.cards[i] != 0 && group == ehsys_4E722BBD(D_0000CB10.cards[i])) {
            break;
        }
    }
    return i;
}

/* func_00002044 — one-time 3D setup: render state, light (D_0000CA88),
 * projection, and the eight matrix slots reset to identity.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002044(void) {
    Vec3 v;
    Mtx m;
    int h;
    int i;

    h = ehsys_DFCA450B(0, 0);
    h = ehsys_B56890DA(h, 0x710, 0x778);
    h = ehsys_0939B6BE(h, 0x800, 0x800, 0x1E0, 0x110);
    h = ehsys_758DE09D(h, 0xFFFF, 0);
    h = ehsys_5CBB9E43(h, 0, 2, 3, 0, 0);
    h = ehsys_4175E3DB(h, 4);
    h = ehsys_20560342(h, 0, 0, 0x1E0, 0x110);
    h = ehsys_F462EE6E(h, 2);
    h = ehsys_C8DF61CD(h, 6, 0, 0xFF);
    h = ehsys_4175E3DB(h, 0);
    h = ehsys_348F1A33(h, 7);
    h = ehsys_F462EE6E(h, 1);
    h = ehsys_A36635DA(h, 1);
    h = ehsys_F462EE6E(h, 5);
    h = ehsys_4175E3DB(h, 9);
    h = ehsys_5C1AEE8E(h, 1);
    h = ehsys_F462EE6E(h, 8);
    h = ehsys_4175E3DB(h, 3);
    h = ehsys_4175E3DB(h, 6);
    h = ehsys_4175E3DB(h, 0x12);
    h = ehsys_4175E3DB(h, 0x11);
    h = ehsys_4175E3DB(h, 0x15);
    h = ehsys_23E69878(h, 0xFF0000FF);
    h = ehsys_459B889D(h);
    ehsys_EBD1986B(func_00002E24, func_00002E34);
    ehsys_memset(&D_0000CA88, 0, sizeof(D_0000CA88));
    D_0000CA88.f00 = 0;
    D_0000CA88.color = 0xFF000000;
    D_0000CA88.f08 = -1;
    D_0000CA88.f0C = 0;
    v.x = D_0000C298[0];
    v.y = D_0000C298[1];
    v.z = D_0000C298[2];
    func_000023C8(&D_0000CA88.dir, &v);
    h = ehsys_D71FA940(h);
    h = ehsys_A22C8F51(h, 0);
    h = ehsys_D6E0AA6A(h, 0xFF333333);
    h = ehsys_11EBE80C(h, 0, -1, -1, -1);
    h = ehsys_F0243623(h, 7);
    h = ehsys_890D2BD0(h, 10.0f);
    h = ehsys_917CC4D5(h, 0x404040, 50.0f, 100.0f);
    h = ehsys_4175E3DB(h, 7);
    ehsys_4F22C9AA(0);
    ehsys_9EFDC9B9();
    ehsys_8CA1052F(D_0000C120, D_0000C11C, 10.0f, 40.0f);
    ehsys_4F22C9AA(1);
    ehsys_9EFDC9B9();
    ehsys_4F22C9AA(2);
    ehsys_9EFDC9B9();
    h = ehsys_1D98951B(h, 0xFF770000);
    h = ehsys_399AC68A(h, 0);
    h = ehsys_42110BF9(h, 0);
    h = ehsys_E6E4401F(h, 1, 1);
    h = ehsys_7B1755CF(h, 0.0f, 0.0f);
    h = ehsys_3E24ACE1(h, 1.0f, -1.0f);
    h = ehsys_F462EE6E(h, 9);
    ehsys_13A603EB(&m);
    for (i = 0; i < 8; i++) {
        h = ehsys_F48E70FB(h, i, &m);
    }
    ehsys_1EC5342B(h);
}

/* func_000023C8 — Vec3 copy.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
Vec3 *func_000023C8(Vec3 *dst, Vec3 *src) {
    dst->x = src->x;
    dst->y = src->y;
    dst->z = src->z;
    return dst;
}

static inline Vec3 MakeVec3(float x, float y, float z) {
    Vec3 v;

    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

/* func_000023E8 — per-frame 3D pass setup: light, render state, the
 * model position (slid by D_0000CB10.f2524), projection and camera.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000023E8(void) {
    Vec3 pos;
    float tmp[3];
    Vec4 eye;
    Vec4 at;
    Vec4 up;
    int h;

    h = ehsys_DFCA450B(1, 0);
    h = ehsys_1C2A8EDD(h, 0, &D_0000CA88);
    h = ehsys_D71FA940(h);
    h = ehsys_A22C8F51(h, 0);
    h = ehsys_D6E0AA6A(h, 0xFF333333);
    h = ehsys_B56890DA(h, 0x710, 0x778);
    h = ehsys_0939B6BE(h, 0x800, 0x800, 0x1E0, 0x110);
    h = ehsys_758DE09D(h, 0xFFFF, 0);
    h = ehsys_5CBB9E43(h, 0, 2, 3, 0, 0);
    h = ehsys_4175E3DB(h, 4);
    h = ehsys_20560342(h, 0, 0, 0x1E0, 0x110);
    h = ehsys_F462EE6E(h, 2);
    h = ehsys_C8DF61CD(h, 6, 0, 0xFF);
    h = ehsys_4175E3DB(h, 0);
    h = ehsys_348F1A33(h, 7);
    h = ehsys_F462EE6E(h, 1);
    h = ehsys_A36635DA(h, 1);
    h = ehsys_F462EE6E(h, 5);
    h = ehsys_4175E3DB(h, 9);
    h = ehsys_5C1AEE8E(h, 1);
    h = ehsys_F462EE6E(h, 8);
    h = ehsys_23E69878(h, 0xFF0000FF);
    h = ehsys_917CC4D5(h, 0x404040, 50.0f, 100.0f);
    h = ehsys_4175E3DB(h, 7);
    h = ehsys_E6E4401F(h, 1, 1);
    ehsys_1EC5342B(h);
    ehsys_4F22C9AA(0);
    ehsys_9EFDC9B9();
    tmp[0] = D_0000C2A4[0];
    tmp[1] = D_0000C2A4[1];
    tmp[2] = D_0000C2A4[2];
    pos = *(Vec3 *) tmp;
    pos.x += D_0000CB10.f2524;
    ehsys_0089B865(&pos);
    ehsys_8CA1052F(D_0000C120, D_0000C11C, 10.0f, 40.0f);
    ehsys_4F22C9AA(1);
    ehsys_9EFDC9B9();
    up.x = D_0000C2D0[0];
    up.y = D_0000C2D0[1];
    up.z = D_0000C2D0[2];
    up.w = D_0000C2D0[3];
    at.x = D_0000C2C0[0];
    at.y = D_0000C2C0[1];
    at.z = D_0000C2C0[2];
    at.w = D_0000C2C0[3];
    eye.x = D_0000C2B0[0];
    eye.y = D_0000C2B0[1];
    eye.z = D_0000C2B0[2];
    eye.w = D_0000C2B0[3];
    ehsys_E2A5CE8D(&eye, &at, &up);
    ehsys_D1C6F209();
    ehsys_4F22C9AA(2);
    ehsys_9EFDC9B9();
}

/* func_000026E0 — build the album: reset the cover model's animation,
 * flag the album as open, free the old pages and create the cover
 * (type 4) and the first two pages (types 5, 6).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000026E0(void) {
    Model *mdl = func_000061F4(0x1020);

    D_0000CB10.f4b = 1;
    mdl->frame = 0.0f;
    func_00006800();
    func_000065DC(4, 3, 0);
    func_000065DC(5, 3, 1);
    func_000065DC(6, 3, 2);
}

/* func_00002754 — (re)bind page `pg` to animation row `anim`: copy the
 * model (the closed-album model 0x1022 is swapped for 0x1023 past the last
 * owned page), hide the mesh of every empty slot and, on the two facing
 * pages of a turn (anim 1/2), of every card on the back side whose picture
 * is not loaded yet, then start the animation at `speed`.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002754(Page *pg, int anim, float speed) {
    AnimDef *def = &D_0000C2E0[anim];
    Model *src = func_000061F4(def->id);
    Model *mdl;
    int mask;
    int i;
    unsigned short c;

    if (pg->page >= (D_0000CB10.n_main + 0x11) / 0x12 && def->id == 0x1022) {
        src = func_000061F4(0x1023);
    }
    mdl = &pg->mdl;
    ehsys_memcpy(mdl, src, sizeof(Model));
    mask = 0;
    for (i = 0; i < 18; i++) {
        if (pg->cards[i] == 0) {
            mask |= 1 << (i < 9 ? i + 11 : i + 12);
        } else if (anim == 1 && i < 9) {
            c = (pg->cards + 9)[2 - i % 3 + i / 3 * 3];
            if (c != 0 && ehsys_B8DDA661(D_0000CB10.handle2, c, ehsys_B401BB25(c)) == 0) {
                mask |= 1 << (i + 11);
            }
        } else if (anim == 2 && i >= 9) {
            c = (pg->cards - 9)[2 - i % 3 + i / 3 * 3];
            if (c != 0 && ehsys_B8DDA661(D_0000CB10.handle2, c, ehsys_B401BB25(c)) == 0) {
                mask |= 1 << (i + 12);
            }
        }
    }
    ehsys_FF360587(mdl, mask);
    ehsys_9490E557(mdl, 0, func_00002E44);
    pg->mdl.frame = def->start;
    if (def->start == def->end) {
        pg->speed = 0.0f;
    } else {
        pg->speed = speed;
    }
    pg->anim = anim;
}

/* func_000029D8 — draw the album cover model (0x1020) and run its
 * open/close animation (D_0000CB10.f4b: 0 closed, 1 opening, 2 open,
 * 3 closing), sliding the view by D_0000CB10.f2524.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000029D8(void) {
    DrawParam p;
    float v[4];
    Model *mdl;

    ehsys_4F22C9AA(2);
    ehsys_memset(&p, 0, sizeof(p));
    p.f00 = 1;
    p.f04 = 1;
    ehsys_9EFDC9B9();
    mdl = func_000061F4(0x1020);
    ehsys_memset(v, 0, sizeof(v));
    if (D_0000CB10.f4b != 0) {
        v[0] = 1.0f;
    }
    ehsys_9F25AE2F(mdl, v);
    switch (D_0000CB10.f4b) {
    case 0:
        mdl->frame = 0.0f;
        break;
    case 1:
        D_0000CB10.f2524 += D_0000C118;
        if (mdl->frame >= 14.0f) {
            D_0000CB10.f4b = 2;
            mdl->frame = 14.0f;
        }
        break;
    case 2:
        mdl->frame = 14.0f;
        break;
    case 3:
        D_0000CB10.f2524 -= D_0000C114;
        if (mdl->frame >= 30.0f) {
            D_0000CB10.f4b = 0;
            mdl->frame = 0.0f;
        }
        break;
    }
    ehsys_BB22B13B(mdl, &p);
}

/* func_00002B68 — draw every page model, then the cards of the open page.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002B68(void) {
    Page *pg;

    for (pg = D_0000CB10.pages; pg != 0; pg = pg->next) {
        if (pg->anim != 0 && pg->anim != 7) {
            func_00002BE4(pg);
        }
    }
    if (D_0000CB10.mode == 3) {
        func_00002D54(D_0000CB10.idx);
    }
}

/* func_00002BE4 — draw one page model at its position (the cover slides
 * with its opening animation), advance its animation and chain to the
 * next row when it ends (closing the cover or the back page also
 * recycles the page stack).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002BE4(Page *pg) {
    Vec3 pos;
    DrawParam p;
    float v[4];
    Model *m;
    AnimDef *def;

    ehsys_4F22C9AA(2);
    ehsys_memset(&p, 0, sizeof(p));
    p.f00 = 1;
    p.f04 = 1;
    p.pg = pg;
    ehsys_9EFDC9B9();
    m = &pg->mdl;
    ehsys_memset(v, 0, sizeof(v));
    pos.x = pg->x;
    if (pg->anim == 3) {
        pos.x = pg->x + D_0000C110 * (m->frame / 14.0f);
    }
    pos.y = pg->y;
    pos.z = pg->z;
    ehsys_0089B865(&pos);
    v[0] = pg->speed;
    ehsys_9F25AE2F(m, v);
    ehsys_BB22B13B(m, &p);
    if (pg->speed > 0.0f) {
        def = &D_0000C2E0[pg->anim];
        if (m->frame > def->end) {
            switch (pg->anim) {
            case 4:
                func_00006848();
                break;
            case 5:
                func_00006900();
                break;
            }
            func_00002754(pg, def->next, pg->speed);
        }
    }
}

/* func_00002D54 — draw the selection cursor model (0x1024) over card
 * slot `idx` of the open page.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002D54(int idx) {
    Vec3 pos;
    DrawParam p;
    float v[4];
    Model *m;

    ehsys_4F22C9AA(2);
    ehsys_memset(&p, 0, sizeof(p));
    p.f00 = 1;
    p.f04 = 1;
    ehsys_9EFDC9B9();
    m = func_000061F4(0x1024);
    m->flags |= 2;
    ehsys_memset(v, 0, sizeof(v));
    pos.x = D_0000C360[idx].x;
    pos.y = 0.0f;
    pos.z = D_0000C360[idx].z;
    ehsys_0089B865(&pos);
    v[0] = 1.0f;
    ehsys_9F25AE2F(m, v);
    ehsys_BB22B13B(m, &p);
}

/* func_00002E24 — engine allocator hook (module heap).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void *func_00002E24(int size) {
    return ehsys_41AABF28(D_0000CB10.heap, size);
}

/* func_00002E34 — engine free hook (module heap).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002E34(void *p) {
    ehsys_20E340D9(D_0000CB10.heap, p);
}

/* func_00002E44 — page mesh callback: meshes 0x12..0x23 are the 18 card
 * slots. Texture the slot with the card's picture once it is loaded,
 * otherwise with a blinking placeholder (resources 0x100C/0x100D).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00002E44(int *ph, int a1, int mesh, int a3, DrawParam *ctx) {
    Page *pg = ctx->pg;
    int done = 0;
    unsigned short c;
    int h;
    void *res;

    if (mesh < 0x12 || mesh >= 0x24) {
        return 0;
    }
    c = pg->cards[mesh - 0x12];
    if (c == 0) {
        return 0;
    }
    h = *ph;
    if (ehsys_B8DDA661(D_0000CB10.handle2, c, ehsys_B401BB25(c)) == 0) {
        h = ehsys_F96953BE(h, D_0000CB10.handle2, c, ehsys_B401BB25(c), &done);
    }
    if (done == 0) {
        switch ((unsigned short) ((*ehsys_B2B9FEFA() >> 2) % 3)) {
        case 0:
            res = func_0000608C(0x100C);
            break;
        case 1:
            res = func_0000608C(0x100D);
            break;
        default:
            return 0;
        }
        h = ehsys_9EA6989A(h, res);
        h = ehsys_09794DF0(h, 0, 1);
    } else {
        h = ehsys_09794DF0(h, 0, 0);
    }
    h = ehsys_7B1755CF(h, 0.0f, 1.0f);
    *ph = ehsys_3E24ACE1(h, 1.0f, -1.0f);
    return 1;
}

/* func_00002FD0 — enter the album view.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002FD0(void) {
    func_00004488();
    func_00005238(8);
    D_0000CB10.f252C = 0;
    D_0000CB10.f2530 = 1.0f;
    D_0000CB10.f2534 = 1.0f;
}

/* func_00003010 — mode 3: album input. The analog stick scrolls the
 * card text, X leaves (mode 8), triangle opens the letter-jump dialog,
 * L/R turn the page, otherwise the cursor view (grid / side arrows /
 * bottom buttons) handles the d-pad.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003010(void) {
    Pad *pad = ehsys_F5E3080C();
    Scroll *sc = &D_0000CB10.scroll;
    int dy = 0;
    int v;
    int held;

    if (pad->stick_y > 0xC0) {
        dy = (pad->stick_y - 0xC0) * 6;
    }
    if (pad->stick_y < 0x40) {
        dy = (pad->stick_y - 0x40) * 6;
    }
    v = sc->pos + dy;
    if (v <= 0) {
        sc->pos = 0;
    } else if (v >= sc->max) {
        sc->pos = sc->max;
    } else {
        sc->pos = v;
    }
    held = pad->press;
    if (held & ehsys_get_button_code(1)) {
        D_0000CB10.mode = 8;
        ehsys_2AE3C0AD(0x94C5);
    } else if (held & 0x1000) {
        func_000013AC();
        ehsys_2AE3C0AD(0x94C7);
    } else if (pad->repeat & 0x100) {
        D_0000CB10.f252C = 0x100;
        func_00003C5C(D_0000CB10.f2534);
        ehsys_2AE3C0AD(0x94CC);
    } else if (pad->repeat & 0x200) {
        D_0000CB10.f252C = 0x200;
        func_00003D90(D_0000CB10.f2530);
        ehsys_2AE3C0AD(0x94CC);
    } else {
        switch (D_0000CB10.view) {
        case 0:
            func_000031C4();
            break;
        case 1:
            func_00003434();
            break;
        case 2:
            func_00003678();
            break;
        }
    }
    func_00004A54();
}

/* func_000031C4 — view 0: cursor on the 6x3 card grid of the open pages.
 * Leaving the grid sideways goes to the page-turn arrows (view 1), down
 * to the bottom buttons (view 2). O opens the card under the cursor
 * (mode 6), square jumps to the bottom buttons.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000031C4(void) {
    Pad *pad = ehsys_F5E3080C();
    int col = D_0000CB10.idx % 6;
    int row = D_0000CB10.idx / 6;
    int held;

    if (pad->repeat & 0x80) {
        ehsys_2AE3C0AD(0x94C3);
        if (col <= 0) {
            D_0000CB10.view = 1;
            D_0000CB10.f2529_hi = 0;
            return;
        }
        col--;
    }
    if (pad->repeat & 0x20) {
        ehsys_2AE3C0AD(0x94C3);
        if (col >= 5) {
            D_0000CB10.view = 1;
            D_0000CB10.f2529_hi = 1;
            return;
        }
        col++;
    }
    if (pad->repeat & 0x10) {
        if (row > 0) {
            row--;
            ehsys_2AE3C0AD(0x94C3);
        }
    }
    if (pad->repeat & 0x40) {
        ehsys_2AE3C0AD(0x94C3);
        if (row >= 2) {
            D_0000CB10.view = 2;
            D_0000CB10.f2529_lo = col;
            return;
        }
        row++;
    }
    D_0000CB10.idx = col + row * 6;
    func_00004C0C();
    func_00003C00();
    held = pad->press;
    if (held & ehsys_get_button_code(0)) {
        if ((unsigned short) func_00003AE8()) {
            D_0000CB10.mode = 6;
            ehsys_2AE3C0AD(0x94C4);
        }
    } else if (held & 0x8000) {
        D_0000CB10.view = 2;
        ehsys_2AE3C0AD(0x94C3);
    }
}

/* func_00003434 — view 1: cursor on a page-turn arrow (f2529_hi: 0 left,
 * 1 right).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003434(void) {
    Pad *pad = ehsys_F5E3080C();

    if (pad->repeat & 0x80) {
        if (D_0000CB10.f2529_hi == 0) {
            D_0000CB10.f252C = 0x80;
            func_00003C5C(D_0000CB10.f2534);
            ehsys_2AE3C0AD(0x94CC);
        } else {
            D_0000CB10.view = 0;
            ehsys_2AE3C0AD(0x94C3);
        }
    }
    if (pad->repeat & 0x20) {
        if (D_0000CB10.f2529_hi == 1) {
            D_0000CB10.f252C = 0x20;
            func_00003D90(D_0000CB10.f2530);
            ehsys_2AE3C0AD(0x94CC);
        } else {
            D_0000CB10.view = 0;
            ehsys_2AE3C0AD(0x94C3);
        }
    }
    if (pad->repeat & 0x10) {
        D_0000CB10.view = 0;
        ehsys_2AE3C0AD(0x94C3);
    }
    if (pad->repeat & 0x40) {
        if (D_0000CB10.f2529_hi == 0) {
            D_0000CB10.f2529_lo = 0;
        } else {
            D_0000CB10.f2529_lo = 5;
        }
        D_0000CB10.view = 2;
        ehsys_2AE3C0AD(0x94C3);
    }
    if (pad->press & ehsys_get_button_code(0)) {
        if (D_0000CB10.f2529_hi == 0) {
            func_00003C5C(D_0000CB10.f2534);
        } else {
            func_00003D90(D_0000CB10.f2530);
        }
        D_0000CB10.f252C = ehsys_get_button_code(0);
        ehsys_2AE3C0AD(0x94CC);
    }
    func_00003C00();
}

/* func_00003678 — view 2: the six bottom buttons (first page, previous
 * letter, previous page, next page, next letter, last page).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003678(void) {
    Pad *pad = ehsys_F5E3080C();
    int i;

    if (pad->repeat & 0x80) {
        D_0000CB10.f2529_lo = (D_0000CB10.f2529_lo + 5) % 6;
        ehsys_2AE3C0AD(0x94C3);
    }
    if (pad->repeat & 0x20) {
        D_0000CB10.f2529_lo = (D_0000CB10.f2529_lo + 1) % 6;
        ehsys_2AE3C0AD(0x94C3);
    }
    if (pad->repeat & 0x10) {
        D_0000CB10.view = 0;
        func_00005238(7);
        ehsys_2AE3C0AD(0x94C3);
    }
    if (pad->repeat & ehsys_get_button_code(0)) {
        ehsys_2AE3C0AD(0x94C4);
        switch (D_0000CB10.f2529_lo) {
        case 0:
            func_00004690(0);
            break;
        case 1:
            i = func_00004D64(1);
            if (i >= 0) {
                func_00004690(i);
            }
            break;
        case 2:
            func_00003C5C(D_0000CB10.f2534);
            break;
        case 3:
            func_00003D90(D_0000CB10.f2530);
            break;
        case 4:
            i = func_00004D64(0);
            if (i >= 0) {
                func_00004690(i);
            }
            break;
        case 5:
            func_00004690(D_0000CB10.n_total - 1);
            break;
        }
        if (D_0000CB10.mode == 4) {
            func_00005238(D_0000CB10.f2529_lo);
        }
    } else if (pad->press & 0x8000) {
        D_0000CB10.view = 0;
        func_00005238(7);
        ehsys_2AE3C0AD(0x94C3);
    }
    func_00003C00();
}

/* func_000038C8 — per frame in mode 3: queue the picture of every card
 * that is (or is about to be) visible, walking the page stack from the
 * back: the facing page of a turn shows its other side. Then flush the
 * queue (func_000064C0) and update the card text (func_000052C4).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000038C8(void) {
    Page *pg = D_0000CB10.pages;
    unsigned char a[9];
    unsigned char b[9];
    unsigned char *p;
    int i;
    unsigned short c;

    if (pg != 0 && D_0000CB10.mode == 3) {
        func_0000646C();
        ehsys_memset(a, 0, sizeof(a));
        ehsys_memset(b, 0, sizeof(b));
        while (pg->next != 0) {
            pg = pg->next;
        }
        for (;;) {
            switch (pg->anim) {
            case 1:
                for (i = 17; i >= 0; i--) {
                    c = pg->cards[i];
                    if (c != 0) {
                        if (i >= 9) {
                            p = &a[2 - i % 3 + (i / 3 * 3 - 9)];
                            if (*p == 0) {
                                *p = 1;
                                func_00006480(c);
                            }
                        } else {
                            a[i] = 1;
                        }
                    }
                }
                break;
            case 2:
                for (i = 0; i < 18; i++) {
                    c = pg->cards[i];
                    if (c != 0) {
                        if (i < 9) {
                            p = &b[i];
                            if (*p == 0) {
                                *p = 1;
                                func_00006480(c);
                            }
                        } else {
                            (b - 9)[2 - i % 3 + i / 3 * 3] = 1;
                        }
                    }
                }
                break;
            }
            if (pg == D_0000CB10.pages) {
                break;
            }
            pg = pg->prev;
        }
    }
    func_000064C0();
    func_000052C4();
}

/* func_00003AB4 — the page under the cursor: slot 3 (left) or 4 (right).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
Page *func_00003AB4(void) {
    return func_00006A48(D_0000CB10.idx % 6 < 3 ? 3 : 4);
}

/* func_00003AE8 — card id under the grid cursor (0 if none). The left
 * page shows the back half (cards 9..17) of its page.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00003AE8(void) {
    Page *pg = func_00003AB4();
    int col;
    int row;
    int i;

    if (pg == 0) {
        return 0;
    }
    col = D_0000CB10.idx % 6;
    row = D_0000CB10.idx / 6;
    if (col < 3) {
        i = col + (row * 3 + 9);
    } else {
        i = (row - 1) * 3;
        i += col;
    }
    return pg->cards[i];
}

/* func_00003B6C — album index of the card under the grid cursor.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
unsigned short func_00003B6C(void) {
    Page *pg = func_00003AB4();
    int col;
    int row;
    int i;

    if (pg == 0) {
        return 0;
    }
    col = D_0000CB10.idx % 6;
    row = D_0000CB10.idx / 6;
    if (col < 3) {
        i = col + (row * 3 + 9);
    } else {
        i = (row - 1) * 3;
        i += col;
    }
    return i + pg->page * 18;
}

/* func_00003C00 — when the card under the cursor changes, show its text
 * from the top. Returns 1 on a change.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00003C00(void) {
    int cur = D_0000CB10.scroll.f00;
    unsigned short c = func_00003AE8();

    if (c == 0) {
        return 0;
    }
    if (cur == c) {
        return 0;
    }
    D_0000CB10.scroll.f00 = c;
    D_0000CB10.scroll.pos = 0;
    return 1;
}

/* func_00003C5C — turn back one page: the left page (slot 3) plays its
 * turn animation and moves to the end of the draw list (mode 4); if the
 * back of the stack is now the last loaded page, load the previous one.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003C5C(float speed) {
    Page *pg;
    Page *prev;
    Page *next;
    int page = -1;
    int slot = 8;

    for (pg = D_0000CB10.pages; pg != 0; pg = pg->next) {
        if (pg->slot == 3) {
            prev = pg->prev;
            next = pg->next;
            func_00002754(pg, 5, speed);
            if (prev != 0 || next != 0) {
                if (prev != 0) {
                    prev->next = next;
                } else {
                    D_0000CB10.pages = next;
                }
                if (next != 0) {
                    next->prev = prev;
                }
                while (next != 0) {
                    prev = next;
                    next = next->next;
                }
                prev->next = pg;
                pg->prev = prev;
                pg->next = 0;
            }
            D_0000CB10.mode = 4;
            break;
        }
    }
    for (pg = D_0000CB10.pages; pg != 0; pg = pg->next) {
        if (pg->slot < slot) {
            slot = pg->slot;
            page = pg->page;
        }
    }
    if (slot == 1 && page > 0) {
        func_000065DC(0, 1, page - 1);
    }
}

/* func_00003D90 — turn forward one page (slot 4), loading the next page
 * behind the stack if there is one.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003D90(float speed) {
    Page *pg;
    Page *prev;
    Page *next;
    int page = -1;
    int slot = 0;

    for (pg = D_0000CB10.pages; pg != 0; pg = pg->next) {
        if (pg->slot == 4) {
            prev = pg->prev;
            next = pg->next;
            func_00002754(pg, 4, speed);
            if (prev != 0 || next != 0) {
                if (prev != 0) {
                    prev->next = next;
                } else {
                    D_0000CB10.pages = next;
                }
                if (next != 0) {
                    next->prev = prev;
                }
                while (next != 0) {
                    prev = next;
                    next = next->next;
                }
                prev->next = pg;
                pg->prev = prev;
                pg->next = 0;
            }
            D_0000CB10.mode = 4;
            break;
        }
    }
    for (pg = D_0000CB10.pages; pg != 0; pg = pg->next) {
        if (pg->slot > slot) {
            slot = pg->slot;
            page = pg->page;
        }
    }
    if (slot == 6 && page < (D_0000CB10.n_total + 0x11) / 0x12 - 1) {
        func_000065DC(7, 2, page + 1);
    }
}

/* func_00003EE4 — mode 4: a page is turning. The turning page drags the
 * other pages along; when it is done, a multi-page jump (D_0000CB10.jumping)
 * turns the next page until jump_page is open, O skips the animation
 * (rebuilding the stack at the target) and X cancels the jump.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003EE4(void) {
    Page *pg = D_0000CB10.pages;
    Page *moving = 0;
    Page *p2;
    Pad *pad;
    int base;
    int i;
    float dx;
    float dz;

    func_00004A54();
    if (D_0000CB10.jumping) {
        pad = ehsys_F5E3080C();
        if (pad->press & ehsys_get_button_code(0)) {
            func_00006800();
            pg = D_0000CB10.pages;
            if (D_0000CB10.jump_idx % 6 < 3) {
                base = D_0000CB10.jump_page - 3;
            } else {
                base = D_0000CB10.jump_page - 4;
            }
            for (i = 1; i < 7; i++) {
                if (base + i >= 0 && base + i < (D_0000CB10.n_total + 0x11) / 0x12) {
                    func_000065DC(i, i < 4 ? 1 : 2, base + i);
                }
            }
            D_0000CB10.jumping = 0;
            D_0000CB10.idx = D_0000CB10.jump_idx;
            ehsys_2AE3C0AD(0x94C4);
        } else {
            pad = ehsys_F5E3080C();
            if (pad->press & ehsys_get_button_code(1)) {
                D_0000CB10.jumping = 0;
                ehsys_2AE3C0AD(0x94C5);
            }
        }
    }
    for (; pg != 0; pg = pg->next) {
        switch (pg->anim) {
        case 4:
            moving = pg;
            if (!D_0000CB10.jumping) {
                pg->speed = D_0000CB10.f2530;
            }
            break;
        case 5:
            moving = pg;
            if (!D_0000CB10.jumping) {
                pg->speed = D_0000CB10.f2534;
            }
            break;
        }
    }
    if (moving == 0) {
        if (D_0000CB10.jumping) {
            if (D_0000CB10.jump_idx % 6 < 3) {
                p2 = func_00006A48(3);
            } else {
                p2 = func_00006A48(4);
            }
            if (p2 == 0) {
                D_0000CB10.jumping = 0;
                D_0000CB10.idx = D_0000CB10.jump_idx;
                func_00004C0C();
                D_0000CB10.mode = 3;
            } else if (p2->page < D_0000CB10.jump_page) {
                func_00003D90(D_0000CB10.jump_speed);
                ehsys_2AE3C0AD(0x94CC);
            } else if (D_0000CB10.jump_page < p2->page) {
                func_00003C5C(D_0000CB10.jump_speed);
                ehsys_2AE3C0AD(0x94CC);
            } else {
                D_0000CB10.jumping = 0;
                D_0000CB10.idx = D_0000CB10.jump_idx;
                func_00004C0C();
                D_0000CB10.mode = 3;
            }
        } else {
            D_0000CB10.mode = 3;
            func_00004C0C();
        }
        return;
    }
    pg = D_0000CB10.pages;
    dx = D_0000C16C * moving->speed / 25.0f;
    dz = D_0000C168 * moving->speed / 25.0f;
    while (pg != 0) {
        if (pg != moving) {
            switch (moving->anim) {
            case 4:
                if (pg->slot < 4) {
                    pg->y -= dz;
                } else {
                    pg->y += dz;
                }
                if (pg->slot > 1 && pg->slot < 7) {
                    pg->x -= dx;
                }
                break;
            case 5:
                if (pg->slot < 4) {
                    pg->y += dz;
                } else {
                    pg->y -= dz;
                }
                if (pg->slot > 0 && pg->slot < 6) {
                    pg->x += dx;
                }
                break;
            }
        }
        pg = pg->next;
    }
}

/* func_000043E8 — 1 if `id` is in the extra-card list D_0000C3F0.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000043E8(unsigned short id) {
    int i;

    for (i = 0; D_0000C3F0[i] >= 0; i++) {
        if (id == D_0000C3F0[i]) {
            return 1;
        }
    }
    return 0;
}

/* func_0000442C — put extra card `id` at its fixed slot in `out`; returns
 * the new length of the extra part.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_0000442C(unsigned short *out, int id, int n) {
    int i;

    for (i = 0; D_0000C3F0[i] >= 0; i++) {
        if (id == D_0000C3F0[i]) {
            out[i] = id;
            if (i > n - 1) {
                n = i + 1;
            }
            break;
        }
    }
    return n;
}

/* func_00004488 — build the album: every valid, collectible card in
 * name order gets a slot (0 while not owned), the cards of D_0000C3F0 go
 * to an extra part after the last full page (owned ones only), and the
 * completion is counted in per mille.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00004488(void) {
    unsigned short order[0x15EF];
    unsigned short extra[0x1F4];
    int k;
    int i;
    int n;
    int nextra;
    int owned;
    unsigned short c;

    for (k = 0; k < sizeof(order) / sizeof(order[0]); k++) {
        order[k] = 0;
    }
    for (k = 0; k < sizeof(extra) / sizeof(extra[0]); k++) {
        extra[k] = 0;
    }
    for (i = 0xE10; i < 0x23FF; i++) {
        n = ehsys_93EACD81(i);
        if (n > 0) {
            order[n - 1] = i;
        }
    }
    n = 0;
    nextra = 0;
    owned = 0;
    ehsys_memset(D_0000CB10.cards, 0, sizeof(D_0000CB10.cards));
    for (i = 0; i < 0x15EF; i++) {
        c = order[i];
        if (c == 0) {
            continue;
        }
        if (!ehsys_28F7224B(c)) {
            continue;
        }
        if (ehsys_67312719(c)) {
            continue;
        }
        if (func_000043E8(c)) {
            if (ehsys_7B47DFE9(c)) {
                nextra = func_0000442C(extra, c, nextra);
            }
        } else if (!ehsys_7B47DFE9(c)) {
            D_0000CB10.cards[n] = 0;
            n++;
        } else {
            D_0000CB10.cards[n] = c;
            n++;
            owned++;
        }
    }
    D_0000CB10.n_main = n;
    D_0000CB10.permille = owned * 1000 / n;
    if (nextra > 0) {
        n = (n + 0x11) / 0x12 * 0x12;
        ehsys_memcpy(&D_0000CB10.cards[n], extra, nextra * 2);
    }
    D_0000CB10.n_total = n + nextra;
}

/* func_00004690 — show album slot `idx`: move the cursor there if its
 * page is open, otherwise start a multi-page jump (faster the farther).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00004690(int idx) {
    int page;
    int r;
    int pos;
    int cur;
    int d;
    Page *pg;

    if (idx < 0) {
        return 0;
    }
    if (idx >= D_0000CB10.n_total) {
        return 0;
    }
    page = idx / 18;
    r = idx % 18;
    if (r < 9) {
        pos = r % 3 + r / 3 * 6 + 3;
        pg = func_00006A48(4);
        if (pg != 0) {
            cur = pg->page;
        } else {
            cur = (D_0000CB10.n_total + 0x11) / 0x12;
        }
    } else {
        r -= 9;
        pos = r + r / 3 * 3;
        pg = func_00006A48(3);
        if (pg != 0) {
            cur = pg->page;
        } else {
            cur = -1;
        }
    }
    if (page == cur) {
        D_0000CB10.idx = pos;
        func_00004C0C();
        return 1;
    }
    D_0000CB10.jumping = 1;
    D_0000CB10.jump_idx = pos;
    D_0000CB10.jump_page = page;
    if (page < cur) {
        d = cur - page;
        if (d > 0x32) {
            D_0000CB10.jump_speed = 12.5f;
        } else if (d > 0x14) {
            D_0000CB10.jump_speed = 6.25f;
        } else {
            D_0000CB10.jump_speed = 3.125f;
        }
        func_00003C5C(D_0000CB10.jump_speed);
    } else if (cur < page) {
        d = page - cur;
        if (d > 0x32) {
            D_0000CB10.jump_speed = 12.5f;
        } else if (d > 0x14) {
            D_0000CB10.jump_speed = 6.25f;
        } else {
            D_0000CB10.jump_speed = 3.125f;
        }
        func_00003D90(D_0000CB10.jump_speed);
    }
    return 1;
}

/* func_000048BC — mode 6: fade out, open the engine's card detail view
 * for the card under the cursor, wait for it to close, fade back in.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000048BC(void) {
    switch (D_0000CB10.step) {
    case 0:
        ehsys_5998BFD7(1, 0xFF000000, 1);
        D_0000CB10.step++;
    case 1:
        if (!ehsys_450CD6C5()) {
            break;
        }
        ehsys_50D6F57B((unsigned short) func_00003AE8(), func_0000608C(0x1006), func_0000608C(0x1009));
        ehsys_6D6C9B94();
        ehsys_D1A604C7(func_0000608C(0x100A));
        D_0000CB10.f2540 = 1;
        D_0000CB10.step++;
        break;
    case 2:
        if (func_00005744() == 1) {
            break;
        }
        if (!ehsys_B779C0DF()) {
            break;
        }
        D_0000CB10.f2540 = 0;
        ehsys_D470D0B2(1, 0xFF000000, 1);
        D_0000CB10.step++;
    case 3:
        if (!ehsys_450CD6C5()) {
            break;
        }
        D_0000CB10.step = 0;
        D_0000CB10.mode = 3;
        break;
    }
}

/* func_00004A54 — page-turn acceleration: while the turn button is held
 * (D_0000CB10.f252C), every auto-repeat speeds the turn up by 0.5 (max
 * 5.0); releasing it resets both speeds to 1.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00004A54(void) {
    Pad *pad = ehsys_F5E3080C();

    if (D_0000CB10.f252C == 0) {
        return;
    }
    if (D_0000CB10.f252C & pad->f0) {
        if (D_0000CB10.f252C & pad->repeat) {
            if (D_0000CB10.f252C == 0x80 || D_0000CB10.f252C == 0x100) {
                D_0000CB10.f2534 += 0.5f;
                if (D_0000CB10.f2534 > 5.0f) {
                    D_0000CB10.f2534 = 5.0f;
                }
            }
            if (D_0000CB10.f252C == 0x20 || D_0000CB10.f252C == 0x200) {
                D_0000CB10.f2530 += 0.5f;
                if (D_0000CB10.f2530 > 5.0f) {
                    D_0000CB10.f2530 = 5.0f;
                }
            }
            if (D_0000CB10.f252C == ehsys_get_button_code(0)) {
                if (D_0000CB10.f2529_hi == 0) {
                    D_0000CB10.f2534 += 0.5f;
                    if (D_0000CB10.f2534 > 5.0f) {
                        D_0000CB10.f2534 = 5.0f;
                    }
                } else {
                    D_0000CB10.f2530 += 0.5f;
                    if (D_0000CB10.f2530 > 5.0f) {
                        D_0000CB10.f2530 = 5.0f;
                    }
                }
            }
        }
    } else {
        D_0000CB10.f252C = 0;
        D_0000CB10.f2530 = 1.0f;
        D_0000CB10.f2534 = 1.0f;
    }
}

/* func_00004C0C — if the page under the cursor is missing (first/last
 * page), move the cursor to the same row of the other page.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00004C0C(void) {
    if (func_00003AB4() != 0) {
        return;
    }
    if (D_0000CB10.idx % 6 < 3) {
        if (func_00006A48(4) == 0) {
            return;
        }
        D_0000CB10.idx += 3 - D_0000CB10.idx % 3;
        if (D_0000CB10.view == 1) {
            D_0000CB10.view = 0;
        }
    } else {
        if (func_00006A48(3) == 0) {
            return;
        }
        D_0000CB10.idx -= D_0000CB10.idx % 3 + 1;
        if (D_0000CB10.view == 1) {
            D_0000CB10.view = 0;
        }
    }
}

/* func_00004D64 — album index of the first card of the previous
 * (dir != 0) or next (dir == 0) name-initial group, -1 if none / no move.
 * Both directions treat the extra part after the last main page as one
 * more group. Two of the scan loops are buggy in the original (one never
 * advances, one runs the wrong way) and are kept as they are.
 * NONMATCHING: right size (309 words) and control flow; the target gives
 * k $s1, total/2nd group $s2, i $s3 where this gives total $s1, i $s2,
 * k $s3 (54 words differ, registers only, plus one `bnel v0,s4` operand
 * order in the last loop). All 120 declaration orders give the same
 * allocation; decomp-permuter (~1100 iterations) got from 810 to 620. */
int func_00004D64(int dir) {
    int cur = (unsigned short) func_00003B6C();
    int total = D_0000CB10.n_total;
    int g = -1;
    int k = 0;
    int i = (D_0000CB10.n_main + 8) / 9 * 9;

    if (dir != 0) {
        if (cur <= 0) {
            return g;
        }
        if (cur >= i) {
            if (cur == i) {
                i = D_0000CB10.n_main - 1;
                cur = ehsys_4E722BBD(D_0000CB10.cards[i]);
                for (k = i; --i >= 0;) {
                    if (D_0000CB10.cards[i] != 0) {
                        if (ehsys_4E722BBD(D_0000CB10.cards[i]) < cur) {
                            i = k;
                            break;
                        }
                        k = i;
                    }
                }
            } else {
                while (i < total) {
                    if (D_0000CB10.cards[i] != 0) {
                        break;
                    }
                }
            }
            if (i == (unsigned short) func_00003B6C()) {
                return -1;
            }
            return i;
        }
        if (D_0000CB10.cards[cur] == 0) {
            i = cur;
            while (--i >= 0) {
                if (D_0000CB10.cards[i] != 0) {
                    cur = i;
                    break;
                }
            }
            if (cur == (unsigned short) func_00003B6C()) {
                return -1;
            }
            return cur;
        }
        total = ehsys_4E722BBD(D_0000CB10.cards[cur]);
        for (i = 0; i < D_0000CB10.n_total; i++) {
            if (D_0000CB10.cards[i] != 0 && total == ehsys_4E722BBD(D_0000CB10.cards[i])) {
                k = 1;
                break;
            }
        }
        if (total <= 1 || k != 0) {
            for (cur = 0; cur < D_0000CB10.n_total; cur++) {
                if (D_0000CB10.cards[cur] != 0) {
                    break;
                }
            }
        } else {
            k = -1;
            while (--cur >= 0) {
                if (D_0000CB10.cards[cur] != 0) {
                    if (ehsys_4E722BBD(D_0000CB10.cards[cur]) <= total - 2) {
                        cur = k;
                        break;
                    }
                    k = cur;
                }
            }
            if (cur < 0) {
                if (k < 0) {
                    cur = 0;
                } else {
                    cur = k;
                }
            }
        }
        if ((unsigned short) func_00003B6C() < cur) {
            cur = (unsigned short) func_00003B6C();
        }
    } else {
        if (cur >= i) {
            cur = total - 1;
            if (cur == (unsigned short) func_00003B6C()) {
                return g;
            }
            return cur;
        }
        if (D_0000CB10.cards[cur] != 0) {
            g = ehsys_4E722BBD(D_0000CB10.cards[cur]);
        }
        if (D_0000CB10.cards[cur] == 0) {
            for (i = cur; i < D_0000CB10.n_total; i++) {
                if (D_0000CB10.cards[i] != 0) {
                    cur = i;
                    break;
                }
            }
            if (cur == (unsigned short) func_00003B6C()) {
                return -1;
            }
            return cur;
        }
        for (i = D_0000CB10.n_main - 1; i >= 0; i++) {
            if (D_0000CB10.cards[i] != 0 && g == ehsys_4E722BBD(D_0000CB10.cards[i])) {
                k = 1;
                break;
            }
        }
        if (g == 0x1A || k != 0) {
            for (i = D_0000CB10.n_main - 1; i >= 0; i--) {
                if (D_0000CB10.cards[i] != 0) {
                    cur = i;
                    break;
                }
            }
            if (cur == (unsigned short) func_00003B6C()) {
                return -1;
            }
            return cur;
        }
        for (cur++; cur < total; cur++) {
            if (cur >= D_0000CB10.n_main) {
                g = -1;
            }
            if (D_0000CB10.cards[cur] != 0 && g != ehsys_4E722BBD(D_0000CB10.cards[cur])) {
                break;
            }
        }
        if (cur >= total) {
            return -1;
        }
    }
    if (cur == (unsigned short) func_00003B6C()) {
        return -1;
    }
    return cur;
}

/* func_00005238 — play animation `n` of the frame model (0x1010):
 * 0..5 highlight a bottom button, 6 and 7 the arrows, 8 the idle loop.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00005238(int n) {
    void *res = func_0000608C(0x1010);
    int a;
    int b;

    if (res == 0) {
        return;
    }
    switch (n) {
    case 6:
        a = 0;
        b = 9;
        break;
    case 7:
        a = b = 9;
        break;
    case 8:
        b = 0;
        a = 0;
        break;
    default:
        a = n * 5 + 0xA;
        b = a + 4;
        break;
    }
    ehsys_831C5769(res, a, b + 1);
}

/* func_000052C4 — hold the frame model's animation on its last frame.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000052C4(void) {
    void *res = func_0000608C(0x1010);
    int first;
    int last;
    int cur;

    if (res == 0) {
        return;
    }
    ehsys_B8770B92(res, &first, &last);
    cur = ehsys_558B2DCB(res);
    if (last > 0 && cur >= last - 1) {
        ehsys_831C5769(res, last - 1, last);
    }
}

/* func_00005330 — mode 0, first step: reset the zoom state and set up
 * the JPEG decoder and the card-picture archive. Returns 1 once the
 * archive is open.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00005330(void) {
    Fx *fx = &D_0000CB10.fx;
    int i;

    if (fx->jpeg == 0) {
        ehsys_memset(fx, 0, sizeof(Fx));
        fx->jpeg = 1;
        fx->on = 0;
        for (i = 0; i < 9; i++) {
            fx->idx[i * 2] = i;
            fx->idx[i * 2 + 1] = i + 9;
        }
        sceJpegInitMJpeg();
        sceJpegCreateMJpeg(0x100, 0x100);
        fx->jbuf = ehsys_41AABF28(D_0000CB10.heap, 0x45000);
        fx->handle = ehsys_D80C7D36(fx->jbuf, 0x45000, 0);
    } else if (ehsys_2717C28C(fx->handle)) {
        return 1;
    }
    return 0;
}

/* func_0000541C — mode 8, first step: shut the JPEG decoder down and
 * close the card-picture archive; 1 once it is closed.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_0000541C(void) {
    Fx *fx = &D_0000CB10.fx;

    if (fx->jpeg == 1) {
        fx->jpeg = 0;
        func_0000555C();
        sceJpegDeleteMJpeg();
        sceJpegFinishMJpeg();
        ehsys_20E340D9(D_0000CB10.heap, fx->jbuf);
        ehsys_5DBE93DB(fx->handle);
    } else if (ehsys_101103B3(fx->handle)) {
        return 1;
    }
    return 0;
}

/* func_000054B8 — start zooming into the card under the cursor.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000054B8(void) {
    Fx *fx = &D_0000CB10.fx;
    int a;
    int b;

    fx->on = 1;
    ehsys_B46025DF(D_0000CB10.heap, &a, &b);
    fx->tex = ehsys_41AABF28(D_0000CB10.heap, 0x40000);
    fx->step = 0;
    fx->fA76 = 0;
    fx->card = (unsigned short) func_00003AE8();
    fx->card_sub = ehsys_B401BB25(fx->card);
    fx->dx = -0xB5;
    fx->dy = -0x3B;
    fx->size = -0xB2;
    fx->angle = 0.0f;
    if (ehsys_06A1D1D8() > 0) {
        ehsys_F6905EFA(0, 600);
    }
}

/* func_0000555C — end the card zoom and free the picture.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_0000555C(void) {
    Fx *fx = &D_0000CB10.fx;

    fx->on = 0;
    fx->active = 0;
    if (fx->tex != 0) {
        ehsys_20E340D9(D_0000CB10.heap, fx->tex);
        fx->tex = 0;
    }
}

/* func_000055B8 — decode the zoomed card's picture (JPEG from the card
 * archive -> YCbCr -> 256x256 texture). Returns 1 when it is ready.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000055B8(void) {
    Fx *fx = &D_0000CB10.fx;
    void *data;
    int size;

    if (fx->active == 1) {
        return 1;
    }
    switch (fx->step) {
    case 0:
        if (ehsys_06A1D1D8() > 0 && ehsys_57724B02()) {
            return 0;
        }
        data = ehsys_8C5B148E(fx->handle, fx->card, fx->card_sub);
        if (data == 0) {
            return 0;
        }
        size = ehsys_C01F381E(fx->handle, fx->card, fx->card_sub);
        fx->ycc_size = sceJpegGetOutputInfo(data, size, &fx->info, 1);
        fx->ycc_raw = ehsys_41AABF28(D_0000CB10.heap, fx->ycc_size + 0x40);
        fx->ycc = (void *) (((int) fx->ycc_raw + 0x3F) & ~0x3F);
        sceJpegDecodeMJpegYCbCr(data, size, fx->ycc, fx->ycc_size, 1);
        fx->step++;
        if (ehsys_06A1D1D8() > 0) {
            ehsys_F6905EFA(0x20, 0x4B0);
        }
        return 0;
    case 1:
        sceJpegCsc(fx->tex, fx->ycc, 0x1000100, 0x100, fx->info);
        ehsys_20E340D9(D_0000CB10.heap, fx->ycc_raw);
        fx->active = 1;
        return 1;
    }
    return 0;
}

/* func_00005744 — card zoom (mode 6 step 2): O on a card starts it;
 * then fade in, zoom in, let the player pan (d-pad), zoom (stick) and
 * rotate (L/R; triangle resets), X zooms back out. 1 while it runs.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00005744(void) {
    Fx *fx = &D_0000CB10.fx;
    Pad *pad = ehsys_F5E3080C();

    if (fx->on == 0) {
        if ((pad->press & ehsys_get_button_code(0)) && ehsys_3E4FC8A6() == 1) {
            func_000054B8();
        } else {
            return 0;
        }
    }
    switch (fx->on) {
    case 0:
        break;
    case 1:
        func_00005AB8(1);
        if (fx->fade < 10) {
            fx->fade++;
        } else {
            fx->on = 2;
        }
        break;
    case 2:
        if (func_00005AB8(1) == 1) {
            fx->on = 3;
        }
        break;
    case 3:
        if (pad->press & ehsys_get_button_code(1)) {
            fx->sdx = fx->dx;
            fx->sdy = fx->dy;
            fx->ssize = fx->size;
            fx->on = 4;
            fx->sangle = fx->angle;
            if (ehsys_06A1D1D8() > 0) {
                ehsys_F6905EFA(0x7F, 600);
            }
            break;
        }
        if (pad->stick_y > 0xC0) {
            fx->size -= 4;
            if (fx->size < -0xC8) {
                fx->size = -0xC8;
            }
        } else if (pad->stick_y < 0x40) {
            fx->size += 4;
            if (fx->size > 0xA0) {
                fx->size = 0xA0;
            }
        }
        if (pad->f0 & 0x80) {
            fx->dx -= 2;
            if (fx->dx < -0xF0) {
                fx->dx = -0xF0;
            }
        } else if (pad->f0 & 0x20) {
            fx->dx += 2;
            if (fx->dx > 0xF0) {
                fx->dx = 0xF0;
            }
        }
        if (pad->f0 & 0x10) {
            fx->dy -= 2;
            if (fx->dy < -0x88) {
                fx->dy = -0x88;
            }
        } else if (pad->f0 & 0x40) {
            fx->dy += 2;
            if (fx->dy > 0x88) {
                fx->dy = 0x88;
            }
        }
        if (pad->f0 & 0x100) {
            fx->angle -= D_0000C160[1];
            if (fx->angle < D_0000C160[0]) {
                fx->angle += D_0000C15C;
            }
        } else if (pad->f0 & 0x200) {
            fx->angle += D_0000C160[1];
            if (fx->angle > D_0000C158) {
                fx->angle -= D_0000C15C;
            }
        }
        if (pad->press & 0x1000) {
            fx->dx = 0;
            fx->dy = 0;
            fx->size = 0;
            fx->angle = 0.0f;
        }
        break;
    case 4:
        if (fx->fade > 0) {
            fx->fade--;
        }
        if (func_00005AB8(0) == 1 && fx->fade == 0) {
            fx->on = 5;
        }
        break;
    case 5:
        func_0000555C();
        return 0;
    default:
        return 0;
    }
    return 1;
}

/* .rodata layout shim: the shipped .data holds 0x20 bytes (seven float
 * constants and the module_sdk_version) between jtbl_0000C13C (func_00005744)
 * and jtbl_0000C174 (func_00006BB4); these words keep the last jump table
 * at its address. Never read (the code uses the D_0000C154.. externs). */
const unsigned int rodata_shim_C154[8] = {
    0x3FC90FDB, /* pi/2 = D_0000C154 */
    0x40490FDB, /* pi = D_0000C158 */
    0x40C90FDB, /* 2 pi = D_0000C15C */
    0xC0490FDB, /* -pi = D_0000C160 */
    0x3D0EFA35, /* 2 degrees = D_0000C164 */
    0x3D23D70A, /* 0.04f = D_0000C168 */
    0x3D75C28F, /* 0.06f = D_0000C16C */
    0x06020010, /* module_sdk_version */
};

/* func_00005AB8 — zoom animation step, in (1) or out (0), eased with
 * 1 - cos(pi/2 * t). Returns 1 at the end.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00005AB8(int in) {
    Fx *fx = &D_0000CB10.fx;
    float c;
    float t;

    if (in == 1) {
        if (!func_000055B8()) {
            return 0;
        }
        if (fx->fade2 < 10) {
            t = 1.0f - ehsys_A38AA0E9(D_0000C154 * fx->fade2 / 10.0f);
            fx->dx = -181.0f * t;
            fx->dy = -59.0f * t;
            fx->size = -178.0f * t;
            fx->fade2++;
            return 0;
        }
        fx->dx = 0;
        fx->dy = 0;
        fx->size = 0;
        return 1;
    }
    if (fx->fade2 > 0) {
        c = ehsys_A38AA0E9(D_0000C154 * fx->fade2 / 10.0f);
        t = 1.0f - c;
        fx->dx = -181.0f * t + fx->sdx * c;
        fx->dy = -59.0f * t + fx->sdy * c;
        fx->size = -178.0f * t + fx->ssize * c;
        fx->angle = fx->sangle * c;
        fx->fade2--;
        return 0;
    }
    fx->dx = -0xB5;
    fx->dy = -0x3B;
    fx->angle = 0.0f;
    fx->size = -0xB2;
    return 1;
}

/* func_00005C90 — reset the resource loader and sort both resource
 * tables by id (searched with func_000063F4).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00005C90(void) {
    ehsys_memset(&D_0000CAD0, 0, sizeof(D_0000CAD0));
    D_0000CAD0.ready = 1;
    ehsys_qsort(D_0000C694, 0x21, sizeof(ResEntry), func_000063E4);
    ehsys_qsort(D_0000C96C, 5, sizeof(MdlEntry), func_000063E4);
}

/* func_00005D10 — mode 0 loader, one step per frame: open the work and
 * card-picture buffers and request database/cardalbum.ehp, wait, then
 * hand the card-drawing resources to the engine. 1 when done.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00005D10(void) {
    ResParam p;

    if (D_0000CAD0.ready) {
        switch (D_0000CAD0.step) {
        case 0:
            D_0000CAFC = ehsys_41AABF28(D_0000CB10.heap, 0x74000);
            D_0000CB10.handle = ehsys_291D6262(D_0000CAFC, 0x74000, 0);
            D_0000CB00 = ehsys_41AABF28(D_0000CB10.heap, 0x3F8000);
            D_0000CB10.handle2 = ehsys_47719795(D_0000CB00, 0x3F8000, 0);
            func_00005FDC();
            break;
        case 1:
            if (ehsys_2717C28C(D_0000CB10.handle) && ehsys_2717C28C(D_0000CB10.handle2)) {
                D_0000CAD0.step++;
            }
            break;
        case 2:
            ehsys_memset(&p, 0, sizeof(p));
            p.res[0] = func_0000608C(0x1004);
            p.res[1] = func_0000608C(0x1005);
            p.res[2] = func_0000608C(0x1011);
            p.res[3] = func_0000608C(0x1007);
            p.res[4] = func_0000608C(0x1008);
            p.handle = D_0000CB10.handle;
            ehsys_1CC5878C(&p);
            D_0000CB10.scroll.f00 = -1;
            D_0000CB10.scroll.pos = 0;
            D_0000CAD0.step++;
            break;
        default:
            D_0000CAD0.step = 0;
            return 1;
        }
    }
    return 0;
}

/* func_00005EFC — free everything the loader allocated.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00005EFC(void) {
    func_00005F04();
}

/* func_00005F04 — free the buffers, the models and the archive.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00005F04(void) {
    int i;
    MdlEntry *e;

    if (D_0000CAFC != 0) {
        ehsys_20E340D9(D_0000CB10.heap, D_0000CAFC);
        D_0000CAFC = 0;
    }
    if (D_0000CB00 != 0) {
        ehsys_20E340D9(D_0000CB10.heap, D_0000CB00);
        D_0000CB00 = 0;
    }
    for (i = 0; i < 5U; i++) {
        e = &D_0000C96C[i];
        if (e->data != 0) {
            ehsys_60BA0722(e->data);
            e->data = 0;
        }
    }
    if (D_0000CAF8 != 0) {
        ehsys_20E340D9(D_0000CB10.heap, D_0000CAF8);
        D_0000CAF8 = 0;
    }
}

/* func_00005FDC — return database/cardalbum.ehp, starting its
 * asynchronous load (completion: func_00006354) on the first call.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void *func_00005FDC(void) {
    int size;

    if (D_0000CAF8 != 0) {
        return D_0000CAF8;
    }
    if (D_0000CAD0.ready) {
        D_0000CAD0.ready = 0;
        size = ehsys_D2A768F4(3, D_0000C9D0);
        D_0000CAF8 = ehsys_41AABF28(D_0000CB10.heap, size);
        ehsys_8288EEBC(3, D_0000C9D0, D_0000CAF8, size, func_00006354, D_0000CAF8);
    }
    return 0;
}

/* func_0000608C — get resource `id`, loading it from the archive on
 * first use. The file name takes the language letter from "jegfis";
 * resources 0x1031/0x1032 always use the German ('g') file of "jagfisb".
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void *func_0000608C(unsigned short id) {
    char path[0x100];
    LangCodes6 lang;
    LangCodes7 lang2;
    ResEntry *e;
    void *arc;
    int idx = func_000063F4(id, D_0000C694, 0x21, sizeof(ResEntry));

    if (idx < 0) {
        return 0;
    }
    e = &D_0000C694[idx];
    if (e->data != 0) {
        return e->data;
    }
    arc = func_00005FDC();
    lang = *(LangCodes6 *) D_0000C9E8;
    lang2 = *(LangCodes7 *) D_0000C9F0;
    if (arc == 0) {
        return 0;
    }
    if ((unsigned int) (id - 0x1031) < 2) {
        ehsys_sprintf(path, e->fmt, lang2.c[2]);
    } else {
        ehsys_sprintf(path, e->fmt, lang.c[ehsys_get_language()]);
    }
    e->data = ehsys_C6C15111(arc, path, 0);
    if (e->aux != 0) {
        ehsys_FDAFCF3A(e->data, ehsys_C6C15111(arc, e->aux, 0));
    }
    return e->data;
}

/* func_000061F4 — get model `id` (album cover/pages/cursor), building it
 * from its .tms (+ optional .tmt animation, + optional extra file) on
 * first use.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
Model *func_000061F4(unsigned short id) {
    MdlEntry *e;
    void *arc;
    void *tms;
    void *tmt;
    void *extra;
    int idx = func_000063F4(id, D_0000C96C, 5, sizeof(MdlEntry));

    if (idx < 0) {
        return 0;
    }
    e = &D_0000C96C[idx];
    if (e->data != 0) {
        return e->data;
    }
    arc = func_00005FDC();
    tmt = 0;
    extra = 0;
    if (arc == 0) {
        return 0;
    }
    tms = ehsys_C6C15111(arc, e->tms, 0);
    if (e->tmt != 0) {
        tmt = ehsys_C6C15111(arc, e->tmt, 0);
    }
    if (e->extra != 0) {
        extra = ehsys_C6C15111(arc, e->extra, 0);
    }
    if (tmt == 0) {
        e->data = ehsys_3765ED01(tms, 0);
    } else if (*(int *) tmt != 0x544D4754) {
        e->data = ehsys_3765ED01(tms, tmt);
    } else {
        e->data = ehsys_3765ED01(tms, 0);
        ehsys_D0E95BD8(e->data, tmt);
    }
    if (extra != 0) {
        ehsys_CB9CCF0B(e->data, extra);
    }
    return e->data;
}

/* func_00006354 — load-completion callback of func_00005FDC.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00006354(void *buf, int size) {
    int ok = size > 0;

    if (ok) {
        if (buf != 0) {
            ehsys_4B0DABFA(0, buf);
        }
        D_0000CAD0.step++;
    }
    D_0000CAD0.ready = 1;
    return ok;
}

/* func_000063E4 — qsort comparator on the u16 id at the head of a record.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000063E4(unsigned short *a, unsigned short *b) {
    return *a - *b;
}

/* func_000063F4 — binary search by id in a table of `stride`-byte
 * records; -1 if absent.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000063F4(unsigned short id, void *tbl, int n, int stride) {
    int half = stride / 2;
    int lo;
    int hi;
    int mid;
    unsigned short v;

    hi = n - 1;
    lo = 0;
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        v = ((unsigned short *) tbl)[mid * half];
        if (id == v) {
            return mid;
        }
        if (v < id) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return -1;
}

/* func_0000646C — empty the picture request queue.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_0000646C(void) {
    ehsys_memset(D_0000CAD0.queue, 0, sizeof(D_0000CAD0.queue));
}

/* func_00006480 — queue a card picture request (dropped when full).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00006480(unsigned short card) {
    int i;

    for (i = 0; i < 18; i++) {
        if (D_0000CAD0.queue[i] == 0) {
            D_0000CAD0.queue[i] = card;
            return;
        }
    }
}

/* func_000064C0 — request the first queued picture that is not loaded
 * yet (one per frame) and drop the entries before it.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000064C0(void) {
    int i;
    int j;
    unsigned short c;
    int h;
    int tmp;

    for (i = 0; i < 18; i++) {
        c = D_0000CAD0.queue[i];
        if (c == 0) {
            break;
        }
        if (ehsys_B8DDA661(D_0000CB10.handle2, c, ehsys_B401BB25(c)) == 1) {
            h = ehsys_DFCA450B(0, 0);
            ehsys_F96953BE(h, D_0000CB10.handle2, c, ehsys_B401BB25(c), &tmp);
            ehsys_1EC5342B(h);
            break;
        }
    }
    if (i > 0) {
        for (j = 0; j < 18; j++) {
            if (i < 18) {
                D_0000CAD0.queue[j] = D_0000CAD0.queue[i];
                i++;
            } else {
                D_0000CAD0.queue[j] = 0;
            }
        }
    }
}

/* func_000065DC — load album page `page` into stack slot `slot`
 * (sorted into the draw list by depth) and start animation `anim`.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
Page *func_000065DC(int slot, int anim, int page) {
    Page *p = D_0000CB10.pages;
    float y = D_0000C9F8[slot].y;
    Page *prev = 0;
    Page *next = 0;
    int i;
    Page *pg;
    int page18;

    if (page < 0 || page >= (D_0000CB10.n_total + 0x11) / 0x12) {
        return 0;
    }
    while (p != 0) {
        if (y < p->y) {
            next = p;
            break;
        }
        prev = p;
        p = p->next;
    }
    pg = ehsys_41AABF28(D_0000CB10.heap, sizeof(Page));
    ehsys_memset(pg, 0, sizeof(Page));
    if (prev != 0) {
        prev->next = pg;
    } else {
        D_0000CB10.pages = pg;
    }
    pg->prev = prev;
    pg->next = next;
    if (next != 0) {
        next->prev = pg;
    }
    pg->slot = slot;
    pg->live = 1;
    page18 = page * 18;
    for (i = 0; i < 18; i++) {
        int j = page18 + i;

        if (j >= D_0000CB10.n_total) {
            pg->cards[i] = 0;
        } else {
            unsigned short c = D_0000CB10.cards[j];

            ehsys_B401BB25(c);
            pg->cards[i] = c;
        }
    }
    pg->page = page;
    pg->x = D_0000C9F8[slot].x;
    pg->y = y;
    pg->z = D_0000C9F8[slot].z;
    func_00002754(pg, anim, 1.0f);
    return pg;
}

/* func_00006800 — free every page.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00006800(void) {
    Page *p = D_0000CB10.pages;
    Page *n;

    D_0000CB10.pages = 0;
    while (p != 0) {
        n = p->next;
        ehsys_20E340D9(D_0000CB10.heap, p);
        p = n;
    }
}

/* func_00006848 — after a backward turn: drop the pages in slots 0/1 and
 * move the others one slot down.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00006848(void) {
    Page *prev;
    Page *p;
    Page *dead;

    p = D_0000CB10.pages;
    prev = 0;

    while (p != 0) {
        if (p->slot < 2) {
            dead = p;
            p = p->next;
            if (prev == 0) {
                D_0000CB10.pages = p;
                p->prev = 0;
            } else {
                prev->next = p;
                if (p != 0) {
                    p->prev = prev;
                }
            }
            ehsys_20E340D9(D_0000CB10.heap, dead);
        } else {
            p->slot--;
            prev = p;
            p = p->next;
        }
    }
    func_000069B8();
}

/* func_00006900 — after a forward turn: drop the pages in slots 6/7 and
 * move the others one slot up.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00006900(void) {
    Page *prev;
    Page *p;
    Page *dead;

    p = D_0000CB10.pages;
    prev = 0;

    while (p != 0) {
        if (p->slot >= 6) {
            dead = p;
            p = p->next;
            if (prev == 0) {
                D_0000CB10.pages = p;
                p->prev = 0;
            } else {
                prev->next = p;
                if (p != 0) {
                    p->prev = prev;
                }
            }
            ehsys_20E340D9(D_0000CB10.heap, dead);
        } else {
            p->slot++;
            prev = p;
            p = p->next;
        }
    }
    func_000069B8();
}

/* func_000069B8 — put every page at its slot's position.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000069B8(void) {
    Page *p;

    for (p = D_0000CB10.pages; p != 0; p = p->next) {
        p->x = D_0000C9F8[p->slot].x;
        p->y = D_0000C9F8[p->slot].y;
        p->z = D_0000C9F8[p->slot].z;
    }
}

/* func_00006A48 — the page in stack slot `slot`, or 0.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
Page *func_00006A48(unsigned short slot) {
    Page *p;

    for (p = D_0000CB10.pages; p != 0; p = p->next) {
        if (p->slot == slot) {
            return p;
        }
    }
    return 0;
}

/* func_00006A84 — returns 0 (unreferenced in this module).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00006A84(void) {
    return 0;
}

/* func_00006A8C — exported init (libcardalbum_rel): clear the state,
 * take the caller's heap or create a 10 MB one, set up 3D and the
 * resource tables, pick the button layout by language.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00006A8C(int arg, int heap) {
    ehsys_memset(&D_0000CB10, 0, sizeof(D_0000CB10));
    D_0000CB10.arg = arg;
    if (heap == 0) {
        D_0000CB10.block = ehsys_C07BB470(D_0000CA58, 2, 0, 0xA00000, 1, 0);
        ehsys_D979E9BF(D_0000CB10.block, &D_0000CB10.block_addr, 0);
        D_0000CB10.heap = ehsys_1856E536(D_0000CB10.block_addr, 0xA00000);
    } else {
        D_0000CB10.heap = heap;
    }
    ehsys_sceKernelChangeCurrentThreadAttr(0, 0x4000);
    func_00002044();
    func_00005C90();
    ehsys_C9D9E989(1);
    ehsys_4AA58320(1);
    if (ehsys_get_language() == 0) {
        ehsys_33ED567B(1);
        ehsys_D853F15C(0);
    } else {
        ehsys_33ED567B(0);
        ehsys_D853F15C(1);
    }
}

/* func_00006BB4 — exported per-frame update; returns 1 when the module
 * should exit. Modes: 0 load, 1 fade in, 2 open the cover, 3 browse,
 * 4 page turn, 5 letter jump, 6 card zoom, 7 idle, 8 unload.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00006BB4(void) {
    switch (D_0000CB10.mode) {
    case 0:
        if (!func_00005330()) {
            return 0;
        }
        if (func_00005D10()) {
            func_00002FD0();
            D_0000CB10.mode = 1;
            D_0000CB10.timer = 0;
            ehsys_D470D0B2(1, 0xFF000000, 0x10);
            ehsys_13A4081A(0);
        }
        return 0;
    case 1:
        if (!ehsys_450CD6C5()) {
            break;
        }
        if (D_0000CB10.timer++ > 0x20) {
            func_000026E0();
            func_00005238(6);
            D_0000CB10.mode = 2;
            D_0000CB10.timer = 0;
        }
        break;
    case 2:
        if (D_0000CB10.f4b == 2) {
            func_00005238(7);
            D_0000CB10.idx = 3;
            D_0000CB10.mode = 3;
        }
        break;
    case 3:
        func_00003010();
        break;
    case 4:
        func_00003EE4();
        break;
    case 6:
        func_000048BC();
        break;
    case 5:
        func_00001774();
        break;
    case 8:
        if (func_0000541C() && func_00006D78()) {
            return 1;
        }
        break;
    case 7:
        break;
    }
    func_000038C8();
    return 0;
}

/* func_00006D78 — mode 8 (after func_0000541C): close both archives,
 * then free everything (and the own memory block). 1 when done.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00006D78(void) {
    int busy;

    switch (D_0000CB10.step) {
    case 0:
        busy = 0;
        ehsys_BFFBA174();
        if (D_0000CB10.handle != 0) {
            ehsys_5DBE93DB(D_0000CB10.handle);
            busy = 1;
        }
        if (D_0000CB10.handle2 != 0) {
            ehsys_5DBE93DB(D_0000CB10.handle2);
            busy = 1;
        }
        if (busy) {
            D_0000CB10.step++;
        } else {
            D_0000CB10.step = 2;
        }
        break;
    case 1:
        if (ehsys_101103B3(D_0000CB10.handle) && ehsys_101103B3(D_0000CB10.handle2)) {
            D_0000CB10.step++;
        }
        break;
    case 2:
        func_00006800();
        func_00005EFC();
        if (D_0000CB10.block > 0) {
            ehsys_F6414A71(D_0000CB10.block, D_0000CB10.block_addr);
            ehsys_ED1410E0(D_0000CB10.block);
        }
        return 1;
    }
    return 0;
}

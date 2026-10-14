/*
 * rel_umd_replace.prx -- reconstructed code (matching decompilation)
 *
 * Compiler:     Metrowerks CodeWarrior for PSP -- MWCC 1.3 SP7 (mwccpsp_3.0.1_219)
 * Flags:        -O4,s -sdatathreshold 0   (see docs/09)
 * Verification: scripts/mwcc_build.sh + scripts/mwcc_diff.py, and exact
 *               word-for-word comparison with every relocation resolved
 *               (struct-member addends, baked absolute addresses and the
 *               .rodata constants by value). All 76 functions are tagged
 *               "MATCH 100%" and `make MODULE=rel_umd_replace SRC=1` is OK.
 *
 * WHAT THE MODULE DOES ("UMD replace" = previous-game disc check).
 * The player swaps the Tag Force 5 UMD for a disc of an earlier Tag Force
 * game; the module reads sector 16 of the raw UMD device ("umd1:"), takes
 * the 10-character disc ID stored at offset 0x373 of that ISO-9660 primary
 * volume descriptor and compares it against the IDs of TF5/TF4/TF3/TF2/TF1
 * (func_00003014). Each previous game unlocks a fixed set of bonus cards
 * (3 copies each, func_00001D98); owning all four unlocks two more
 * (func_00002884). A disc that was already used, or an unknown one, still
 * gives one pseudo-random card seeded by a checksum of its volume
 * descriptor (func_00001864). The cards are then shown one by one in a
 * small 3D card viewer with a "got card" message (func_000004F4 and the
 * card-display state machine func_0000028C), then the player is asked to
 * put the TF5 disc back.
 *
 * Disc table (strings at D_00009364..D_000093E8; result in D_0000BAC0.disc):
 *   0 = TF5 ULES-01474                              (the game itself)
 *   1 = TF4 ULJM-05479 / ULUS-10481 / ULES-01362
 *   2 = TF3 ULJM-05373 / ULES-01183 / ULES-01183   (compared TWICE: TF3 had
 *       no US release, the third test is a copy-paste leftover in the
 *       original source, preserved here)
 *   3 = TF2 ULJM-05260 / ULUS-10302 / ULES-00925
 *   4 = TF1 ULJM-05151 / ULUS-10136 / ULES-00600
 *   5 = anything else
 *
 * GLOBAL STATE (all in .bss, declared as structs; the target addresses some
 * fields through a base register and others through their own %hi/%lo pair,
 * which is exactly what MWCC does for `g.field` vs `p->field`):
 *   D_000096F0 UmdState  0x1CE0  card viewer (func_00000184 .. func_00000DF0)
 *   D_0000B3D0 UmdMain   0x06F0  main state machine (func_000011BC), the
 *                                reward list, message boxes
 *   D_0000BAC0 UmdDisc   0x01FC  disc-swap sub state machine (func_00002A60)
 *   D_0000BCC0           0x0800  sector 16 of the inserted disc
 *   D_00290CC0/D_00290CE4        archive slot table + loader state
 * Fields from 0xB910 upward are referenced with BAKED absolute addresses
 * (no relocation entry) in the shipped PRX; the code is identical either way.
 *
 * DATA LAYOUT (needed for `make SRC=1`). The shipped module has no .rodata:
 * jump tables and float constants sit in .data at 0x9224..0x934F, and the
 * Makefile places this object's .rodata there as NOLOAD, so the order of
 * our .rodata sections must reproduce the original:
 *   9224 jtbl 28C | 9244 jtbl 358 | 9264 jtbl 4F4 | 927C floats of A10 |
 *   9284 floats of 730 | 928C jtbl 11BC | 92D4 jtbl 1D98 | 92EC floats of
 *   14A0 | 92F4 jtbl 2A60 | 9320 three 16-byte-aligned camera vectors.
 * A single MWCC translation unit emits each jump table in front of its
 * function but pools every float literal at the very END of the unit (and
 * merges duplicates such as the two 1.7647059f), which cannot produce that
 * order -- the original was most likely split into several source files.
 * Here the constants are file-scope `static const` arrays defined at the
 * right points (MWCC emits those at their definition, interleaved with the
 * jump tables, and they generate the same lui/lwc1 code as a literal).
 * Strings are all extern D_ symbols for the same reason (the .data blob is
 * linked verbatim).
 *
 * MWCC LEVERS THAT MATTERED HERE (in addition to src/rel_movie_viewer.c):
 *   - switch tests are emitted in REVERSE source order, bodies in source
 *     order (func_00000624, func_00000B20, func_00001330, func_00001B9C ...);
 *     a jump table needs every case up to the highest index, so trailing
 *     empty cases (`case 7: break;`) are what make the table 8 long;
 *   - `if (!f()) { long } else { x = 1; }` gives the target's
 *     `bnel -> join` with the assignment in the delay slot, the other
 *     polarity does not (func_00001D98, func_00002A60);
 *   - `!!(pad[1] & btn)` produces the sltu/xori/bnez test sequence
 *     (func_00002EE0, func_00003284, func_000033DC);
 *   - stack slot order follows declaration order in reverse, and Mtx must
 *     be 16-byte aligned (func_00000A10, func_00000730);
 *   - a struct of float rows (Vec4) is copied with lwc1/swc1, a 64-byte
 *     Mtx with a lw/sw loop (func_00000988 vs func_00000730);
 *   - Allegrex min/max = __builtin_allegrex_min/max (func_00001864);
 *   - a `case` placed after `default:` moves which branch gets the
 *     join-block delay-slot fill (func_00000DF0);
 *   - field-by-field struct copies change the argument-setup order of the
 *     following call (func_000014A0);
 *   - a "void" import that really takes an argument shows up as a value kept
 *     in $a0 with a copy to $s0: check the EBOOT body (func_0000255C).
 *
 * Japanese text: none in this module (all strings are ASCII file names,
 * disc IDs and the "database/umd_replace/%s" path).
 */

typedef struct Vec4 {
    float x, y, z, w;
} Vec4;

/* 4x4 matrix, rows x/y/z/w (w = translation); 16-byte aligned like every
 * VFPU matrix (the stack slots in func_00000A10 prove the alignment) */
typedef struct Mtx {
    Vec4 x, y, z, w;
} Mtx __attribute__((aligned(16)));

typedef struct Entry { /* 0xD0 */
    char pad0[0x80];
    Mtx mtx;        /* +0x80 */
    int id;         /* +0xC0 */
    char padC4[0xC];
} Entry;

/* 0x7C-byte message-window parameter block, filled by func_00000CF0 /
 * func_00000DF0 and handed to ehsys_50B8DF28 / ehsys_D7155BDE. */
typedef struct MsgParam {
    int f00;            /* +0x00 */
    int f04;
    unsigned char f08;  /* +0x08 */
    char pad09[3];
    short x;            /* +0x0C */
    short y;            /* +0x0E */
    int f10;
    int f14;            /* +0x14 */
    short f18;          /* +0x18 */
    short pad1A;
    int pad1C;
    short w;            /* +0x20 */
    short h;            /* +0x22 */
    int f24;            /* +0x24 */
    int pad28[4];
    int text;           /* +0x38 */
    int f3C;            /* +0x3C */
    int f40;            /* +0x40 */
    int pad44;
    short f48;          /* +0x48 */
    short f4A;          /* +0x4A */
    short f4C;          /* +0x4C */
    short f4E;          /* +0x4E */
    unsigned char f50;  /* +0x50 */
    char pad51[3];
    int f54;            /* +0x54 */
    int pad58[3];
    int f64;            /* +0x64 */
    int f68;            /* +0x68 */
    int pad6C[4];
} MsgParam;

typedef struct BitsCC {
    unsigned char b0 : 1;
    unsigned char b1 : 1;
    unsigned char b2 : 1;
    unsigned char b3 : 1;
    unsigned char b4 : 1;
    unsigned char b5 : 1;
    unsigned char b6 : 1;
    unsigned char b7 : 1;
} BitsCC;

typedef struct Nibbles {
    unsigned char lo : 4;
    unsigned char hi : 4;
} Nibbles;

/* 0x124-byte message box (dialog work area + MsgParam + flags), set up by
 * func_00000DF0 */
typedef struct MsgBox {
    char work[0x4C];    /* +0x00 */
    MsgParam param;     /* +0x4C */
    int fC8;            /* +0xC8 */
    BitsCC fCC;         /* +0xCC */
    char padCD;
    Nibbles fCE;        /* +0xCE */
    char padCF[8];
    Nibbles fD7;        /* +0xD7 */
    unsigned char kind; /* +0xD8 */
    char padD9[0x124 - 0xD9];
} MsgBox;

typedef struct UmdState { /* D_000096F0, 0x1CE0 bytes */
    int state;          /* +0x0000 */
    int model;          /* +0x0004 */
    int pad8[2];
    Entry entries[32];  /* +0x0010 */
    int count;          /* +0x1A10 */
    unsigned int timer; /* +0x1A14 */
    char dialog[0x4C];  /* +0x1A18 */
    MsgParam param;     /* +0x1A64 */
    char text[0x200];   /* +0x1AE0 */
} UmdState;

/* D_0000B3D0, 0x6F0 bytes: the module's main state machine (state = the
 * switch index of func_000011BC) */
typedef struct UmdMain {
    int state;          /* +0x000 */
    int ret_state;      /* +0x004: state to go back to after message 16 */
    int disc;           /* +0x008: identified disc, see the header table */
    int f00C;
    struct {
        unsigned short num; /* copies to give */
        unsigned short id;  /* card id */
    } list[0x100];      /* +0x010 */
    int n_list;         /* +0x410: entries in list */
    int cur;            /* +0x414: entry being shown (states 12/13) */
    int n_cards;        /* +0x418: total cards, sizes the scrolling list */
    MsgBox box41C;      /* +0x41C */
    char box540[0x4C];  /* +0x540 */
    MsgParam param58C;  /* +0x58C */
    int saved608;       /* +0x608 */
    int fx_trans;       /* +0x60C: trans03.ehf/.tmf effect (resources 7/8) */
    int fx_trans_work[0x18]; /* +0x610 */
    int fx_circle;      /* +0x670: umd_circle01.ehf/.tmf (resources 9/10) */
    int pad674[3];
    int fx_circle_work[0x18]; /* +0x680 */
    int show_circle;    /* +0x6E0: drawn while the disc check is busy */
    int pad6E4[3];
} UmdMain;

/* D_0000BAC0, 0x1FC bytes: the UMD-swap sub-state machine (func_00002A60) */
typedef struct UmdDisc {
    int state;          /* +0x000 */
    int disc;           /* +0x004: 0 = TF5 .. 4 = TF1, 5 = unknown */
    int f008;           /* +0x008: set by func_00002A38's argument */
    int cb;             /* +0x00C: UMD callback id */
    MsgBox box10;       /* +0x010 */
    char box134[0x4C];  /* +0x134 */
    MsgParam param180;  /* +0x180 */
} UmdDisc;

/* archive slot: file name + loaded handle (0x24 bytes); the same layout is
 * used by the static resource-name table at D_000094B4, where the int is
 * the archive slot instead */
typedef struct ResFile {
    char name[0x20];
    int handle;         /* +0x20 */
} ResFile;

/* D_00290CE4: archive loader state */
typedef struct Loader {
    int heap;           /* +0x00 */
    int arc;            /* +0x04 */
    int loading;        /* +0x08: slot being loaded, -1 = idle */
    void (*cb)(int, int); /* +0x0C */
    int cb_arg;         /* +0x10 */
} Loader;

extern ResFile D_00290CC0[];
extern Loader D_00290CE4;
extern ResFile D_000094B4[];  /* resource table: "strTbl.bin", "card.tms", ... */
extern char D_000093F4[][0x20]; /* "basic_lang_jpn.ehp", ... (indexed by language) */
extern char D_000096AC[];     /* "ehp" */
extern char D_000096B0[];     /* "database/umd_replace/%s" */
extern char D_00010CC0[];     /* 0x180000-byte heap */
extern char D_00190CC0[];     /* 0x100000-byte archive work area */

extern UmdState D_000096F0;
extern UmdDisc D_0000BAC0;
extern unsigned char D_0000BCC0[];
extern char D_00009350[]; /* "UMD" */
extern char D_00009354[]; /* "disc0:" */
extern UmdMain D_0000B3D0;
extern volatile int D_000096EC;
extern Entry D_00009700; /* == D_000096F0.entries[0], addressed by its own symbol */
extern char D_000096E0;
extern float D_00009284;
extern float D_00009288;

extern void ehsys_B4471B5E(void (*)(void), void (*)(void), void *);
extern void ehsys_13A4081A(int);
extern void ehsys_C2E59A21(int, void (*)(void), void (*)(void));
extern void ehsys_frame_sync(void);
extern void ehsys_8060707A(void);
extern void ehsys_08813E19(int, int);
extern int sceKernelChangeCurrentThreadAttr(int, int);
extern void ehsys_memset(void *, int, int);
extern void ehsys_30FECAEF(void *, int);
extern int ehsys_3765ED01(int, int);
extern void ehsys_1CC5878C(void *);
extern void ehsys_5DF04F49(void *);
extern void ehsys_9490E557(int, int, void *);
extern void ehsys_BFFBA174(void);
extern int ehsys_B779C0DF(void);
extern void ehsys_D470D0B2(int, int, int);
extern int ehsys_8867335A(void *, void *);
extern void ehsys_834A6E13(void *, void *);
extern void ehsys_C1F72EF5(void);
extern int ehsys_DFCA450B(int, int);
extern void ehsys_14BFFB01(int *, void *);
extern void ehsys_1EC5342B(int);
extern void ehsys_13A603EB(Mtx *);
extern int ehsys_E58C0FDC(int, int);
extern int ehsys_get_language(void);
extern int ehsys_AD0E1E03(int);
extern void ehsys_E50B9594(char *, int, ...);
extern void *ehsys_F5E3080C(void);
extern int ehsys_get_button_code(int);
extern void ehsys_FA53F08C(void *, void *);
extern void *ehsys_53636C1D(void);
extern void ehsys_D697947E(Mtx *, Mtx *, Mtx *);
extern void ehsys_8B0A8B96(float, Mtx *, Mtx *);
extern void ehsys_50D6F57B(int, int, int);
extern void ehsys_D1A604C7(int);
extern int ehsys_B401BB25(int);
extern int ehsys_8C5B148E(int, int, int);

int func_00000038(void);
void func_000000DC(void);
int func_000000E4(void);
int func_00000118(void);
void func_00000150(void);
void func_0000017C(void);
void func_00000184(void);
void func_00000624(void);
void func_00000730(void);
Mtx *func_00000988(Mtx *dst, Mtx *src);
void func_00000A10(Entry *e);
int func_00000B20(int *h, int unused, int mode);
void func_00000CF0(int text);
void func_00000DF0(MsgBox *b, int text, int type, int nib, int flag);
int func_00000FD0(MsgBox *box);
int func_00001018(MsgBox *box, int h);
void func_0000042C(int count);
void func_000004F4(int id, int arg1);
int func_00000614(void);
void func_0000028C(void);
void func_000010A0(void);
void func_00001164(void);
int func_000011BC(void);
void func_00001330(void);
void func_00000358(void);
int func_000014A0(int);
int func_00001748(int text);
void func_00001A1C(void);
void func_00001A88(void);
void func_00001A54(void);
void func_00001B58(void);
void func_00001B9C(void);
void func_00001C7C(void);
void func_00001CC0(void);
void func_00001D98(void);
void func_00002450(void);
void func_00002484(void);
void func_000024C4(void);
void func_00002514(void);
void func_0000255C(void);
void func_000026F0(void);
void func_00002754(void);
void func_000027A4(void);
void func_0000282C(void);
void func_0000293C(void);
void func_000029BC(void);
void func_00002E78(void);
void func_0000364C(void);
int func_000036D4(void);
void func_00003714(void);
void func_0000371C(int slot, void (*cb)(void), int arg);
int func_00003804(const char *name, int slot);
void func_000038D0(const char *name, int unused, int async);
int func_00003970(int buf, int size);
int func_000036B0(void);
void func_00002A38(int);
int func_00003284(const char *path, int flags, int mode, int *res);
int func_000033DC(int fd, void *buf, int size, int *res);
int func_0000327C(int, int, void *);
int func_00002EE0(int *);
int func_00003014(int *);
void func_00003550(int);
unsigned char *func_00002ED4(void);
void func_00002884(void);
int func_00001864(unsigned char *save);
int func_00002A60(void);
void func_00002A58(void);
int func_00002EC8(void);
int func_00003898(int);
int func_000038A8(int);
int func_000037E4(int);
int func_000038B8(void);
int func_000038C4(void);

extern int ehsys_F462EE6E(int, int);
extern int ehsys_09794DF0(int, int, int);
extern int ehsys_5CBB9E43(int, int, int, int, int, int);
extern int ehsys_4175E3DB(int, int);
extern int ehsys_CABEA834(int, int);
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
extern void ehsys_50B8DF28(void *, MsgParam *);
extern void ehsys_D7155BDE(void *, MsgParam *);
extern void ehsys_2AE3C0AD(int);
extern void ehsys_2012884C(MsgBox *);
extern int ehsys_39ADDA9F(void);
extern void ehsys_E1139F1A(MsgBox *);
extern int ehsys_D3448D59(void);
extern int ehsys_1D821C84(void);
extern void ehsys_33ED567B(int);
extern void ehsys_D853F15C(int);
extern void ehsys_EF9B5D06(int *, MsgBox *);
extern int ehsys_459B889D(int);
extern void ehsys_EBD1986B(int (*)(int), int (*)(int));
extern int ehsys_F96D270D(void);
extern void ehsys_4AA58320(int);
extern void ehsys_5998BFD7(int, int, int);
extern void ehsys_03E45FFF(void);
extern void ehsys_4F22C9AA(int);
extern void ehsys_F2BD0694(void);
extern void ehsys_9EFDC9B9(void);
extern void ehsys_8CA1052F(float, float, float, float);
extern void ehsys_EF9740B0(Mtx *);
extern void ehsys_BB22B13B(int, int *);
extern void ehsys_F8CDBCAB(void);

extern int ehsys_85F89FE8(int);
extern int ehsys_7187E0F0(int, int, int *);
extern int ehsys_89812D8A(int);
extern void ehsys_878EE1D3(int *);
extern int ehsys_450CD6C5(void);

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
extern int ehsys_1D98951B(int, int);
extern int ehsys_399AC68A(int, int);
extern int ehsys_42110BF9(int, int);
extern int ehsys_C62F9EAC(int, int);
extern int ehsys_E6E4401F(int, int, int);
extern int ehsys_D6E0AA6A(int, int);
extern int ehsys_D71FA940(int);
extern int ehsys_94982653(int, int);
extern void ehsys_E2A5CE8D(Vec4 *, Vec4 *, Vec4 *);
extern void ehsys_D1C6F209(void);
extern void ehsys_5305D550(int);
extern int ehsys_D4DE5DD8(void);
extern int ehsys_28F7224B(int);
extern int ehsys_67312719(int);
extern void ehsys_FDAFCF3A(int, int);
extern void ehsys_42144DB8(int, int);

/* .rodata placement -- see the file header ("data layout") */
static const float s_proj_14A0[2];
static const Vec4 s_cam_eye __attribute__((aligned(16)));
static const Vec4 s_cam_at __attribute__((aligned(16)));
static const Vec4 s_cam_up __attribute__((aligned(16)));

extern int ehsys_D79EAEBD(void);
extern int ehsys_5ABF64AE(void);
extern int ehsys_62A243C4(void);
extern int ehsys_3FD815F4(void);
extern void ehsys_D907D31A(int, int);
extern void ehsys_48B2E54A(int, int);
extern void ehsys_5A6B0741(int);
extern void ehsys_6E04A2B5(void);
extern void ehsys_C15BF702(void);
extern void ehsys_7A8E41CA(void);
extern void ehsys_5ED6B8AE(void);
extern void ehsys_6DF8A193(void);
extern int ehsys_D0FBFC51(unsigned char *);
extern void ehsys_83B39FA9(unsigned char *);

extern void ehsys_C4FA973E(int, int);
extern void ehsys_79B05ABC(unsigned short); /* sets flag 0x08 in the record of card `id` */
extern void ehsys_EFADAB40(unsigned short, int);

extern int sceKernelCreateCallback(const char *, void *, void *);
extern int sceKernelDeleteCallback(int);
extern int sceUmdRegisterUMDCallBack(int);
extern int sceUmdUnRegisterUMDCallBack(int);
extern int sceUmdActivate(int, const char *);
extern int sceImposeSetUMDPopup(int);
extern int sceUmdReplaceProhibit(void);
extern int sceUmdReplacePermit(void);

extern int sceUmdCheckMedium(void);
extern int sceUmdGetDriveStat(void);
extern int ehsys_EB263A38(void);
extern int sceIoOpen(const char *, int, int);
extern int sceIoClose(int);
extern int sceIoRead(int, void *, int);
extern int sceIoLseek(int, long long, int);
extern int sceUmdGetDiscInfo(void *);
extern void ehsys_memcpy(void *, void *, int);
extern int ehsys_strcmp(const char *, const char *);
extern char D_0000935C[]; /* "umd1:" */
extern char D_00009364[]; /* "ULES-01474" TF5 (EU) */
extern char D_00009370[]; /* "ULJM-05479" TF4 (JP) */
extern char D_0000937C[]; /* "ULUS-10481" TF4 (US) */
extern char D_00009388[]; /* "ULES-01362" TF4 (EU) */
extern char D_00009394[]; /* "ULJM-05373" TF3 (JP) */
extern char D_000093A0[]; /* "ULES-01183" TF3 (EU) */
extern char D_000093AC[]; /* "ULJM-05260" TF2 (JP) */
extern char D_000093B8[]; /* "ULUS-10302" TF2 (US) */
extern char D_000093C4[]; /* "ULES-00925" TF2 (EU) */
extern char D_000093D0[]; /* "ULJM-05151" TF1 (JP) */
extern char D_000093DC[]; /* "ULUS-10136" TF1 (US) */
extern char D_000093E8[]; /* "ULES-00600" TF1 (EU) */

extern int ehsys_1856E536(void *, int);
extern int ehsys_291D6262(void *, int, int);
extern int ehsys_2717C28C(int);
extern void ehsys_1078C73B(int);
extern int ehsys_C6C15111(int, const char *, int);
extern int ehsys_strlen(const char *);
extern void ehsys_4B0DABFA(int, int);
extern int ehsys_41AABF28(int, int);
extern int ehsys_20E340D9(int, int);
extern void ehsys_sprintf(char *, const char *, ...);
extern int ehsys_D2A768F4(int, char *);
extern void ehsys_8288EEBC(int, char *, int, int, int (*)(int, int), int);

/* ==== CODE ==== */

/* append 3 copies of `card` to the reward list (func_00001D98) */
#define ADD_CARD(card)                      \
    m->list[m->n_list].num = 3;             \
    m->list[m->n_list].id = (card);         \
    m->n_list++;                            \
    m->n_cards++


/* func_00000000 -- module entry: registers the main routine (func_00000038) and the empty
 * teardown hook (func_000000DC); does nothing when called with a non-zero arg.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000000(int arg0) {
    if (arg0 == 0) {
        ehsys_B4471B5E((void (*)(void)) func_00000038, func_000000DC, &D_000096E0);
    }
    return 0;
}

/* func_00000038 -- main routine: init, install the draw/exit frame callbacks, run the state
 * machine until it reports completion, fade out and request mode 14.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000038(void) {
    int done = 0;

    ehsys_13A4081A(0);
    if (func_000000E4()) {
        ehsys_C2E59A21(0, func_00000150, func_0000017C);
        while (!done) {
            done |= func_00000118();
            ehsys_frame_sync();
            if (done) {
                ehsys_8060707A();
                ehsys_frame_sync();
                ehsys_08813E19(14, 0);
                break;
            }
        }
    }
}

/* func_000000DC -- empty teardown hook registered by func_00000000.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000000DC(void) {
}

/* func_000000E4 -- init: give the thread the VFPU attribute (0x4000), run the module init
 * (func_000010A0) and mark the module active; always returns 1.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000000E4(void) {
    sceKernelChangeCurrentThreadAttr(0, 0x4000);
    func_000010A0();
    D_000096EC = 1;
    return 1;
}

/* func_00000118 -- per-frame update: run the main state machine; when it finishes clear the
 * active flag and return 1.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000118(void) {
    volatile int *p = &D_000096EC;
    if (func_000011BC()) {
        *p = 0;
        return 1;
    }
    return 0;
}

/* func_00000150 -- draw callback: draw the frame (func_00001330) while the module is active.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000150(void) {
    if (D_000096EC) {
        func_00001330();
    }
}

/* func_0000017C -- exit callback: module teardown (func_00001164).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_0000017C(void) {
    func_00001164();
}

/* func_00000184 -- card viewer init: clear D_000096F0 and create its message window.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000184(void) {
    ehsys_memset(&D_000096F0, 0, sizeof(UmdState));
    ehsys_30FECAEF(D_000096F0.dialog, func_000038B8());
}

/* func_000001C4 -- card viewer: load the card model (resources 1..6 of the archive).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000001C4(void) {
    int buf[9];

    D_000096F0.model = ehsys_3765ED01(func_000037E4(1), 0);
    ehsys_memset(buf, 0, sizeof(buf));
    buf[0] = func_000037E4(2);
    buf[1] = func_000037E4(3);
    buf[3] = func_000037E4(4);
    buf[4] = func_000037E4(5);
    buf[5] = func_000037E4(6);
    buf[7] = func_000038C4();
    ehsys_1CC5878C(buf);
}

/* func_00000250 -- card viewer teardown: release the message window and the model.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000250(void) {
    ehsys_5DF04F49(D_000096F0.dialog);
    ehsys_9490E557(D_000096F0.model, 0, 0);
    ehsys_BFFBA174();
}

/* func_0000028C -- card viewer update: dispatch on D_000096F0.state (1-2 scrolling list,
 * 3-5 single card, 6 waiting for the fade, 7 closing the message window).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_0000028C(void) {
    UmdState *s = &D_000096F0;

    switch (s->state) {
    case 1:
    case 2:
        func_00000624();
        break;
    case 3:
    case 4:
    case 5:
        func_00000730();
        break;
    case 6:
        if (ehsys_B779C0DF()) {
            ehsys_D470D0B2(1, 0xFF000000, 1);
            s->param.f00 = 1;
            s->state = 5;
        }
        break;
    case 7:
        if (ehsys_8867335A(s->dialog, &s->param) == 4) {
            ehsys_834A6E13(s->dialog, &s->param);
            s->state = 0;
        }
        break;
    }
}

/* func_00000358 -- card viewer draw.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000358(void) {
    UmdState *s = &D_000096F0;
    int h;
    int i;

    switch (s->state) {
    case 1:
    case 2:
        for (i = 0; i < s->count; i++) {
            func_00000A10(&s->entries[i]);
        }
        break;
    case 3:
    case 4:
    case 5:
        func_00000A10(&s->entries[0]);
        break;
    case 6:
        ehsys_C1F72EF5();
        break;
    case 7:
        break;
    }
    h = ehsys_DFCA450B(4, 0);
    ehsys_14BFFB01(&h, s->dialog);
    ehsys_1EC5342B(h);
}

/* func_0000042C -- card viewer: start the scrolling list of `count` cards.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_0000042C(int count) {
    UmdState *s = &D_000096F0;
    Entry *e;
    int i;

    D_000096F0.count = count;
    D_000096F0.state = 1;
    for (i = 0; i < s->count; i++) {
        e = &s->entries[i];
        ehsys_13A603EB(&e->mtx);
        e->mtx.w.y = 5.0f * i + 10.0f;
        e->id = 0;
    }
    func_00000CF0(ehsys_E58C0FDC(func_000037E4(0), 3));
}

/* func_000004F4 -- card viewer: show one card (`id`, `num` copies) with the "got card"
 * message; the argument order of the format call depends on the language
 * (language 0 puts the card name first).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000004F4(int id, int arg1) {
    Entry *e;
    UmdState *s = &D_000096F0;

    D_000096F0.state = 3;
    e = D_000096F0.entries;
    ehsys_13A603EB(&e->mtx);
    D_000096F0.timer = 0;
    e->id = id;
    switch (ehsys_get_language()) {
    default:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        ehsys_E50B9594(s->text, ehsys_E58C0FDC(func_000037E4(0), 6), arg1, ehsys_AD0E1E03(id));
        break;
    case 0:
        ehsys_E50B9594(s->text, ehsys_E58C0FDC(func_000037E4(0), 6), ehsys_AD0E1E03(id), arg1);
        break;
    }
    func_00000CF0((int) s->text);
    func_00000730();
}

/* func_00000614 -- card viewer busy? (state != 0)
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000614(void) {
    return D_000096F0.state != 0;
}

/* func_00000624 -- card viewer: scrolling-list update (states 1 and 2).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000624(void) {
    UmdState *s = &D_000096F0;
    int *pad;
    int btn;
    int i;

    pad = ehsys_F5E3080C();
    btn = ehsys_get_button_code(0) & 0xFFFF;
    switch (D_000096F0.state) {
    case 1:
        for (i = 0; i < s->count; i++) {
            s->entries[i].mtx.w.y -= 0.25f;
        }
        if (s->entries[s->count - 1].mtx.w.y <= -10.0f) {
            s->state = 2;
        }
        break;
    case 2:
        if (pad[1] & (btn & 0xFFFF)) {
            ehsys_FA53F08C(s->dialog, &s->param);
            s->state = 7;
        }
        break;
    }
}

static const float s_proj_A10[2] = {1.7647059f, 0.38397247f}; /* 480/272, 22 deg */
static const float s_rot_730[2] = {3.1415927f, 0.31f};

/* func_00000730 -- card viewer: single-card update -- spins the card in over 15 frames
 * (state 4), then waits for a button (state 5).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000730(void) {
    UmdState *s = &D_000096F0;
    int *pad;
    int btn;
    char *cam;
    Mtx view;
    Mtx ofs;
    Mtx m;
    Mtx rot;
    float a;
    int ok;

    pad = ehsys_F5E3080C();
    btn = ehsys_get_button_code(0) & 0xFFFF;
    cam = ehsys_53636C1D();
    view = *(Mtx *) (cam + 0x80);
    view.w.x = *(float *) (cam + 0x10);
    view.w.y = *(float *) (cam + 0x14);
    view.w.z = *(float *) (cam + 0x18);
    ehsys_13A603EB(&ofs);
    ofs.w.y = s_rot_730[1];
    ofs.w.z = -0.5f;
    ehsys_13A603EB(&m);
    ehsys_D697947E(&m, &m, &view);
    ehsys_D697947E(&m, &m, &ofs);
    a = s_rot_730[0] * (D_000096F0.timer / 15.0f);
    ehsys_13A603EB(&rot);
    ehsys_8B0A8B96(a, &rot, &rot);
    ehsys_D697947E(&m, &m, &rot);
    func_00000988(&D_00009700.mtx, &m);
    switch (D_000096F0.state) {
    case 3:
        ok = ehsys_8C5B148E(func_000038C4(), s->entries[0].id, ehsys_B401BB25((unsigned short) s->entries[0].id)) != 0;
        if (ok) {
            s->state = 4;
        }
        break;
    case 4:
        if (s->timer < 15) {
            s->timer++;
        } else {
            s->state = 5;
        }
        break;
    case 5:
        if (pad[1] & (btn & 0xFFFF)) {
            ehsys_FA53F08C(s->dialog, &s->param);
            s->state = 7;
        }
        if (pad[1] & 0x1000) {
            int a1 = func_000037E4(11);
            ehsys_50D6F57B((unsigned short) s->entries[0].id, a1, func_000037E4(12));
            ehsys_D1A604C7(func_000037E4(13));
            s->param.f00 = 0;
            s->state = 6;
        }
        break;
    }
}

/* func_00000988 -- copy a 4x4 matrix row by row, returns dst.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
Mtx *func_00000988(Mtx *dst, Mtx *src) {
    dst->x = src->x;
    dst->y = src->y;
    dst->z = src->z;
    dst->w = src->w;
    return dst;
}

/* func_00000A10 -- card viewer: draw one card entry (perspective 22 deg, 480/272).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000A10(Entry *e) {
    int cfg[13];
    Mtx m;

    ehsys_memset(cfg, 0, sizeof(cfg));
    ehsys_4F22C9AA(0);
    ehsys_F2BD0694();
    ehsys_9EFDC9B9();
    ehsys_8CA1052F(s_proj_A10[1], s_proj_A10[0], 5.0f, 400.0f);
    ehsys_9490E557(D_000096F0.model, 0, func_00000B20);
    m = e->mtx;
    ehsys_4F22C9AA(2);
    ehsys_F2BD0694();
    ehsys_9EFDC9B9();
    ehsys_EF9740B0(&m);
    cfg[0] = 1;
    cfg[1] = 2;
    ehsys_BB22B13B(D_000096F0.model, cfg);
    ehsys_4F22C9AA(2);
    ehsys_F8CDBCAB();
    ehsys_4F22C9AA(0);
    ehsys_F8CDBCAB();
}

/* func_00000B20 -- card model draw callback (render-state command chain); mode 0 = front,
 * mode 1 = back with the card image when an id is set.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000B20(int *h, int unused, int mode) {
    Entry *e = D_000096F0.entries;
    int v = *h;

    switch (mode) {
    case 1:
    case 0:
        v = ehsys_F462EE6E(v, 9);
        v = ehsys_F462EE6E(v, 4);
        v = ehsys_09794DF0(v, 3, 1);
        v = ehsys_5CBB9E43(v, 0, 2, 3, 0, 0);
        v = ehsys_4175E3DB(v, 1);
        v = ehsys_CABEA834(v, -1);
        v = ehsys_B4F2178B(v);
        v = ehsys_5DE4869F(v, 0x160000, -1, 0x80, 0x100, 3);
        if (mode == 0) {
            v = ehsys_AD222020(v, 0, 0);
        } else if (e->id != 0) {
            v = ehsys_FD84C4D3(v, 0, 0, e->id);
        }
        v = ehsys_B9064026(v);
        v = ehsys_B4F2178B(v);
        v = ehsys_F462EE6E(v, 9);
        v = ehsys_572A577B(v, 3, 0, 0, 0);
        v = ehsys_3DE5C8DC(v, 0, 0x80, 0x100, 0x80, ehsys_E47E40E4() + 0x160000);
        v = ehsys_09794DF0(v, 3, 0);
        v = ehsys_F17C5C28(v, 1, 1);
        v = ehsys_7B1755CF(v, 0.0f, 0.6015625f);
        v = ehsys_3E24ACE1(v, 0.84375f, -0.6015625f);
        *h = ehsys_F462EE6E(v, 1);
        return 1;
    }
    return 0;
}

/* func_00000CF0 -- card viewer: set up the message window parameters for `text`.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000CF0(int text) {
    MsgParam *p = &D_000096F0.param;

    ehsys_memset(p, 0, sizeof(MsgParam));
    p->f00 = 1;
    p->f64 = 1;
    p->f68 = 1;
    p->f3C = 1;
    p->y = 210;
    p->w = 400;
    p->h = 60;
    p->f08 = 2;
    p->f24 = -1;
    p->f54 = -1;
    p->f50 = 13;
    p->f4C = 8;
    p->f48 = 10;
    p->f4A = 10;
    p->text = text;
    p->f40 = 0;
    p->f4E = 0;
    ehsys_50B8DF28(D_000096F0.dialog, p);
    p->x = 240.0f - p->w * 0.5f;
    ehsys_D7155BDE(D_000096F0.dialog, p);
    ehsys_2AE3C0AD(0x94CB);
}

/* func_00000DF0 -- set up a 0x124-byte message box (window parameters + flags) for `text`.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). `kind` must be an
 * int (a u8 adds `andi 0xff` to the reused compare constant 3) and `case 3`
 * must come AFTER `default`: the target's `beq` + nop for type 3 is the
 * `kind = 3` delay-slot fill later deleted as redundant (v0 already holds 3);
 * with case 3 before default the join block's `li t0,1` fills that slot. */
void func_00000DF0(MsgBox *b, int text, int type, int nib, int flag) {
    MsgParam *p = &b->param;
    int kind;

    ehsys_memset(b, 0, sizeof(MsgBox));
    switch (type) {
    case 1:
        kind = 0;
        break;
    case 2:
        kind = 4;
        break;
    default:
        kind = 0;
        break;
    case 3:
        kind = 3;
        break;
    }
    p->f00 = 1;
    p->f08 = 2;
    p->x = 0;
    p->y = 0;
    p->f14 = 1;
    p->f4C = 8;
    p->f48 = 10;
    p->f4A = 10;
    p->f4E = 0;
    p->f24 = -1;
    p->f64 = 1;
    p->f68 = 1;
    p->text = text;
    p->f3C = 1;
    p->f40 = 0;
    p->f50 = 13;
    p->f54 = -1;
    p->f18 = 450;
    b->fCC.b1 = 1;
    b->fCC.b5 = 1;
    b->kind = kind;
    b->fCE.lo = type;
    b->fCC.b2 = flag != 0;
    b->fD7.lo = nib;
    b->fC8 = func_000038B8();
    ehsys_2012884C(b);
    p->x = 240.0f - p->w * 0.5f;
    p->y = 136.0f - p->h * 0.5f;
    ehsys_2AE3C0AD(0x94CB);
}

/* func_00000FD0 -- update a message box; returns the selection (-1 while still open).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000FD0(MsgBox *box) {
    int r = ehsys_39ADDA9F();
    if (r != -1) {
        ehsys_E1139F1A(box);
    }
    return r;
}

/* func_00001018 -- draw a message box with the font mode chosen by the language.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00001018(MsgBox *box, int h) {
    int a = ehsys_D3448D59();
    int b = ehsys_1D821C84();

    if (ehsys_get_language() == 0) {
        ehsys_33ED567B(1);
    } else {
        ehsys_D853F15C(1);
    }
    ehsys_EF9B5D06(&h, box);
    ehsys_33ED567B(a);
    ehsys_D853F15C(b);
    return h;
}

/* func_000010A0 -- module init: archive/heap, main state, card viewer, UMD callbacks.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000010A0(void) {
    ehsys_1EC5342B(ehsys_459B889D(ehsys_DFCA450B(0, 0)));
    ehsys_EBD1986B(func_00003898, func_000038A8);
    ehsys_memset(&D_0000B3D0, 0, sizeof(UmdMain));
    D_0000B3D0.n_list = 0;
    D_0000B3D0.cur = 0;
    D_0000B3D0.n_cards = 0;
    func_0000364C();
    func_00000184();
    ehsys_30FECAEF(D_0000B3D0.box540, func_000038B8());
    func_0000293C();
    D_0000B3D0.saved608 = ehsys_F96D270D();
    ehsys_4AA58320(1);
    ehsys_5998BFD7(1, 0xFF000000, 1);
}

/* func_00001164 -- module teardown (reverse of func_000010A0).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001164(void) {
    ehsys_4AA58320(D_0000B3D0.saved608);
    func_000029BC();
    func_00000250();
    ehsys_5DF04F49(D_0000B3D0.box540);
    func_000036D4();
    ehsys_03E45FFF();
    ehsys_EBD1986B(0, 0);
}

/* func_000011BC -- main state machine (D_0000B3D0.state, 0..17); returns 1 when done.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000011BC(void) {
    int ret = 0;

    func_00003714();
    switch (D_0000B3D0.state) {
    default:
    case 0:
        func_00001A1C();
        break;
    case 1:
        func_00001A54();
        break;
    case 2:
        break;
    case 3:
        func_00001B58();
        break;
    case 4:
        func_00001B9C();
        break;
    case 5:
        func_00001C7C();
        break;
    case 6:
        func_00001CC0();
        break;
    case 7:
        func_00001D98();
        break;
    case 8:
        func_00002450();
        break;
    case 9:
        func_00002484();
        break;
    case 10:
        func_000024C4();
        break;
    case 11:
        func_00002514();
        break;
    case 12:
        func_0000255C();
        break;
    case 13:
        func_000026F0();
        break;
    case 14:
        func_00002754();
        break;
    case 15:
        func_000027A4();
        break;
    case 16:
        func_0000282C();
        break;
    case 17:
        if (ehsys_450CD6C5()) {
            func_00001164();
            ret = 1;
        }
        break;
    }
    return ret;
}

/* func_00001330 -- draw the frame: 3D pass, two animated layers, card viewer or disc
 * message, then the main message box.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001330(void) {
    UmdMain *m = &D_0000B3D0;
    int h;

    h = ehsys_DFCA450B(0, 0);
    h = func_000014A0(h);
    ehsys_1EC5342B(h);
    h = ehsys_DFCA450B(1, 0);
    if (D_0000B3D0.fx_trans) {
        m->fx_trans_work[0]++;
        h = ehsys_85F89FE8(h);
        h = ehsys_7187E0F0(h, m->fx_trans, m->fx_trans_work);
        h = ehsys_89812D8A(h);
    }
    if (m->show_circle) {
        m->fx_circle_work[0]++;
        h = ehsys_85F89FE8(h);
        h = ehsys_7187E0F0(h, m->fx_circle, m->fx_circle_work);
        h = ehsys_89812D8A(h);
    }
    ehsys_1EC5342B(h);
    switch (m->state) {
    case 4:
        func_00002E78();
        break;
    case 9:
    case 13:
        func_00000358();
        break;
    }
    h = ehsys_DFCA450B(4, 0);
    h = func_00001018(&m->box41C, h);
    ehsys_14BFFB01(&h, m->box540);
    ehsys_878EE1D3(&h);
    ehsys_1EC5342B(h);
}

/* func_000014A0 -- 3D scene setup: render state chain, perspective and the look-at camera.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). The camera
 * vectors are copied FIELD BY FIELD: a whole-struct `up = s_cam_up;` emits the
 * same lwc1/swc1 pairs but MWCC then schedules the `&up`/`&at`/`&eye` argument
 * setup right-to-left (a2 first); field copies give the target's a0 first. */
int func_000014A0(int h) {
    Vec4 eye;
    Vec4 at;
    Vec4 up;
    int r;

    h = ehsys_B56890DA(h, 0x710, 0x778);
    h = ehsys_0939B6BE(h, 0x800, 0x800, 0x1E0, 0x110);
    h = ehsys_758DE09D(h, 0xFFFF, 0);
    h = ehsys_5CBB9E43(h, 0, 2, 3, 0, 0);
    h = ehsys_4175E3DB(h, 4);
    h = ehsys_20560342(h, 0, 0, 0x1E0, 0x110);
    h = ehsys_F462EE6E(h, 2);
    h = ehsys_C8DF61CD(h, 6, 0, 0xFF);
    h = ehsys_F462EE6E(h, 0);
    h = ehsys_348F1A33(h, 7);
    h = ehsys_F462EE6E(h, 1);
    h = ehsys_A36635DA(h, 1);
    h = ehsys_F462EE6E(h, 5);
    h = ehsys_F462EE6E(h, 9);
    h = ehsys_5C1AEE8E(h, 1);
    h = ehsys_F462EE6E(h, 8);
    h = ehsys_23E69878(h, -1);
    h = ehsys_917CC4D5(h, 0x404040, 50.0f, 100.0f);
    h = ehsys_4175E3DB(h, 7);
    ehsys_4F22C9AA(0);
    ehsys_9EFDC9B9();
    ehsys_8CA1052F(s_proj_14A0[1], s_proj_14A0[0], 5.0f, 400.0f);
    h = ehsys_1D98951B(h, 0xFF000000);
    h = ehsys_399AC68A(h, 0);
    h = ehsys_42110BF9(h, 0);
    h = ehsys_C62F9EAC(h, 0x17);
    h = ehsys_E6E4401F(h, 1, 1);
    h = ehsys_D6E0AA6A(h, -1);
    h = ehsys_D71FA940(h);
    r = ehsys_94982653(h, 0);
    ehsys_4F22C9AA(1);
    up.x = s_cam_up.x;
    up.y = s_cam_up.y;
    up.z = s_cam_up.z;
    up.w = s_cam_up.w;
    at.x = s_cam_at.x;
    at.y = s_cam_at.y;
    at.z = s_cam_at.z;
    at.w = s_cam_at.w;
    eye.x = s_cam_eye.x;
    eye.y = s_cam_eye.y;
    eye.z = s_cam_eye.z;
    eye.w = s_cam_eye.w;
    ehsys_E2A5CE8D(&eye, &at, &up);
    ehsys_D1C6F209();
    ehsys_4F22C9AA(2);
    ehsys_9EFDC9B9();
    return r;
}

/* func_00001748 -- main state machine: set up the message window parameters for `text`.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00001748(int text) {
    MsgParam *p = &D_0000B3D0.param58C;

    ehsys_memset(p, 0, sizeof(MsgParam));
    p->f00 = 1;
    p->f14 = 1;
    p->f64 = 1;
    p->f68 = 1;
    p->f3C = 1;
    p->f08 = 2;
    p->f24 = -1;
    p->f54 = -1;
    p->f50 = 13;
    p->f4C = 8;
    p->f4E = 8;
    p->f48 = 10;
    p->f4A = 10;
    p->f18 = 450;
    p->text = text;
    p->f40 = 0;
    ehsys_50B8DF28(D_0000B3D0.box540, p);
    p->x = 240.0f - p->w * 0.5f;
    p->y = 136.0f - p->h * 0.5f;
    ehsys_D7155BDE(D_0000B3D0.box540, p);
    ehsys_2AE3C0AD(0x94CB);
}

/* func_00001864 -- pick a pseudo-random bonus card id (3600..9214) seeded by a checksum of
 * the disc's volume descriptor (disc id at +0x373 and the path table
 * bytes); skips invalid/unavailable cards and the fixed reward cards.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00001864(unsigned char *save) {
    int sum1 = 0;
    int sum2;
    int i;
    int n;
    int id;

    for (i = 0; i < 10; i++) {
        sum1 += save[0x373 + i];
    }
    n = __builtin_allegrex_min(__builtin_allegrex_max(*(int *) (save + 0x84), 0x40), 0x774);
    sum2 = 0;
    for (i = 0; i < n; i++) {
        sum2 += save[0x8C + i];
    }
    ehsys_5305D550(sum1 + sum2 + 1);
    i = 0;
    do {
        id = (ehsys_D4DE5DD8() >> 2) % 5615 + 3600;
        if (ehsys_28F7224B(id)) {
            if (ehsys_67312719(id) || id == 0x1445 || id == 0x1387 || id == 0x2352 || id == 0x1AB2 || id == 0x1388 || id == 0x22BC || id == 0xEA8 || id == 0x1386 || id == 0x22A0 || id == 0xE42 || id == 0xE12 || id == 0x22A1 || id == 0xECE || id == 0xE62 || id == 0x2188 || id == 0x2189 || id == 0x218A || id == 0x218B) {
            } else {
                goto found;
            }
        }
        i++;
    } while (i < 10000);
    id = 0x1366;
found:
    return id;
}

/* func_00001A1C -- state 0: wait until the archive heap is ready.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001A1C(void) {
    UmdMain *m = &D_0000B3D0;

    if (func_000036B0()) {
        m->state = 1;
    }
}

/* func_00001A54 -- state 1: load archive slot 0 (language archive), continue in
 * func_00001A88 once loaded.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001A54(void) {
    func_0000371C(0, func_00001A88, 0);
    D_0000B3D0.state = 2;
}

/* func_00001A88 -- archive-loaded callback: set up the two animations and the card model.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001A88(void) {
    D_0000B3D0.state = 3;
    D_0000B3D0.fx_trans = func_000037E4(7);
    ehsys_FDAFCF3A(D_0000B3D0.fx_trans, func_000037E4(8));
    ehsys_42144DB8(D_0000B3D0.fx_trans, 0);
    ehsys_memset(D_0000B3D0.fx_trans_work, 0, sizeof(D_0000B3D0.fx_trans_work));
    D_0000B3D0.fx_circle = func_000037E4(9);
    ehsys_FDAFCF3A(D_0000B3D0.fx_circle, func_000037E4(10));
    ehsys_42144DB8(D_0000B3D0.fx_circle, 0);
    ehsys_memset(D_0000B3D0.fx_circle_work, 0, sizeof(D_0000B3D0.fx_circle_work));
    func_000001C4();
    ehsys_D470D0B2(1, 0xFF000000, 16);
}

/* func_00001B58 -- state 3: start the disc check and show the "insert disc" prompt
 * (message 8).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001B58(void) {
    func_00002A38(0);
    D_0000B3D0.state = 4;
    func_00001748(ehsys_E58C0FDC(func_000037E4(0), 8));
}

/* func_00001B9C -- state 4: run the disc check and react to its result.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001B9C(void) {
    UmdMain *m = &D_0000B3D0;
    int r;

    ehsys_F5E3080C();
    r = func_00002A60();
    switch (r) {
    case 2:
        ehsys_FA53F08C(m->box540, &m->param58C);
        m->state = 16;
        m->ret_state = 4;
        break;
    case 8:
        func_00002A58();
        m->state = 3;
        break;
    case 7:
        m->disc = func_00002EC8();
        m->state = 7;
        break;
    }
    switch (r) {
    case 3:
    case 4:
    case 5:
        m->show_circle = 1;
        break;
    default:
        m->show_circle = 0;
        break;
    }
}

/* func_00001C7C -- state 5: ask for the disc again (message 7).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001C7C(void) {
    func_00001748(ehsys_E58C0FDC(func_000037E4(0), 7));
    func_00002A38(1);
    D_0000B3D0.state = 6;
}

/* func_00001CC0 -- state 6: run the disc check and react to its result.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001CC0(void) {
    UmdMain *m = &D_0000B3D0;
    int r;

    r = func_00002A60();
    switch (r) {
    case 8:
        func_00002A58();
        m->state = 3;
        break;
    case 7:
        m->disc = func_00002EC8();
        ehsys_FA53F08C(m->box540, &m->param58C);
        m->state = 16;
        if (m->n_list > 0) {
            m->ret_state = 7;
        } else {
            m->ret_state = 14;
        }
        break;
    }
    switch (r) {
    case 3:
    case 4:
    case 5:
        m->show_circle = 1;
        break;
    default:
        m->show_circle = 0;
        break;
    }
}

/* func_00001D98 -- state 7: grant the rewards of the identified disc (see the disc table
 * in the header); an already used/unknown disc gets one random card.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001D98(void) {
    UmdMain *m = &D_0000B3D0;
    unsigned char *save;
    int fallback;
    unsigned char *name;

    save = func_00002ED4();
    fallback = 0;
    switch (D_0000B3D0.disc) {
    default:
    case 0:
        if (m->n_list > 0) {
            m->state = 12;
        } else {
            m->state = 14;
        }
        break;
    case 1:
        if (!ehsys_D79EAEBD()) {
            ADD_CARD(0xE42);
            ADD_CARD(0xE12);
            ADD_CARD(0x129E);
            ADD_CARD(0x22A1);
            ehsys_D907D31A(9, 1);
            ehsys_48B2E54A(0x3A, 2);
            ehsys_5A6B0741(10000);
            if (ehsys_3FD815F4() && ehsys_62A243C4() && ehsys_5ABF64AE()) {
                func_00002884();
            }
            ehsys_6E04A2B5();
            m->state = 8;
        } else {
            fallback = 1;
        }
        break;
    case 2:
        if (!ehsys_5ABF64AE()) {
            ADD_CARD(0xEA8);
            ADD_CARD(0x1386);
            ehsys_6DF8A193();
            ADD_CARD(0xFA7);
            ADD_CARD(0x22A0);
            ehsys_D907D31A(11, 1);
            ehsys_48B2E54A(0x3A, 2);
            ehsys_5A6B0741(10000);
            if (ehsys_3FD815F4() && ehsys_62A243C4() && ehsys_D79EAEBD()) {
                func_00002884();
            }
            ehsys_C15BF702();
            m->state = 8;
        } else {
            fallback = 1;
        }
        break;
    case 3:
        if (!ehsys_62A243C4()) {
            ADD_CARD(0x1AB2);
            ADD_CARD(0x1388);
            ehsys_6DF8A193();
            ADD_CARD(0xFF8);
            ADD_CARD(0x22BC);
            ehsys_D907D31A(7, 1);
            ehsys_48B2E54A(0x3A, 2);
            ehsys_5A6B0741(10000);
            if (ehsys_3FD815F4() && ehsys_5ABF64AE() && ehsys_D79EAEBD()) {
                func_00002884();
            }
            ehsys_7A8E41CA();
            m->state = 8;
        } else {
            fallback = 1;
        }
        break;
    case 4:
        if (!ehsys_3FD815F4()) {
            ADD_CARD(0x1445);
            ADD_CARD(0x1387);
            ehsys_6DF8A193();
            ADD_CARD(0xFC9);
            ADD_CARD(0x2352);
            ehsys_D907D31A(3, 1);
            ehsys_48B2E54A(0x3A, 2);
            ehsys_5A6B0741(10000);
            if (ehsys_62A243C4() && ehsys_5ABF64AE() && ehsys_D79EAEBD()) {
                func_00002884();
            }
            ehsys_5ED6B8AE();
            m->state = 8;
        } else {
            fallback = 1;
        }
        break;
    case 5:
        fallback = 1;
        break;
    }
    if (fallback) {
        name = save + 0x373;
        if (ehsys_D0FBFC51(name)) {
            m->state = 10;
        } else {
            m->list[m->n_list].num = 3;
            m->list[m->n_list].id = func_00001864(save);
            m->state = 8;
            m->n_list++;
            m->n_cards++;
            ehsys_83B39FA9(name);
        }
    }
}

static const float s_proj_14A0[2] = {1.7647059f, 0.383972f};

/* func_00002450 -- state 8: show the reward list.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002450(void) {
    func_0000042C(D_0000B3D0.n_cards);
    D_0000B3D0.n_cards = 0;
    D_0000B3D0.state = 9;
}

/* func_00002484 -- state 9: wait for the card viewer.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002484(void) {
    UmdMain *m = &D_0000B3D0;

    func_0000028C();
    if (!func_00000614()) {
        m->state = 3;
    }
}

/* func_000024C4 -- state 10: message box 1 -- shown when ehsys_D0FBFC51 reports that this
 * disc id was already used (probably "no more cards from this disc").
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000024C4(void) {
    func_00000DF0(&D_0000B3D0.box41C, ehsys_E58C0FDC(func_000037E4(0), 1), 1, 1, 1);
    D_0000B3D0.state = 11;
}

/* func_00002514 -- state 11: wait for that message box.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002514(void) {
    UmdMain *m = &D_0000B3D0;

    switch (func_00000FD0(&D_0000B3D0.box41C)) {
    case 1:
        m->state = 3;
        break;
    }
}

/* func_0000255C -- state 12: show reward `cur` and add its copies to the collection; some
 * cards also get a flag (ehsys_79B05ABC) and unlock something through ehsys_EFADAB40.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). ehsys_79B05ABC takes the
 * card id as an `unsigned short` (EBOOT 0x53084: `andi a0,a0,0xFFFF`, then sets bit 3
 * of the card record), it is not void. Testing and passing the u16 `card` while the
 * other calls use the int copy `id` is what keeps the value in $a0 for the tests and
 * the call, with the `$s0 = $a0` copy in the first delay slot. */
void func_0000255C(void) {
    UmdMain *m = &D_0000B3D0;
    int i;
    int id;
    int n;
    unsigned short card;

    func_000004F4(D_0000B3D0.list[D_0000B3D0.cur].id, D_0000B3D0.list[D_0000B3D0.cur].num);
    D_0000B3D0.state = 13;
    for (i = 0; i < m->list[m->cur].num; i++) {
        ehsys_C4FA973E(m->list[m->cur].id, 1);
        card = m->list[m->cur].id;
        id = card;
        if (card == 0x1386 || card == 0xFA7 || card == 0xFC9 || card == 0x129E || card == 0x1296 ||
            card == 0xFF8 || card == 0x1388) {
            ehsys_79B05ABC(card);
            ehsys_EFADAB40(id, 1);
            n = 0;
            switch (id) {
            case 0x1386:
                n = 1;
                break;
            case 0xFA7:
                n = 4;
                break;
            case 0xFC9:
                n = 4;
                break;
            case 0x1296:
                n = 1;
                break;
            case 0xFF8:
                n = 4;
                break;
            case 0x1388:
                n = 1;
                break;
            case 0x129E:
                if (ehsys_get_language() == 0) {
                    n = 3;
                } else {
                    n = 1;
                }
                break;
            }
            ehsys_EFADAB40(id, n);
        }
    }
}

/* func_000026F0 -- state 13: next reward, or state 14 when done.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000026F0(void) {
    UmdMain *m = &D_0000B3D0;

    func_0000028C();
    if (!func_00000614()) {
        m->cur++;
        if (m->cur < m->n_list) {
            m->state = 12;
        } else {
            m->state = 14;
        }
    }
}

/* func_00002754 -- state 14: message box 2 with a choice.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002754(void) {
    func_00000DF0(&D_0000B3D0.box41C, ehsys_E58C0FDC(func_000037E4(0), 2), 3, 2, 0);
    D_0000B3D0.state = 15;
}

/* func_000027A4 -- state 15: 2 = quit (fade out, state 17), 3/-2 = check another disc.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000027A4(void) {
    UmdMain *m = &D_0000B3D0;

    switch (func_00000FD0(&D_0000B3D0.box41C)) {
    case 2:
        m->state = 17;
        ehsys_5998BFD7(1, 0xFF000000, 16);
        break;
    case -2:
    case 3:
        m->n_list = 0;
        m->cur = 0;
        m->state = 3;
        break;
    }
}

/* func_0000282C -- state 16: close the message window, return to the saved state.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_0000282C(void) {
    UmdMain *m = &D_0000B3D0;

    if (ehsys_8867335A(D_0000B3D0.box540, &D_0000B3D0.param58C) == 4) {
        ehsys_834A6E13(m->box540, &m->param58C);
        m->state = m->ret_state;
    }
}

/* func_00002884 -- bonus for owning all four previous games: two more cards.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002884(void) {
    D_0000B3D0.list[D_0000B3D0.n_list].num = 3;
    D_0000B3D0.list[D_0000B3D0.n_list].id = 0xECE;
    D_0000B3D0.n_list++;
    D_0000B3D0.n_cards++;
    D_0000B3D0.list[D_0000B3D0.n_list].num = 3;
    D_0000B3D0.list[D_0000B3D0.n_list].id = 0xE62;
    D_0000B3D0.n_list++;
    D_0000B3D0.n_cards++;
}

/* func_0000293C -- UMD init: create and register the UMD callback, activate the drive.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_0000293C(void) {
    ehsys_memset(&D_0000BAC0, 0, sizeof(UmdDisc));
    D_0000BAC0.cb = sceKernelCreateCallback(D_00009350, func_0000327C, 0);
    sceUmdRegisterUMDCallBack(D_0000BAC0.cb);
    sceUmdActivate(1, D_00009354);
    sceImposeSetUMDPopup(0);
    ehsys_30FECAEF(D_0000BAC0.box134, func_000038B8());
}

/* func_000029BC -- UMD teardown: prohibit disc replacement again, unregister the callback.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000029BC(void) {
    UmdDisc *d = &D_0000BAC0;

    func_00002A58();
    ehsys_5DF04F49(D_0000BAC0.box134);
    while (1) {
        if (sceUmdReplaceProhibit() >= 0) {
            break;
        }
        ehsys_frame_sync();
    }
    sceImposeSetUMDPopup(1);
    if (d->cb) {
        sceUmdUnRegisterUMDCallBack(d->cb);
        sceKernelDeleteCallback(d->cb);
        d->cb = 0;
    }
}

/* func_00002A38 -- start a disc check (`ask_first` = skip the first prompt).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002A38(int arg) {
    D_0000BAC0.disc = 0;
    D_0000BAC0.f008 = arg;
    D_0000BAC0.state = 1;
    sceUmdReplacePermit();
}

/* func_00002A58 -- empty.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002A58(void) {
}

/* func_00002A60 -- disc check state machine (D_0000BAC0.state); returns the state.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00002A60(void) {
    UmdDisc *d = &D_0000BAC0;
    int *pad;
    int btn0;
    int btn1;
    int res;

    pad = ehsys_F5E3080C();
    btn0 = ehsys_get_button_code(0) & 0xFFFF;
    btn1 = ehsys_get_button_code(1) & 0xFFFF;
    res = 0;
    switch (D_0000BAC0.state) {
    case 1:
        if ((pad[1] & (btn0 & 0xFFFF)) || d->f008) {
            func_00003550(ehsys_E58C0FDC(func_000037E4(0), 7));
            d->state = 2;
        } else if (pad[1] & (btn1 & 0xFFFF)) {
            d->f008 = 1;
            ehsys_2AE3C0AD(0x94C5);
        }
        break;
    case 2:
        d->state = 3;
        break;
    case 3:
        res = 0;
        if (func_00002EE0(&res)) {
            d->state = 4;
        } else {
            switch (res) {
            case 0:
                break;
            case 1:
                ehsys_834A6E13(d->box134, &d->param180);
                d->state = 8;
                ehsys_2AE3C0AD(0x94C5);
                break;
            case 2:
                ehsys_834A6E13(d->box134, &d->param180);
                func_00000DF0(&d->box10, ehsys_E58C0FDC(func_000037E4(0), 10), 1, 1, 1);
                d->state = 6;
                break;
            case 3:
                break;
            case 4:
                ehsys_834A6E13(d->box134, &d->param180);
                func_00000DF0(&d->box10, ehsys_E58C0FDC(func_000037E4(0), 5), 1, 1, 1);
                d->state = 6;
                break;
            }
        }
        break;
    case 4:
        res = 0;
        if (!func_00003014(&res)) {
            switch (res) {
            case 0:
            case 1:
                ehsys_834A6E13(d->box134, &d->param180);
                d->state = 8;
                break;
            case 2:
                ehsys_834A6E13(d->box134, &d->param180);
                func_00000DF0(&d->box10, ehsys_E58C0FDC(func_000037E4(0), 10), 1, 1, 1);
                d->state = 6;
                break;
            case 4:
                ehsys_834A6E13(d->box134, &d->param180);
                func_00000DF0(&d->box10, ehsys_E58C0FDC(func_000037E4(0), 5), 1, 1, 1);
                d->state = 6;
                break;
            }
        } else {
            ehsys_834A6E13(d->box134, &d->param180);
            if (d->f008) {
                if (d->disc == 0) {
                    d->state = 7;
                } else {
                    func_00000DF0(&d->box10, ehsys_E58C0FDC(func_000037E4(0), 9), 2, 1, 0);
                    d->state = 5;
                }
            } else {
                d->state = 7;
            }
        }
        break;
    case 5:
        switch (func_00000FD0(&d->box10)) {
        case 1:
            func_00003550(ehsys_E58C0FDC(func_000037E4(0), 7));
            d->state = 3;
            break;
        case 4:
        case -2:
            d->state = 8;
            break;
        }
        break;
    case 6:
        switch (func_00000FD0(&d->box10)) {
        case 1:
            d->state = 8;
            break;
        }
        break;
    case 7:
    case 8:
        break;
    }
    if (d->state == 7 && d->disc == 0) {
        while (1) {
            if (sceUmdReplaceProhibit() >= 0) {
                break;
            }
            ehsys_frame_sync();
        }
    }
    return d->state;
}

static const Vec4 s_cam_eye __attribute__((aligned(16))) = {0.0f, -5.0f, 5.0f, 1.0f};
static const Vec4 s_cam_at __attribute__((aligned(16))) = {0.0f, 0.0f, 0.0f, 1.0f};
static const Vec4 s_cam_up __attribute__((aligned(16))) = {0.0f, 1.0f, 0.0f, 0.0f};

/* func_00002E78 -- draw the disc-check message box.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002E78(void) {
    int h;

    h = ehsys_DFCA450B(4, 0);
    h = func_00001018(&D_0000BAC0.box10, h);
    ehsys_14BFFB01(&h, D_0000BAC0.box134);
    ehsys_1EC5342B(h);
}

/* func_00002EC8 -- identified disc (0 = TF5 .. 4 = TF1, 5 = unknown).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00002EC8(void) {
    return D_0000BAC0.disc;
}

/* func_00002ED4 -- buffer holding sector 16 of the inserted disc.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
unsigned char *func_00002ED4(void) {
    return D_0000BCC0;
}

/* func_00002EE0 -- wait for a disc to be inserted and spun up (300-frame timeouts).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00002EE0(int *res) {
    int *pad;
    int btn;
    unsigned int n_absent;
    unsigned int n_busy;
    int present;
    int stat;

    pad = ehsys_F5E3080C();
    btn = ehsys_get_button_code(1) & 0xFFFF;
    n_absent = 0;
    n_busy = 0;
    while (1) {
        present = sceUmdCheckMedium();
        stat = sceUmdGetDriveStat();
        if (!!(pad[1] & btn)) {
            *res = 1;
            return 0;
        }
        if ((ehsys_EB263A38() & 0xB0000) == 0) {
            if (present == 0) {
                if (++n_absent >= 300) {
                    *res = 4;
                    return 0;
                }
            } else if (stat & 0x20) {
                *res = 0;
                return 1;
            } else if (stat == 10) {
                if (++n_busy >= 300) {
                    *res = 2;
                    return 0;
                }
            }
        }
        ehsys_frame_sync();
    }
}

/* func_00003014 -- read sector 16 of "umd1:" and identify the disc by its id.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00003014(int *res) {
    UmdDisc *d = &D_0000BAC0;
    int fd;
    char id[16];

    fd = func_00003284(D_0000935C, 1, 0, res);
    if (fd < 0) {
        return 0;
    }
    if (*res != 0) {
        return 0;
    }
    if (sceIoLseek(fd, 16, 0) < 0) {
        sceIoClose(fd);
        return 0;
    }
    if (func_000033DC(fd, D_0000BCC0, 1, res) < 0) {
        sceIoClose(fd);
        return 0;
    }
    if (*res != 0) {
        sceIoClose(fd);
        return 0;
    }
    sceIoClose(fd);
    ehsys_memcpy(id, &D_0000BCC0[0x373], 10);
    id[10] = 0;
    if (ehsys_strcmp(id, D_00009364) == 0) {
        d->disc = 0;
    } else if (ehsys_strcmp(id, D_00009370) == 0 || ehsys_strcmp(id, D_0000937C) == 0 ||
               ehsys_strcmp(id, D_00009388) == 0) {
        d->disc = 1;
    } else if (ehsys_strcmp(id, D_00009394) == 0 || ehsys_strcmp(id, D_000093A0) == 0 ||
               ehsys_strcmp(id, D_000093A0) == 0) {
        d->disc = 2;
    } else if (ehsys_strcmp(id, D_000093AC) == 0 || ehsys_strcmp(id, D_000093B8) == 0 ||
               ehsys_strcmp(id, D_000093C4) == 0) {
        d->disc = 3;
    } else if (ehsys_strcmp(id, D_000093D0) == 0 || ehsys_strcmp(id, D_000093DC) == 0 ||
               ehsys_strcmp(id, D_000093E8) == 0) {
        d->disc = 4;
    } else {
        d->disc = 5;
    }
    return 1;
}

/* func_0000327C -- UMD callback (does nothing).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_0000327C(int count, int arg, void *common) {
    return 0;
}

/* func_00003284 -- sceIoOpen that waits for the drive and retries on transient errors.
 * The four retry tests followed by an unconditional loop-back are only
 * reproduced when something non-trivial follows the last test: with plain
 * `continue`s MWCC folds the final `fd == 0x80010016` branch because both of
 * its successors are the loop head (every if/switch/goto/do-while phrasing
 * folds it). The permuter found that an EMPTY short-circuit conditional
 * (`if (a && b) {}` -- any operands) keeps it; the original most likely had
 * a compiled-out debug print/assert macro there.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00003284(const char *path, int flags, int mode, int *res) {
    int *pad;
    int btn;
    int fd;
    int info[2];

    pad = ehsys_F5E3080C();
    btn = ehsys_get_button_code(1) & 0xFFFF;
    while (1) {
        ehsys_frame_sync();
        if (!!(pad[1] & btn)) {
            *res = 1;
            break;
        }
        if (sceUmdCheckMedium() == 0) {
            *res = 4;
            break;
        }
        if (sceUmdGetDriveStat() & 0x20) {
            info[0] = 8;
            fd = sceUmdGetDiscInfo(info);
            if (!(info[1] & 0x10)) {
                *res = 2;
                break;
            }
            fd = sceIoOpen(path, flags, mode);
            if (fd >= 0) {
                *res = 0;
                break;
            }
            if (fd == 0x8001B002) {
                continue;
            }
            if (fd == 0x80010013) {
                continue;
            }
            if (fd == 0x8001000C) {
                continue;
            }
            if (fd == 0x80010016) {
                continue;
            }
            /* empty short-circuit test: see the function comment */
            if (fd && fd) {
            }
        }
    }
    return fd;
}

/* func_000033DC -- sceIoRead that waits for the drive and retries on transient errors.
 * Same shape as func_00003284, same empty short-circuit test after the
 * last retry code.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000033DC(int fd, void *buf, int size, int *res) {
    int *pad;
    int btn;
    int ret;
    int info[2];

    pad = ehsys_F5E3080C();
    btn = ehsys_get_button_code(1) & 0xFFFF;
    while (1) {
        ehsys_frame_sync();
        if (!!(pad[1] & btn)) {
            *res = 1;
            break;
        }
        if (sceUmdCheckMedium() == 0) {
            *res = 4;
            break;
        }
        if (sceUmdGetDriveStat() & 0x20) {
            info[0] = 8;
            ret = sceUmdGetDiscInfo(info);
            if (!(info[1] & 0x10)) {
                *res = 2;
                break;
            }
            ret = sceIoRead(fd, buf, size);
            if (ret >= 0) {
                *res = 0;
                break;
            }
            if (ret == 0x80010016) {
                continue;
            }
            if (ret == 0x80010013) {
                continue;
            }
            if (ret == 0x8001B003) {
                continue;
            }
            if (ret == 0x8001000E) {
                continue;
            }
            if (ret == 0x80210002) {
                continue;
            }
            if (ret == 0x8001B001) {
                continue;
            }
            /* empty short-circuit test: see func_00003284 */
            if (ret && ret) {
            }
        }
    }
    return ret;
}

/* func_00003550 -- disc check: set up the message window parameters for `text`.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003550(int text) {
    MsgParam *p = &D_0000BAC0.param180;

    ehsys_memset(p, 0, sizeof(MsgParam));
    p->f00 = 1;
    p->f14 = 1;
    p->f3C = 1;
    p->f08 = 2;
    p->f24 = -1;
    p->f54 = -1;
    p->f50 = 13;
    p->f4C = 8;
    p->f4E = 8;
    p->f48 = 10;
    p->f4A = 10;
    p->text = text;
    p->f40 = 0;
    ehsys_50B8DF28(D_0000BAC0.box134, p);
    p->x = 240.0f - p->w * 0.5f;
    p->y = 136.0f - p->h * 0.5f;
    ehsys_2AE3C0AD(0x94CB);
}

/* func_0000364C -- resource init: heap and archive work area.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_0000364C(void) {
    D_00290CE4.heap = 0;
    D_00290CE4.arc = 0;
    D_00290CE4.loading = -1;
    D_00290CE4.heap = ehsys_1856E536(D_00010CC0, 0x180000);
    D_00290CE4.arc = ehsys_291D6262(D_00190CC0, 0x100000, 0);
}

/* func_000036B0 -- archive ready?
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000036B0(void) {
    return ehsys_2717C28C(D_00290CE4.arc) != 0;
}

/* func_000036D4 -- resource teardown.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000036D4(void) {
    Loader *l = &D_00290CE4;

    if (D_00290CE4.heap) {
        ehsys_1078C73B(l->heap);
        l->heap = 0;
    }
    return 1;
}

/* func_00003714 -- empty.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003714(void) {
}

/* func_0000371C -- load archive slot `slot` (slot 0 = language-dependent
 * basic_lang_*.ehp), then call `cb(handle, arg)`.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_0000371C(int slot, void (*cb)(void), int arg) {
    Loader *l = &D_00290CE4;
    ResFile *f = &D_00290CC0[slot];
    const char *name;

    if (f->handle) {
        if (cb) {
            ((void (*)(int, int)) cb)(f->handle, arg);
        }
        return;
    }
    if (slot == 0) {
        name = D_000093F4[ehsys_get_language()];
    } else {
        name = f->name;
    }
    func_000038D0(name, f->handle, 1);
    l->loading = slot;
    l->cb = (void (*)(int, int)) cb;
    l->cb_arg = arg;
}

/* func_000037E4 -- get resource `id` from the static resource table.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000037E4(int id) {
    ResFile *r = &D_000094B4[id];

    return func_00003804(r->name, r->handle);
}

/* func_00003804 -- get resource `name` from archive slot `slot`; .ehp files are relocated.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00003804(const char *name, int slot) {
    ResFile *f = &D_00290CC0[slot];
    int r;

    if (f->handle == 0) {
        return 0;
    }
    r = ehsys_C6C15111(f->handle, name, 0);
    if (ehsys_strcmp(name + (ehsys_strlen(name) - 3), D_000096AC) == 0) {
        ehsys_4B0DABFA(0, r);
    }
    return r;
}

/* func_00003898 -- heap alloc (registered with ehsys_EBD1986B).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00003898(int size) {
    return ehsys_41AABF28(D_00290CE4.heap, size);
}

/* func_000038A8 -- heap free (registered with ehsys_EBD1986B).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000038A8(int p) {
    return ehsys_20E340D9(D_00290CE4.heap, p);
}

/* func_000038B8 -- heap handle.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000038B8(void) {
    return D_00290CE4.heap;
}

/* func_000038C4 -- archive handle.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000038C4(void) {
    return D_00290CE4.arc;
}

/* func_000038D0 -- start loading "database/umd_replace/<name>" (completion: func_00003970).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000038D0(const char *name, int unused, int async) {
    char path[0x100];
    int size;
    int buf;

    ehsys_sprintf(path, D_000096B0, name);
    size = ehsys_D2A768F4(3, path);
    buf = func_00003898(size);
    if (async) {
        ehsys_8288EEBC(3, path, buf, size, func_00003970, buf);
    } else {
        ehsys_8288EEBC(3, path, buf, size, func_00003970, 0);
    }
}

/* func_00003970 -- file-load completion: store the handle in the slot, run the callback.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00003970(int buf, int size) {
    Loader *l = &D_00290CE4;
    ResFile *f = &D_00290CC0[D_00290CE4.loading];

    if (size > 0) {
        if (buf) {
            ehsys_4B0DABFA(0, buf);
        }
        f->handle = buf;
        if (l->cb) {
            l->cb(buf, l->cb_arg);
        }
        l->loading = -1;
        return 1;
    }
    return 0;
}

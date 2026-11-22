/*
 * rel_gallery.prx - reconstructed code (matching decompilation)
 *
 * The movie gallery: a grid of thumbnails, and playback of the opening
 * (TF5_opening / TF5_opening_USA), the staff roll (TF5_staffroll /
 * TF5_staffroll_USA) and the per-character clips (SSD_s, SSD_a, ...).
 *
 * Compiler:     MWCC 1.3 SP7 (mwccpsp_3.0.1_219), -O4,s -sdatathreshold 0
 * Verification: scripts/mwcc_build.sh + scripts/mwcc_diff.py, and
 *               `make MODULE=rel_gallery SRC=1`.
 * Status:       97 of 98 functions byte-identical; func_00000814 differs by
 *               the position of one instruction (see its comment), so
 *               `make SRC=1` builds the right size (51332 bytes) and differs
 *               only in those 6 bytes (file 0x87C..0x883).
 *
 * THIS MODULE WAS WRITTEN IN C++, AND THIS FILE IS COMPILED AS C++
 * (`#pragma cplusplus on`, everything inside `extern "C"` so the names stay
 * func_XXXXXXXX). Evidence: vtables in .data (D_0000A7C8 ItemList,
 * D_0000A7E0 Item, D_0000A7F8 Scene, D_0000A804 Player, D_0000A810 Task),
 * each {0, 0, dtor, virtuals...}; deleting destructors taking a `short`
 * flag (`seh` + `blez`) that call operator delete (ehsys_9455886E);
 * `new` through ehsys_57345021(size, heap); and the MWCC runtime's
 * __destroy_arr / __construct_array linked in (func_00000000/0000006C,
 * compiled with `optimize_for_size off`, i.e. other flags than the game).
 * Why C++ mode is needed: a virtual call loads its target into $t9
 * (`lw t9,vptr(a0); lw t9,slot(t9); jalr t9`); a call through a C function
 * pointer always uses $v0. So the virtual calls and `delete`s go through
 * small C++ "view" classes (SceneV, ItemV, ItemListV, PlayerV, TaskV) whose
 * virtual functions are declared but never defined -- MWCC then emits no
 * vtable and no .data. The objects themselves are plain structs with an
 * explicit `vptr` member, and constructors/destructors are ordinary
 * functions that store the D_0000A7xx vtable address themselves.
 *
 * Classes: Scene (0x224, the gallery state machine, global D_0030A854),
 * ItemList (0x250C, 157 thumbnail tiles on two pages of 93 / 64), Item
 * (0x3C, one tile), Player (0x1A4: an async file loader, despite the name
 * it does not play movies), Task (0x20, an update/draw list node; root
 * D_00755884), LoadReq (0xC, one async load).
 *
 * Globals. .data ends at 0xA840; .bss holds the module-registration block
 * D_0000A840, the "running" flag D_0000A84C, a 3 MiB heap arena D_0000A850,
 * then the heap handle D_0030A850, the scene D_0030A854/D_0030A858, a 4 MiB
 * movie work buffer D_0030A880, an image work buffer D_0070A880, and
 * D_00755880/D_00755884. Most of these are addressed with BAKED absolute
 * constants (no relocation); the Makefile PROVIDEs them from the name.
 *
 * Movie playback (func_000024E8): ehsys_70CB33BE(0,1) + wait for
 * ehsys_DCA0DCE1, sprintf "movie/%s.pmf", ehsys_68728B41(path, 3, name)
 * (make device path), ehsys_50EA4D71(buf, 0x400000) (movie work buffer),
 * ehsys_B48A11FB(path, 0xF3F9) (play, blocking), ehsys_245526EE() (end) --
 * the same sequence as rel_movie_viewer.
 *
 * No Shift-JIS text in this module (scripts/sjis_strings.py finds none).
 *
 * Conventions (see src/rel_movie_viewer.c and docs/06):
 *  - globals keep their D_XXXXXXXX placeholder names; proposed names are in
 *    config/symbols/rel_gallery.extra.txt and in the comments.
 *  - no string literals and no initialised data: the object must not emit
 *    .data (the shipped .data is linked verbatim). Float constants that the
 *    target loads from .data are `extern const float D_...` (const lets
 *    MWCC schedule the load freely, like a literal-pool load).
 */

#pragma cplusplus on
#pragma exceptions off

extern "C" {

typedef unsigned int size_t;

/* ---- engine imports ---------------------------------------------------- */
extern void ehsys_B4471B5E(void *, void *, void *);
extern void ehsys_C2E59A21(int, void *, void *);
extern void ehsys_frame_sync(void);
extern void ehsys_8060707A(void);
extern void ehsys_13A4081A(int);
extern void ehsys_08813E19(int, int);
extern void ehsys_sceKernelChangeCurrentThreadAttr(int, int);
extern int ehsys_1856E536(void *, int);      /* create heap on (arena, size) */
extern void ehsys_6368BABC(int);             /* make heap current? */
extern void *ehsys_57345021(size_t, int);    /* operator new(size, heap) */
extern void ehsys_9455886E(void *);          /* operator delete */
extern void ehsys_B46025DF(int, int *, int *);
extern void ehsys_2F21139D(int);
extern void ehsys_1078C73B(int);             /* destroy heap */
extern int ehsys_20560342(int, int, int, int, int);
extern int ehsys_F462EE6E(int, int);
extern int ehsys_CABEA834(int, unsigned int);
extern int ehsys_FDD168CA(int, int, int, int, int, int, int, int);
extern int ehsys_4175E3DB(int, int);
extern int ehsys_C6C15111(int, char *, int);  /* load a file from an archive? */
extern void ehsys_FDAFCF3A(int, int);
extern int ehsys_DFCA450B(int, int);
extern int ehsys_abs(int);
extern int ehsys_get_language(void);
extern int ehsys_459B889D(int);
extern int ehsys_ED17CBAC(int);
extern void ehsys_6E42DDCD(void);
extern void ehsys_C859D5FE(int);
extern int ehsys_AA9913C7(int);
extern void ehsys_BE3756D0(int);
extern void ehsys_5998BFD7(int, int, int);
extern signed char ehsys_635BB4EE(unsigned char);
extern signed char ehsys_8477BA04(int, int);
extern int ehsys_34A7E8C2(int);
extern int ehsys_99FF9D51(int, int);
extern unsigned char *ehsys_D8A0DB10(void);
extern void ehsys_03E45FFF(void);
extern void ehsys_memcpy(void *, void *, int);
extern struct Pad *ehsys_F5E3080C(void);
extern int ehsys_get_button_code(int);
extern void ehsys_2AE3C0AD(int);             /* play sound effect */
extern void ehsys_D470D0B2(int, int, int);   /* fade */
extern void ehsys_0D3E2FA6(int, int);
extern void ehsys_6367AE39(int);
extern int ehsys_sprintf(char *, const char *, ...);
extern void ehsys_68728B41(char *, int, char *);
extern void ehsys_70CB33BE(int, int);
extern int ehsys_DCA0DCE1(void);
extern void ehsys_50EA4D71(void *, int);     /* movie: set work buffer */
extern void ehsys_B48A11FB(char *, int);     /* movie: play file */
extern void ehsys_245526EE(void);            /* movie: release */
extern void ehsys_memset(void *, int, int);
extern int *ehsys_B2B9FEFA(void);
extern int ehsys_85F89FE8(int);
extern int ehsys_7187E0F0(int, int, int *);
extern int ehsys_89812D8A(int);
extern int ehsys_450CD6C5(void);
extern int ehsys_558B2DCB(int);
extern void ehsys_CE8B1945(int, int);
extern int ehsys_E6E4401F(int, int, int);
extern int ehsys_5CBB9E43(int, int, int, int, int, int);
extern int ehsys_8EA6B1E7(int, int, int, int, int);
extern int ehsys_9EA6989A(int, int);
extern void ehsys_1EC5342B(int);
extern int ehsys_09794DF0(int, int, int);
extern unsigned short ehsys_0D961646(void);
extern void ehsys_B8AD96EA(int);             /* begin text batch */
extern void ehsys_C9D9E989(int);
extern void ehsys_942B03D0(int, int);        /* font size */
extern void ehsys_60B55A50(int);             /* text colour */
extern void *ehsys_E58C0FDC(int, int);       /* string table lookup */
extern void ehsys_06380DFA(unsigned short);
extern int ehsys_A4AFF8E6(void);             /* end text batch */
extern void ehsys_B89D38DC(int, int, int, void *); /* draw string (x<<6, y<<6, w<<6, str) */
extern void ehsys_025FE238(char *, void *);  /* start typing a message */
extern int ehsys_82E739A4(char *);
extern void ehsys_C792FA38(int);
extern void ehsys_BC8E65D7(int, int, int, char *, int);
extern float ehsys_A38AA0E9(float);         /* sinf */
extern void ehsys_878EE1D3(int *);
extern int ehsys_memcmp(void *, void *, int);
extern void ehsys_4B0DABFA(int, void *);     /* fix up a loaded EHP in place */
extern int ehsys_D2A768F4(int, char *);      /* file size */
extern void *ehsys_19AC3ED4(int, int);       /* heap alloc (size, heap) */
extern void *ehsys_41AABF28(int, int);       /* heap alloc (heap, size) */
extern void ehsys_8288EEBC(int, char *, void *, int, int (*)(void *, int), void *); /* async load */
extern void ehsys_5CEDF44D(void *);          /* free */
extern int ehsys_7CBFBEC2(void *, int);      /* unpacked size */
extern void ehsys_C204E9F8(void *, int, void *); /* unpack */
extern int *ehsys_9E72EF23(int, int, int);
extern void ehsys_831C5769(int, int, int);   /* animation: play frame range */
extern char *ehsys_7754CFD2(int);          /* character id -> bust-up file name */

/* ---- MWCC C++ runtime --------------------------------------------------- */

typedef void *(*CtorDtor)(void *, short);

#pragma optimize_for_size off
/* func_00000000 — the MWCC runtime's __destroy_arr(block, dtor, size, n):
 * destroys n elements back to front, calling dtor(p, -1).
 * MATCH 100% */
void func_00000000(void *block, CtorDtor dtor, size_t size, size_t n) {
    char *p;

    for (p = (char *) block + size * n; n > 0; n--) {
        p -= size;
        dtor(p, -1);
    }
}

/* __partial_array_destructor: the runtime's guard object; its destructor
 * is inlined into __construct_array. */
typedef struct {
    void *p;
    size_t size;
    size_t n;
    CtorDtor dtor;
    size_t i;
} PartialArrayDtor;

/* func_0000006C — __construct_array(block, ctor, dtor, size, n): constructs
 * n elements with ctor(p, 1); the guard's destructor (inlined at the end)
 * destroys the constructed ones again if construction stopped early.
 * In the C++ original `pad` is an object with a destructor, so it lives in
 * memory and every call is assumed to clobber it (i is re-read after each
 * ctor/dtor call; p, size, n, dtor are re-read after the loop). In C the
 * same happens once a pointer to the counter is live: through `pi` the
 * struct is address-taken and not split into registers.
 * Like func_00000000 it is compiled for speed (`optimize_for_size off`):
 * the runtime library was built with different flags than the game code.
 * MATCH 100% */
void func_0000006C(void *block, CtorDtor ctor, CtorDtor dtor, size_t size, size_t n) {
    PartialArrayDtor pad;
    size_t *pi;
    char *p;
    char *ptr;

    pi = &pad.i;
    pad.p = block;
    pad.size = size;
    pad.n = n;
    pad.dtor = dtor;
    pad.i = n;
    for (pad.i = 0, p = (char *) block; *pi < n; pad.i++, p += size) {
        ctor(p, 1);
    }
    if (*pi < pad.n && pad.dtor) {
        for (ptr = (char *) pad.p + pad.size * *pi; *pi > 0; pad.i--) {
            ptr -= pad.size;
            pad.dtor(ptr, -1);
        }
    }
}
#pragma optimize_for_size reset

/* ---- classes ------------------------------------------------------------- */

/* MWCC vtable: two header words, then the deleting destructor, then the
 * virtual functions in declaration order. */
typedef struct Vtbl {
    int unk0;
    int unk4;
    void *(*dtor)(void *, short);
    int (*v3)();
    int (*v4)();
    int (*v5)();
    int (*v6)();
    int (*v7)();
} Vtbl;


/* A thumbnail tile (0x3C bytes, vtable D_0000A7E0: dtor func_000007B4,
 * update func_00000408, draw func_00000638, func_00000720). */
typedef struct Item {
    short active;      /* +0x00 */
    short group;       /* +0x02 */
    float x;           /* +0x04 */
    float y;           /* +0x08 */
    float dst_x;       /* +0x0C */
    float dst_y;       /* +0x10 */
    short u;           /* +0x14 texture u */
    short v;           /* +0x16 texture v */
    short move;        /* +0x18 */
    short unk1A;       /* +0x1A */
    signed char delay; /* +0x1C */
    signed char unk1D; /* +0x1D */
    signed char dir;   /* +0x1E */
    char pad1F;
    float t;           /* +0x20 */
    float alpha;       /* +0x24 */
    short frame;       /* +0x28 */
    short frames;      /* +0x2A */
    int unk2C;         /* +0x2C */
    int unk30;         /* +0x30 */
    float unk34;       /* +0x34 */
    Vtbl *vptr;        /* +0x38 */
} Item;

/* model draw parameters for ehsys_7187E0F0 (0x60 bytes) */
typedef struct DrawParam {
    int unk0;           /* +0x00 from *ehsys_B2B9FEFA() */
    int unk4;
    int unk8;
    int flags;          /* +0x0C */
    int unk10;
    float x;            /* +0x14 */
    float y;            /* +0x18 */
    int unk1C;
    float sx;           /* +0x20 */
    float sy;           /* +0x24 */
    int unk28[6];
    int color[4];       /* +0x40 */
    int unk50[4];
} DrawParam;

typedef struct ItemGroup {
    Item *first;
    int count;
} ItemGroup;

/* The thumbnail list (vtable D_0000A7C8: dtor func_00001368,
 * func_00000814 load, func_00000A80 update, func_00000AEC draw). */
/* The movie player (0x1A4 bytes, vtable D_0000A804: dtor func_000017D0). */
/* The file loader (0x1A4 bytes, vtable D_0000A804: dtor func_000017D0):
 * file slots 0..0x5E loaded asynchronously by func_00003D94, plus the
 * "bust-up" character model of the selected clip (func_00004028). */
typedef struct Player {
    void *files[0x5F];      /* +0x000 0 basic.ehp, 1..0x5D event pictures, 0x5E language ehp */
    int pending;            /* +0x17C loads in flight */
    int unk180;             /* +0x180 size of the last event picture */
    Vtbl *vptr;             /* +0x184 */
    int unk188;             /* +0x188 current file index */
    void *packed;           /* +0x18C bust-up archive as loaded */
    void *unpacked;         /* +0x190 ... after ehsys_C204E9F8 */
    int mdl[2];             /* +0x194 bust-up models: 0 body, 1 mouth */
    int packed_size;        /* +0x19C */
    int ready;              /* +0x1A0 cleared while the bust-up loads */
} Player;

/* one async load request (0xC bytes) */
typedef struct LoadReq {
    void **slot;
    Player *owner;
    void *buf;
} LoadReq;

typedef struct Pad {
    int unk0;
    int buttons;      /* +0x04 */
    int unk8;
    int trigger;      /* +0x0C edge-triggered buttons */
} Pad;

typedef struct ItemList {
    Item items[0x9D];       /* +0x0000 */
    ItemGroup groups[2];    /* +0x24CC */
    int unk24DC;            /* +0x24DC */
    int tex24E0;            /* +0x24E0 */
    int tex24E4;            /* +0x24E4 */
    int tex24E8;            /* +0x24E8 */
    int tex24EC;            /* +0x24EC */
    int tex24F0;            /* +0x24F0 */
    int tex24F4;            /* +0x24F4 */
    int tex24F8;            /* +0x24F8 */
    short unk24FC;          /* +0x24FC */
    short unk24FE;          /* +0x24FE */
    short scroll[2];        /* +0x2500 first visible row, per page */
    short prev_scroll[2];   /* +0x2504 */
    Vtbl *vptr;             /* +0x2508 */
} ItemList;

/* C++ views of the classes, used only for virtual calls and `delete`
 * (MWCC loads the target into $t9 for a virtual call, never for a call
 * through a C function pointer). Nothing here is defined, so no vtable is
 * emitted; the real vtables are the D_0000A7xx externs, stored into the
 * explicit `vptr` members by the constructors/destructors. */
class SceneV {
    char data[0x220];
public:
    virtual ~SceneV();
};

class ItemV {
    char data[0x38];
public:
    virtual ~ItemV();
    virtual int update();
    virtual int draw(int ctx);
    virtual int unk5(int x);
};

/* The gallery scene object (0x224 bytes, vtable D_0000A7F8). Created in
 * func_00000230 with new(D_0030A850 heap) and kept in D_0030A854; the
 * constructor also publishes it in D_0030A858. */
typedef struct Scene {
    Player *player;         /* +0x000 */
    int unk04;              /* +0x004 */
    ItemList *list;         /* +0x008 */
    int state;              /* +0x00C */
    unsigned char unk10;    /* +0x010 draw the black screen */
    char unk11;             /* +0x011 */
    char is_japanese;       /* +0x012 ehsys_get_language() == 0 */
    char pad13;
    int page;               /* +0x014 */
    short unk18;            /* +0x018 */
    short unk1A;            /* +0x01A */
    signed char unk1C;      /* +0x01C */
    signed char unk1D;      /* +0x01D */
    signed char unk1E;      /* +0x01E */
    char pad1F;
    float unk20;            /* +0x020 */
    float unk24;            /* +0x024 */
    float unk28;            /* +0x028 */
    float unk2C;            /* +0x02C */
    float unk30;            /* +0x030 */
    short unk34;            /* +0x034 */
    short pad36;
    float unk38;            /* +0x038 */
    float unk3C;            /* +0x03C */
    float unk40;            /* +0x040 */
    float unk44;            /* +0x044 */
    int unk48;              /* +0x048 */
    int unk4C;              /* +0x04C */
    int unk50;              /* +0x050 */
    int unk54;              /* +0x054 */
    int dir;                /* +0x058 */
    char *unk5C;            /* +0x05C */
    int unk60;              /* +0x060 */
    int unk64;              /* +0x064 */
    int unk68;              /* +0x068 */
    int unk6C;              /* +0x06C */
    short unk70;            /* +0x070 */
    short unk72;            /* +0x072 */
    short unk74;            /* +0x074 */
    short unk76;            /* +0x076 */
    void *unk78;            /* +0x078 page title string */
    char unk7C[0x100];      /* +0x07C */
    short fade[2];          /* +0x17C text fade-in: 0 title, 1 description */
    signed char unlocked[0x9D]; /* +0x180 one flag per movie, both pages */
    char pad21D[3];
    Vtbl *vptr;             /* +0x220 */
} Scene;

struct Task;
typedef struct TaskLink {
    void *unk0;
    struct Task *prev;
    struct Task *next;
} TaskLink;

/* A task node (0x20 bytes, vtable D_0000A810: dtor func_00004504,
 * func_00004394, update func_000042B8, draw func_00004314, func_000044A4),
 * in an update list and a draw list hanging off the root D_00755884. */
typedef struct Task {
    TaskLink upd;           /* +0x00 */
    TaskLink drw;           /* +0x0C */
    unsigned char upd_on;   /* +0x18 */
    unsigned char drw_on;   /* +0x19 */
    char pad1A[2];
    Vtbl *vptr;             /* +0x1C */
} Task;

class TaskV {
    char data[0x1C];
public:
    virtual ~TaskV();
    virtual void unk3();
    virtual void update();
    virtual void draw();
    virtual void unk6();
};

class PlayerV {
    char data[0x184];
public:
    virtual ~PlayerV();
};

class ItemListV {
    char data[0x2508];
public:
    virtual ~ItemListV();
    virtual void load(Player *arc, signed char *unlocked);
    virtual void update();
    virtual void draw();
};

extern Vtbl D_0000A7C8;   /* ItemList vtable */
extern Vtbl D_0000A7E0;   /* Item vtable */
extern Vtbl D_0000A7F8;   /* Scene vtable */
extern Vtbl D_0000A804;   /* Player vtable */
extern Vtbl D_0000A810;   /* Task vtable */
extern Task *D_00755884;  /* root task (baked absolute address) */
typedef struct {
    signed char v[0x8C];
} UnlockIds;
extern signed char D_0000A230[]; /* UnlockIds: save ids of the first 21 movies */
extern unsigned char D_00009858[0x40]; /* page-2 index -> save byte */

extern char *D_00009D6C[8]; /* thum01j.gim thum02j.gim thum01e.gim thum02e.gim
                              bg.ehf bg.tmf gallpda.ehf gallpda.tmf */
extern char D_00009D8C[];    /* "gallery.gim" */
extern signed char D_00009854[2]; /* tiles per page: 93, 64 */
extern char *D_00009D78;     /* = D_00009D6C[3], "thum02e.gim" */

extern int D_0000A840;    /* module registration block (.bss) */
extern int D_0000A84C;    /* "scene running" flag */
extern char D_0000A850[0x300000]; /* heap arena */
extern int D_0030A850;    /* heap handle (baked absolute address) */
extern Scene *D_0030A854; /* the scene (baked absolute address) */
extern Scene *D_0030A858; /* Scene singleton, set by its ctor/dtor */
extern char D_0070A880[]; /* image work buffer (baked absolute address) */
extern char D_0030A880[0x400000]; /* movie work buffer, 4 MiB */
typedef struct MovieName {
    char *jp;   /* also the default */
    char *us;   /* USA variant, or 0 */
} MovieName;
extern MovieName D_0000A030[]; /* TF5_opening, TF5_staffroll, SSD_s, SSD_a, ... */
extern char D_0000A308[];      /* "movie/%s.pmf" */
extern char *D_0000A2F4;       /* -> "thum01e.gim" */
extern char *D_0000A2F8;       /* -> "thum02e.gim" */
extern char D_0000A2FC[];      /* "gallery.gim" */
extern char D_0000A318[];      /* "strTbl.bin" */
extern signed char D_0000989C[2]; /* first movie index per page: 0, 93 */
extern short D_000098A0[];     /* movie -> string id */
extern int D_000099E8[2];      /* page title string ids */
extern signed char D_00009A00[2]; /* movies per page (scroll bar) */
extern const float D_00009A04; /* 6.2831855f (2 pi) */
extern char D_0000A778[];      /* "EHP" */
extern char D_0000A77C[];      /* "database/gallery/%s" */
extern char D_0000A790[];      /* "%s" */
extern char D_0000A794[];      /* "%s%s.gim" */
extern char D_0000A7A0[];      /* "body.ehf" */
extern char D_0000A7AC[];      /* "mouth.ehf" */
extern char D_0000A7B8[];      /* "field/bustup/%s" */
extern char *D_0000A39C[];     /* basic_lang_<jpn|eng|ger|fra|ita|spa>.ehp */
extern char *D_0000A414[];     /* field/event/<j|e|g|f|i|s>/ */
typedef struct FileEntry {
    int dev;
    char *name;
} FileEntry;
extern FileEntry D_00009A14[]; /* basic.ehp, ev5_03, ev5_04, ... */
typedef struct BustUp {
    int motion;
    short x;           /* x offset of the model in the zoomed picture */
    short pad;
} BustUp;
extern BustUp D_0000A538[];    /* per character, ids 0x18.. (minus 7 from 0x5A) */
extern int D_00755880;         /* scroll-bar blink phase (baked absolute) */
extern signed char D_00009898[2]; /* movies per page: 93, 64 */
extern const float D_00009A08;       /* 0.64f: snap distance^2 */
extern const float D_00009A0C;       /* 0.88f: cursor easing */
extern const float D_00009A10;       /* 0.15f: exit easing */

int func_00000230(void);
int func_000002D4(void);
void func_00000314(void);
void func_00000344(void);
void func_00000228(void);
void func_00000188(void);
Scene *func_000002B8(Scene *self);
void func_000013D0(Scene *self);
int func_00002124(Scene *self);
void func_00003BCC(Scene *self);
void func_0000175C(Scene *self);
Item *func_000007B4(Item *self, short flag);
Item *func_000007FC(Item *self);
int func_00003D84(Player *self, int idx);
void func_000015D0(Scene *self);
Player *func_000015B8(Player *self);
void func_00004220(void);
void func_00003D24(Player *self);
void func_00003D94(Player *self, int id);
void func_0000431C(Scene *self);
void func_00003F94(Player *self);
int func_00001930(Player *self);
int func_00001D00(Player *self);
void func_00002074(Scene *self, int a, int b, int c, int d);
void func_00002678(Scene *self);
int func_000034E0(Scene *self);
void func_00003F9C(Player *self, int id);
void func_000025EC(Scene *self);
int func_00004028(Player *self, int id, int arg);
void func_00003FA4(Player *self);
void func_000024E8(Scene *self, int idx);
int func_000024D8(Player *self);
void func_000024E0(Scene *self, unsigned char v);
void func_00001818(Scene *self);
void func_0000356C(Scene *self, int page);
void func_00004264(void);
void func_00002650(Scene *self);
void func_000025D8(Scene *self);
int func_000041E8(Player *self, int id);
float func_00001328(ItemList *self, int page);
void func_000032F0(int x, int y, void *str);
void func_000037C0(int x, int y, int w, char *str);
int func_00001358(ItemList *self, int page);
void func_000042C0(Scene *self);
void func_000037D4(Scene *self);
void func_00003094(Scene *self);
void func_00003204(Scene *self);
void func_0000320C(Scene *self);
void func_00002F94(Scene *self);
void func_00003640(Scene *self);
void func_00003310(Scene *self);
void func_00002BA8(Scene *self);
void func_00002C80(Scene *self);
void func_00002AE8(Scene *self);
void func_00003F20(LoadReq *self);
void func_00003D6C(Player *self);
void func_00003D7C(Player *self, void **slot, void *data);
void func_00003F30(Player *self);
int func_00003CA8(void *arg, int result);
int func_000041DC(int *flag, int result);
Task *func_000044AC(Task *self);
void func_000044F4(TaskLink *self);
void func_0000439C(Task *t);
void func_000043F4(Task *t);
void func_0000444C(Task *t, unsigned char flag);

/* func_00000150 — module_start: register the module's main/exit pair.
 * MATCH 100% */
int func_00000150(int arg) {
    if (arg == 0) {
        ehsys_B4471B5E(func_00000188, func_00000228, &D_0000A840);
    }
    return 0;
}

/* func_00000188 — module main: init, install the update/draw callbacks and
 * run frames until func_000002D4 reports the scene is over, then fade out
 * and request mode 0xF.
 * MATCH 100% */
void func_00000188(void) {
    int done = 0;

    if (func_00000230()) {
        ehsys_C2E59A21(2, func_00000314, func_00000344);
        while ((done != 0) ^ 1) {
            done |= func_000002D4();
            ehsys_frame_sync();
            if (done) {
                ehsys_8060707A();
                ehsys_13A4081A(1);
                ehsys_frame_sync();
                ehsys_08813E19(0xF, 0);
                break;
            }
        }
    }
}

/* func_00000228 — module exit (empty).
 * MATCH 100% */
void func_00000228(void) {
}

/* func_00000230 — init: 3 MiB heap, new Scene, scene setup.
 * MATCH 100% */
int func_00000230(void) {
    int *running = &D_0000A84C;
    Scene *scene;

    ehsys_sceKernelChangeCurrentThreadAttr(0, 0x4000);
    D_0030A850 = ehsys_1856E536(D_0000A850, 0x300000);
    ehsys_6368BABC(D_0030A850);
    scene = (Scene *) ehsys_57345021(sizeof(Scene), D_0030A850);
    if (scene) {
        scene = func_000002B8(scene);
    }
    D_0030A854 = scene;
    func_000013D0(scene);
    *running = 1;
    return 1;
}

/* func_000002B8 — Scene::Scene().
 * MATCH 100% */
Scene *func_000002B8(Scene *self) {
    self->vptr = &D_0000A7F8;
    D_0030A858 = self;
    return self;
}

/* func_000002D4 — per-frame update; 1 once the scene has finished.
 * MATCH 100% */
int func_000002D4(void) {
    int *running = &D_0000A84C;

    if (func_00002124(D_0030A854)) {
        *running = 0;
        return 1;
    }
    return 0;
}

/* func_00000314 — draw callback.
 * MATCH 100% */
void func_00000314(void) {
    if (D_0000A84C) {
        func_00003BCC(D_0030A854);
    }
}

/* func_00000344 — exit callback: tear the scene down, delete it, free the
 * heap.
 * MATCH 100% */
void func_00000344(void) {
    int a;
    int b;

    func_0000175C(D_0030A854);
    delete (SceneV *) D_0030A854;
    D_0030A854 = 0;
    ehsys_B46025DF(D_0030A850, &a, &b);
    ehsys_2F21139D(D_0030A850);
    ehsys_1078C73B(D_0030A850);
}

/* func_000003B8 — Scene::~Scene() (deleting).
 * MATCH 100% */
Scene *func_000003B8(Scene *self, short flag) {
    if (self) {
        self->vptr = &D_0000A7F8;
        D_0030A858 = 0;
        if (flag > 0) {
            ehsys_9455886E(self);
        }
    }
    return self;
}

/* func_00000408 — Item::update() (vtable slot 3). Counts down `delay`,
 * then runs the current move: 0 = scroll-in (unk34 eases 0 -> 1 in steps
 * of 0.25), 1 = slide from an offset towards (dst_x, dst_y) over `frames`
 * frames, the offset depending on `dir` (0/1: +-36 in y, 2/3: +-58 in x)
 * and fading alpha in or out.
 * MATCH 100% */
int func_00000408(Item *self) {
    float sx;
    float sy;

    if (self->delay != 0) {
        self->delay--;
        return 0;
    }
    switch (self->move) {
    case 0:
        self->unk34 += 0.25f;
        if (self->unk34 > 1.0f) {
            self->unk34 = 1.0f;
        }
        self->y = self->dst_y - (float) (self->unk30 * 36);
        self->y = self->y + self->unk34 * (float) ((self->unk30 - self->unk2C) * 36);
        break;
    case 1:
        if (self->frames != 0) {
            self->frame++;
            if (self->frame > self->frames) {
                self->frame = self->frames;
            }
            self->t = (float) self->frame / (float) self->frames;
        }
        switch (self->dir) {
        case 0:
            sy = 36.0f + self->dst_y;
            sx = self->dst_x;
            self->alpha = self->t;
            break;
        case 1:
            sy = self->dst_y - 36.0f;
            sx = self->dst_x;
            self->alpha = 1.0f - self->t;
            break;
        case 2:
            sx = 58.0f + self->dst_x;
            sy = self->dst_y;
            self->alpha = 1.0f - self->t;
            break;
        case 3:
            sx = self->dst_x - 58.0f;
            sy = self->dst_y;
            self->alpha = self->t;
            break;
        default:
            sx = self->dst_x;
            sy = self->dst_y;
            self->alpha = self->t;
            break;
        }
        self->x = sx + self->t * (self->dst_x - sx);
        self->y = sy + self->t * (self->dst_y - sy);
        if (self->t >= 1.0f) {
            self->move = 0;
        }
        break;
    default:
        break;
    }
    return 0;
}

/* func_00000638 — Item::draw(ctx) (vtable slot 4): one 0x30 x 0x28 sprite
 * from the 0x190 x 0xAE thumbnail sheet at (x, y), alpha from `alpha`.
 * MATCH 100% */
int func_00000638(Item *self, int ctx) {
    int spr;

    ctx = ehsys_20560342(ctx, 0x30, 0x28, 0x190, 0xAE);
    spr = ehsys_F462EE6E(ctx, 2);
    spr = ehsys_CABEA834(spr, ((unsigned char) (255.0f * self->alpha) << 24) | 0xFFFFFF);
    spr = ehsys_FDD168CA(spr, (int) self->x, (int) self->y, 0, self->u, self->v, 0, 0);
    return ehsys_4175E3DB(spr, 2);
}

/* func_00000720 — Item vtable slot 5: returns its argument.
 * MATCH 100% */
int func_00000720(Item *self, int x) {
    return x;
}

/* func_00000728 — ItemList::ItemList().
 * MATCH 100% */
ItemList *func_00000728(ItemList *self) {
    int i;

    self->vptr = &D_0000A7C8;
    func_0000006C(self->items, (CtorDtor) func_000007FC, (CtorDtor) func_000007B4, sizeof(Item), 0x9D);
    for (i = 0; i < 0x9D; i++) {
        self->items[i].active = 0;
        self->items[i].group = 0;
        self->items[i].y = 0.0f;
        self->items[i].x = 0.0f;
    }
    self->unk24DC = 0;
    self->tex24E0 = 0;
    self->tex24E4 = 0;
    return self;
}

/* func_000007B4 — Item::~Item() (deleting).
 * MATCH 100% */
Item *func_000007B4(Item *self, short flag) {
    if (self) {
        self->vptr = &D_0000A7E0;
        if (flag > 0) {
            ehsys_9455886E(self);
        }
    }
    return self;
}

/* func_000007FC — Item::Item().
 * MATCH 100% */
Item *func_000007FC(Item *self) {
    self->vptr = &D_0000A7E0;
    self->delay = 0;
    return self;
}

/* func_00000814 — ItemList::load(archive, unlocked) (vtable slot 3): load
 * the thumbnail sheets and the background model/motion, then lay out the
 * two pages of tiles (9 per row, 52 x 28 px cells); locked entries show
 * the "?" cell at (0x40, 0x24).
 * DRAFT, 1 instruction scheduled differently (155/155 words, logic and
 * registers identical): the target hoists `lui %hi(D_00009D78)` above the
 * prologue's `sw $s0`; we emit it right after. Tried: the extern as a
 * table element or a separate symbol, `const` table / pointer, the local
 * initialised at its declaration or by statement, every declaration order,
 * block-scoped parameter copies, and two decomp-permuter runs (~9,000
 * iterations, no improvement). Breaks `make SRC=1` at file byte 0x87C. */
void func_00000814(ItemList *self, Player *arc, signed char *unlocked) {
    char *name = D_00009D78;
    int j;
    int g;
    int k;
    int n;
    int cnt;
    Item *it;

    self->tex24E0 = ehsys_C6C15111(func_00003D84(arc, 0), D_00009D6C[2], 0);
    self->tex24E4 = ehsys_C6C15111(func_00003D84(arc, 0), name, 0);
    self->tex24E8 = ehsys_C6C15111(func_00003D84(arc, 0), D_00009D8C, 0);
    self->tex24EC = ehsys_C6C15111(func_00003D84(arc, 0), D_00009D6C[4], 0);
    self->tex24F0 = ehsys_C6C15111(func_00003D84(arc, 0), D_00009D6C[5], 0);
    ehsys_FDAFCF3A(self->tex24EC, self->tex24F0);
    self->tex24F4 = ehsys_C6C15111(func_00003D84(arc, 0), D_00009D6C[6], 0);
    self->tex24F8 = ehsys_C6C15111(func_00003D84(arc, 0), D_00009D6C[7], 0);
    ehsys_FDAFCF3A(self->tex24F4, self->tex24F8);
    self->unk24FC = 0;
    self->unk24FE = 6;
    k = 0;
    for (g = 0; g < 2; g++) {
        self->groups[g].first = &self->items[k];
        n = D_00009854[g];
        cnt = 0;
        for (j = 0; j < n; j++) {
            it = &self->items[k];
            it->group = g;
            it->move = 0;
            it->active = 1;
            it->unk1D = g;
            it->y = 800.0f;
            it->x = 800.0f;
            cnt++;
            if (unlocked[k]) {
                it->u = (j % 9) * 52;
                it->v = (j / 9) * 28;
                it->frame = 0;
                it->frames = 6;
                it->unk1A = 1;
            } else {
                it->unk1D = -1;
                it->u = 0x40;
                it->v = 0x24;
                it->unk1A = 0;
            }
            k++;
        }
        self->groups[g].count = cnt;
    }
}

/* func_00000A80 — ItemList::update() (vtable slot 4).
 * MATCH 100% */
void func_00000A80(ItemList *self) {
    int i;

    for (i = 0; i < 0x9D; i++) {
        if (self->items[i].active == 1) {
            ((ItemV *) &self->items[i])->update();
        }
    }
}

/* func_00000AEC — ItemList::draw() (vtable slot 5): the background model
 * with its looping motion (frames 8..0x8B after the intro, 0..6 first),
 * then every active tile.
 * MATCH 100% */
void func_00000AEC(ItemList *self) {
    int bg[24];
    int pda[24];
    int ctx;
    int frame;
    int i;
    Item *it;

    if (self->tex24E0 == 0 || self->tex24E4 == 0) {
        return;
    }
    ctx = ehsys_DFCA450B(4, 0);
    ehsys_memset(bg, 0, 0x60);
    bg[0] = *ehsys_B2B9FEFA();
    ctx = ehsys_85F89FE8(ctx);
    ctx = ehsys_7187E0F0(ctx, self->tex24EC, bg);
    ctx = ehsys_89812D8A(ctx);
    if (self->unk24FC != 0 || ehsys_450CD6C5()) {
        frame = ehsys_558B2DCB(self->tex24F4);
        if (frame < self->unk24FC || frame >= self->unk24FE) {
            if (self->unk24FC == 0) {
                self->unk24FC = 8;
                self->unk24FE = 0x8B;
            } else if (self->unk24FE == 0x92) {
                if (frame >= 0x92) {
                    self->unk24FE = 0;
                }
            } else {
                frame = self->unk24FC;
            }
        }
        if (self->unk24FE != 0) {
            ehsys_memset(pda, 0, 0x60);
            ehsys_CE8B1945(self->tex24F4, frame);
            pda[0] = *ehsys_B2B9FEFA();
            ctx = ehsys_85F89FE8(ctx);
            ctx = ehsys_7187E0F0(ctx, self->tex24F4, pda);
            ctx = ehsys_89812D8A(ctx);
        }
    }
    ctx = ehsys_E6E4401F(ctx, 0, 0);
    ctx = ehsys_5CBB9E43(ctx, 0, 2, 3, 0, 0);
    ctx = ehsys_CABEA834(ctx, 0xFFFFFFFF);
    ctx = ehsys_8EA6B1E7(ctx, 0x34, 0x1C, 0x34, 0x1C);
    for (i = 0; i < 0x9D; i++) {
        it = &self->items[i];
        if (it->active == 1) {
            if (it->unk1D < 0) {
                ctx = ehsys_9EA6989A(ctx, self->tex24E8);
            } else {
                ctx = ehsys_9EA6989A(ctx, it->unk1D == 0 ? self->tex24E0 : self->tex24E4);
            }
            ctx = ((ItemV *) it)->draw(ctx);
        }
    }
    ehsys_1EC5342B(ctx);
}

/* func_00000D54 — page g slides in from below (dir 0): tiles laid out 7
 * per row in 58 x 36 cells from (0x30, 0x28); only the first 93 animate.
 * MATCH 100% */
void func_00000D54(ItemList *self, int g) {
    ItemGroup *grp = &self->groups[g];
    Item *it;
    int i;

    for (i = 0; i < grp->count; i++) {
        it = &grp->first[i];
        it->active = 1;
        if (i < 0x5D) {
            it->dst_x = (float) ((i % 7) * 58 + 0x30);
            it->dst_y = (float) ((i / 7) * 36 + 0x28);
            it->x = it->dst_x;
            it->y = 36.0f + it->dst_y;
            it->t = 0.0f;
            it->dir = 0;
            it->frame = 0;
            it->frames = 6;
            it->move = 1;
            it->delay = 0;
        } else {
            it->t = 1.0f;
            it->delay = 0;
        }
    }
}

/* func_00000E40 — page g slides out downwards (dir 1).
 * MATCH 100% */
void func_00000E40(ItemList *self, int g) {
    ItemGroup *grp = &self->groups[g];
    Item *it;
    int i;

    for (i = 0; i < grp->count; i++) {
        it = &grp->first[i];
        it->active = 1;
        it->x = (float) ((i % 7) * 58 + 0x30);
        it->y = (float) ((i / 7) * 36 + 0x28);
        it->dst_x = it->dst_x;
        it->dst_y = it->dst_y + 36.0f;
        it->t = 0.0f;
        it->dir = 1;
        it->frame = 0;
        it->frames = 6;
        it->unk2C = 0;
        it->unk30 = 0;
        it->unk34 = 0.0f;
        it->move = 1;
    }
}

/* func_00000F1C — switch pages: dir 0 = show `page`, dir 0xFFFF = hide it,
 * otherwise `page` slides out and the other page slides in from the right
 * (dir 3), and the scroll positions are reset.
 * MATCH 100% */
void func_00000F1C(ItemList *self, int page, int dir) {
    int other = (page + 2 + dir) % 2;
    ItemGroup *grp;
    Item *it;
    int i;

    if (dir == 0) {
        func_00000D54(self, page);
        return;
    }
    if (dir == 0xFFFF) {
        func_00000E40(self, page);
        return;
    }
    grp = &self->groups[page];
    for (i = 0; i < grp->count; i++) {
        it = &grp->first[i];
        it->active = 1;
        it->x = (float) ((i % 7) * 58 + 0x30);
        it->y = (float) ((i / 7) * 36 + 0x28);
        it->dst_x = it->dst_x;
        it->dst_y = it->dst_y + 36.0f;
        it->t = 0.0f;
        it->dir = 1;
        it->frame = 0;
        it->frames = 6;
        it->unk2C = 0;
        it->unk30 = 0;
        it->unk34 = 0.0f;
        it->move = 1;
    }
    grp = &self->groups[other];
    for (i = 0; i < grp->count; i++) {
        it = &grp->first[i];
        it->dst_x = (float) ((i % 7) * 58 + 0x30);
        it->dst_y = (float) ((i / 7) * 36 + 0x28);
        it->x = 58.0f + it->dst_x;
        it->y = it->dst_y;
        it->t = 0.0f;
        it->dir = 3;
        it->frame = 0;
        it->frames = 6;
        it->unk2C = 0;
        it->unk30 = 0;
        it->unk34 = 0.0f;
        it->move = 1;
    }
    for (i = 0; i < 2; i++) {
        self->scroll[i] = 0;
        self->prev_scroll[i] = 0;
    }
}

/* func_00001144 — Item: still animating (frame != frames)?
 * MATCH 100% */
int func_00001144(Item *self) {
    return self->frame != self->frames;
}

/* func_00001158 — ItemList: background motion in its loop phase?
 * MATCH 100% */
int func_00001158(ItemList *self) {
    return self->unk24FC == 8;
}

/* ItemGroup::setScroll (inline only): give every tile of the group the
 * new/old scroll for its scroll animation. Passing the values through its
 * `signed char` parameters is what re-sign-extends them (`seb`) at the
 * call site in func_00001168. */
static inline void func_SetScroll(ItemList *self, int page, signed char next, signed char prev) {
    ItemGroup *grp = &self->groups[page];
    Item *it;
    int i;

    for (i = 0; i < grp->count; i++) {
        it = &grp->first[i];
        it->unk2C = next;
        it->unk30 = prev;
        it->unk34 = 0.0f;
    }
}

/* func_00001168 — ItemList: keep row `to` visible after the cursor moved
 * from row `from` on `page`; 0 if no scroll was needed, else 1 and every
 * tile of the page gets the new/old scroll for its scroll animation.
 * MATCH 100% */
int func_00001168(ItemList *self, signed char from, signed char to, int page) {
    signed char diff;
    signed char mode;
    signed char prev;
    signed char next;

    if (to < self->scroll[page] + 5 && to >= self->scroll[page]) {
        return 0;
    }
    diff = from - to;
    if (ehsys_abs(diff) == 1) {
        if (from < to) {
            mode = 0;
        } else {
            mode = 1;
        }
    } else if (diff > 0) {
        mode = 2;
    } else {
        mode = 3;
    }
    if (mode == 0) {
        self->scroll[page] = to - 4;
        self->prev_scroll[page] = from - 4;
        next = to - 4;
        prev = from - 4;
    } else if (mode == 1) {
        self->prev_scroll[page] = self->scroll[page];
        prev = self->scroll[page];
        self->scroll[page]--;
        next = self->scroll[page];
    } else if (mode == 2) {
        self->scroll[page] = to;
        self->prev_scroll[page] = from - 4;
        next = to;
        prev = from - 4;
    } else {
        self->scroll[page] = to - 4;
        self->prev_scroll[page] = from;
        next = to - 4;
        prev = from;
    }
    func_SetScroll(self, page, next, prev);
    return 1;
}

/* func_00001328 — ItemList: top of `page` (y of its first tile) - 40.
 * MATCH 100% */
float func_00001328(ItemList *self, int page) {
    if (page != 0) {
        return self->items[0x5D].y - 40.0f;
    } else {
        return self->items[0].y - 40.0f;
    }
}

/* func_00001358 — ItemList: scroll row of `page`.
 * MATCH 100% */
int func_00001358(ItemList *self, int page) {
    return self->scroll[page];
}

/* func_00001368 — ItemList::~ItemList() (deleting).
 * MATCH 100% */
ItemList *func_00001368(ItemList *self, short flag) {
    if (self) {
        self->vptr = &D_0000A7C8;
        func_00000000(self->items, (CtorDtor) func_000007B4, sizeof(Item), 0x9D);
        if (flag > 0) {
            ehsys_9455886E(self);
        }
    }
    return self;
}

/* func_000013D0 — Scene::init(): defaults, unlock flags, the player and
 * the tile list, then wait for BGM 0x15 and fade in.
 * MATCH 100% */
void func_000013D0(Scene *self) {
    Player *player;
    ItemList *list;

    self->player = 0;
    self->state = 0;
    self->unk10 = 0;
    self->unk04 = 0;
    self->unk5C = 0;
    self->unk60 = 0;
    self->page = 0;
    self->unk1A = 0xFF;
    self->unk1C = -10;
    self->unk70 = 0xFF;
    self->unk72 = -10;
    self->unk50 = 0;
    self->unk54 = 0;
    self->unk64 = 0;
    self->unk68 = 0;
    self->unk6C = 0;
    self->unk1D = 0;
    self->unk1E = 0;
    self->unk20 = 224.0f;
    self->unk24 = 320.0f;
    self->unk28 = 44.0f;
    self->unk2C = 36.0f;
    self->unk18 = 0;
    self->unk30 = 0.25f;
    self->unk11 = 0;
    self->unk78 = 0;
    ehsys_memset(self->unk7C, 0, 0x100);
    self->fade[0] = 0;
    self->fade[1] = 0;
    ehsys_memset(self->unlocked, 0, 0x9D);
    func_000015D0(self);
    func_00004220();
    player = (Player *) ehsys_57345021(sizeof(Player), D_0030A850);
    if (player) {
        player = func_000015B8(player);
    }
    self->player = player;
    func_00003D24(player);
    list = (ItemList *) ehsys_57345021(sizeof(ItemList), D_0030A850);
    if (list) {
        list = func_00000728(list);
    }
    self->list = list;
    self->is_japanese = ehsys_get_language() == 0;
    ehsys_1EC5342B(ehsys_459B889D(ehsys_DFCA450B(0, 0)));
    func_00003D94(self->player, 0x5E);
    func_00003D94(self->player, 0);
    if (!(ehsys_ED17CBAC(0x15) & 1)) {
        ehsys_6E42DDCD();
        ehsys_C859D5FE(0x15);
        while ((ehsys_AA9913C7(0x15) != 0) ^ 1) {
            ehsys_frame_sync();
        }
        ehsys_BE3756D0(0x15);
    }
    ehsys_13A4081A(0);
    ehsys_5998BFD7(1, 0, 1);
}

/* func_000015B8 — Player::Player().
 * MATCH 100% */
Player *func_000015B8(Player *self) {
    self->vptr = &D_0000A804;
    self->unk180 = 0;
    return self;
}

/* func_000015D0 — Scene: read which movies are unlocked from the save.
 * Page 1: the first 21 by their own ids (D_0000A230 table, through
 * ehsys_635BB4EE), the rest as ehsys_8477BA04(id, 0) with ids from 3,
 * skipping 0x5A..0x60. unlocked[0x5D] is always set; unlocked[0x5E] once
 * any (g, j) pair of groups 2..0x90 is both present and seen. The rest of
 * page 2 comes straight from save bytes at 0x2A7AC + D_00009858[k].
 * MATCH 100% */
void func_000015D0(Scene *self) {
    UnlockIds ids = *(UnlockIds *) D_0000A230;
    int i;
    int j;
    int n;
    int g;
    unsigned char *save;
    int id;

    ehsys_memset(self->unlocked, 0, 0x9D);
    for (i = 0; i < 0x5D; i++) {
        if (i < 0x15) {
            self->unlocked[i] = ehsys_635BB4EE((unsigned char) ids.v[i]);
        } else {
            id = i + 3;
            if (id > 0x59) {
                id += 7;
            }
            self->unlocked[i] = ehsys_8477BA04(id, 0);
        }
    }
    self->unlocked[0x5D] = 1;
    for (g = 2; g < 0x91; g++) {
        n = ehsys_34A7E8C2(g);
        for (j = 0; j < n; j++) {
            if (ehsys_99FF9D51(g, j)) {
                if (ehsys_8477BA04(g, j)) {
                    self->unlocked[0x5E] = 1;
                    g = 0x91;
                    break;
                }
            }
        }
    }
    save = ehsys_D8A0DB10();
    for (i = 2; i < 0x40; i++) {
        self->unlocked[0x5D + i] = save[D_00009858[i] + 0x2A7AC];
    }
}

/* func_0000175C — Scene::term(): delete the player and the tile list.
 * MATCH 100% */
void func_0000175C(Scene *self) {
    func_0000431C(self);
    ehsys_03E45FFF();
    func_00003F94(self->player);
    delete (PlayerV *) self->player;
    self->player = 0;
    delete (ItemListV *) self->list;
    self->list = 0;
}

/* func_000017D0 — Player::~Player() (deleting).
 * MATCH 100% */
Player *func_000017D0(Player *self, short flag) {
    if (self) {
        self->vptr = &D_0000A804;
        if (flag > 0) {
            ehsys_9455886E(self);
        }
    }
    return self;
}

/* func_00001818 — empty.
 * MATCH 100% */
void func_00001818(Scene *self) {
}

/* func_00001820 — Scene: copy the current file (a thumbnail image) into
 * the work buffer, patch its header bytes and run func_00002074 over a
 * 32 + 28 column grid for both halves.
 * MATCH 100% */
char *func_00001820(Scene *self, void *src) {
    int i;
    int j;

    ehsys_memcpy(D_0070A880, src, func_00001930(self->player));
    D_0070A880[0x14] = 8;
    D_0070A880[0x16] = 4;
    D_0070A880[0x26] = 4;
    D_0070A880[0x486] = 4;
    D_0070A880[0x48A] = 4;
    D_0070A880[0x49B] = 2;
    D_0070A880[0x4B2] = 4;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 0x20; j++) {
            func_00002074(self, j, i + 0x20, i + 0x3C, 0x1F - j);
        }
        for (j = 0; j < 0x1C; j++) {
            func_00002074(self, j + 0x20, i + 0x20, i + 0x3E, 0x1F - j);
        }
    }
    return D_0070A880;
}

/* func_00001930 — Player: size of the current file.
 * MATCH 100% */
int func_00001930(Player *self) {
    return self->unk180;
}

/* func_00001938 — Scene: grid input. 3 = back (cancel button), 1 = play
 * the selected movie, 2 = page change (R/0x8000 next, L previous), 0 = no
 * action; locked entries buzz. Also pulses the cursor alpha 0x60..0xFF.
 * MATCH 100% */
int func_00001938(Scene *self) {
    Pad *pad;
    int trig;

    pad = ehsys_F5E3080C();
    func_00002678(self);
    self->unk70 += self->unk72;
    if (self->unk70 < 0x60) {
        self->unk70 = 0x60;
        self->unk72 = -self->unk72;
    } else if (self->unk70 > 0xFF) {
        self->unk70 = 0xFF;
        self->unk72 = -self->unk72;
    }
    trig = pad->trigger;
    if (trig & (unsigned short) ehsys_get_button_code(1)) {
        ehsys_2AE3C0AD(0x94C5);
        return 3;
    }
    if (trig & (unsigned short) ehsys_get_button_code(0)) {
        if (self->unk18 == 2) {
            ehsys_2AE3C0AD(0x94C6);
            return 0;
        }
        if (self->unlocked[func_000034E0(self)] == 0) {
            ehsys_2AE3C0AD(0x94C6);
            return 0;
        }
        self->unk76 = func_000034E0(self);
        if (self->unk76 < 0x5D) {
            func_00003F9C(self->player, self->unk76 + 1);
        } else {
            self->unk74 = self->unk76 - 0x5D;
        }
        ehsys_2AE3C0AD(0x94C4);
        return 1;
    }
    if ((trig & 0x200) || (trig & 0x8000)) {
        self->dir = 1;
        func_000025EC(self);
        ehsys_2AE3C0AD(0x94CC);
        return 2;
    }
    if (trig & 0x100) {
        self->dir = -1;
        func_000025EC(self);
        ehsys_2AE3C0AD(0x94CC);
        return 2;
    }
    return 0;
}

/* func_00001AF0 — Scene: zoom the selected thumbnail to full screen and
 * fade (unk50 alpha in, unk4C after frame 15); 1 when done (30 frames).
 * MATCH 100% */
int func_00001AF0(Scene *self) {
    int id;
    float t;
    float a;

    if (!func_00001D00(self->player)) {
        return 0;
    }
    if (self->unk76 >= 0x15 && self->unk76 < 0x5D) {
        id = self->unk76 + 3;
        if (id > 0x59) {
            id += 7;
        }
        if (!func_00004028(self->player, id, 0)) {
            return 0;
        }
    }
    switch (self->unk34) {
    case 0:
        self->unk5C = func_00001820(self, (void *) func_00001D00(self->player));
        self->unk38 = (float) (self->unk1D * 58 + 0x30);
        self->unk3C = (float) (self->unk1E * 36 + 0x28);
        self->unk40 = 52.0f + self->unk38;
        self->unk44 = 28.0f + self->unk3C;
        self->unk34 = 1;
        self->unk48 = 1;
        self->unk4C = 0;
    case 1:
        t = (float) self->unk54 / 15.0f;
        if (t > 1.0f) {
            t = 1.0f;
        }
        self->unk50 = (int) (255.0f * t);
        self->unk38 = 0.0f;
        self->unk3C = 0.0f;
        self->unk40 = 480.0f;
        self->unk44 = 272.0f;
        self->unk48 = 0;
        if (t < 1.0f) {
            a = 0.0f;
        } else {
            a = (float) (self->unk54 - 15) / 15.0f;
        }
        self->unk4C = (int) (255.0f * a);
        if (++self->unk54 >= 0x1E) {
            return 1;
        }
        break;
    }
    return 0;
}

/* func_00001D00 — Player: data of the current file.
 * MATCH 100% */
int func_00001D00(Player *self) {
    return func_00003D84(self, self->unk188);
}

/* func_00001D08 — Scene: fade back out of the zoomed thumbnail; 1 when
 * done, then the player state is reset.
 * MATCH 100% */
int func_00001D08(Scene *self) {
    if (self->unk54 >= 0xF) {
        self->unk4C = (int) (255.0f * ((float) (self->unk54 - 15) / 15.0f));
        self->unk50 = 0xFF;
    } else {
        self->unk4C = 0;
        self->unk50 = (int) (255.0f * ((float) self->unk54 / 15.0f));
    }
    if (--self->unk54 < 0) {
        self->unk54 = 0;
    }
    if (self->unk50 <= 0) {
        self->unk4C = 0;
        self->unk50 = 0;
        func_00003FA4(self->player);
        self->unk5C = 0;
        self->unk60 = 0;
        self->player->mdl[1] = 0;
        self->player->mdl[0] = 0;
        return 1;
    }
    return 0;
}

/* func_00001DEC — Scene: zoom the selected tile up to full screen
 * (0.25 easing per frame), then start the movie.
 * MATCH 100% */
int func_00001DEC(Scene *self) {
    switch (self->unk34) {
    case 0:
        self->unk5C = 0;
        self->unk60 = 0;
        self->player->mdl[1] = 0;
        self->player->mdl[0] = 0;
        self->unk34 = 1;
        self->unk50 = 0;
        self->unk38 = (float) (self->unk1D * 58 + 0x30);
        self->unk3C = (float) (self->unk1E * 36 + 0x28);
        self->unk40 = 52.0f + self->unk38;
        self->unk44 = 28.0f + self->unk3C;
    case 1:
        self->unk50 += 0x10;
        if (self->unk50 >= 0xFF) {
            self->unk50 = 0xFF;
        }
        self->unk38 = self->unk38 + 0.25f * -self->unk38;
        self->unk3C = self->unk3C + 0.25f * -self->unk3C;
        self->unk40 = self->unk40 + 0.25f * (480.0f - self->unk40);
        self->unk44 = self->unk44 + 0.25f * (272.0f - self->unk44);
        if (self->unk38 < 0.25f && 272.0f - self->unk44 < 0.25f) {
            func_000024E8(self, self->unk74);
            return 1;
        }
        break;
    default:
        break;
    }
    return 0;
}

/* func_00001F74 — Scene: fade the overlay out after a movie; 1 when done.
 * MATCH 100% */
int func_00001F74(Scene *self) {
    self->unk50 -= 10;
    if (self->unk50 <= 0) {
        self->unk50 = 0;
        self->unk5C = 0;
        self->unk60 = 0;
        self->player->mdl[1] = 0;
        self->player->mdl[0] = 0;
        return 1;
    }
    return 0;
}

/* func_00001FB4 — Scene: any of buttons 0x6000 pressed (close the zoomed
 * picture)?
 * MATCH 100% */
int func_00001FB4(Scene *self) {
    if (ehsys_F5E3080C()->trigger & 0x6000) {
        ehsys_2AE3C0AD(0x94C4);
        return 1;
    }
    return 0;
}

/* func_00001FEC — Scene: page change; waits for the tiles to settle.
 * MATCH 100% */
int func_00001FEC(Scene *self) {
    if (!func_00001144(self->list->items)) {
        func_00000F1C(self->list, self->page, self->dir);
        self->page = (self->page + 2 + self->dir) % 2;
        return 0;
    }
    func_0000356C(self, 0);
    func_0000356C(self, 1);
    return 1;
}

/* address of 8x8 tile (x, y) in the image work buffer */
static inline unsigned char *func_TileAddr(int x, int y) {
    return (unsigned char *) D_0070A880 + ((y << 12) + (((x - (x & 1)) << 6) + 0x4D0 + ((x & 1) << 3)));
}

/* func_00002074 — Scene: copy tile (sx, sy) of the work image to (dx, dy),
 * rotated (each source column becomes a destination row).
 * MATCH 100% */
void func_00002074(Scene *self, int dx, int dy, int sx, int sy) {
    int i;
    unsigned char *dst = func_TileAddr(dx, dy);
    unsigned char *src = func_TileAddr(sx, sy);

    for (i = 0; i < 8; i++) {
        dst[0] = src[0x70];
        dst[1] = src[0x60];
        dst[2] = src[0x50];
        dst[3] = src[0x40];
        dst[4] = src[0x30];
        dst[5] = src[0x20];
        dst[6] = src[0x10];
        dst[7] = src[0];
        dst += 0x10;
        src++;
    }
}

/* func_00002124 — Scene::update(): the gallery state machine.
 *   0 load, 1 wait for the player, 3 wait for the fade, 10 grid input,
 *   11/12/13 zoom a picture in, wait, zoom out, 14/15 zoom in and play a
 *   movie, fade back, 16 page change, 17 start the exit fade, 18 wait for
 *   it (returns 1: leave).
 * MATCH 100% */
int func_00002124(Scene *self) {
    char *name1;
    char *name2;

    ehsys_F5E3080C();
    func_00004264();
    ((ItemListV *) self->list)->update();
    switch (self->state) {
    case 0:
        func_000024E0(self, 1);
        self->state = 1;
        name1 = D_0000A2F4;
        name2 = D_0000A2F8;
        ((ItemListV *) self->list)->load(self->player, self->unlocked);
        self->unk64 = ehsys_C6C15111(func_00003D84(self->player, 0), name1, 0);
        self->unk68 = ehsys_C6C15111(func_00003D84(self->player, 0), name2, 0);
        self->unk6C = ehsys_C6C15111(func_00003D84(self->player, 0), D_0000A2FC, 0);
        break;
    case 1:
        if (!func_000024D8(self->player)) {
            func_00001818(self);
            func_000024E0(self, 0);
            ehsys_D470D0B2(1, 0xFF000000, 0x10);
            self->state = 3;
        }
        break;
    case 3:
        if (ehsys_450CD6C5()) {
            self->state = 10;
            func_0000356C(self, 0);
            func_0000356C(self, 1);
            func_00000F1C(self->list, self->page, 0);
            self->unk18 = 2;
        }
        break;
    case 10:
        switch (func_00001938(self)) {
        case 1:
            if (self->unk76 < 0x5D) {
                self->state = 11;
            } else {
                self->state = 14;
                ehsys_0D3E2FA6(0x15, 0x1F4);
            }
            self->unk34 = 0;
            break;
        case 2:
            self->state = 16;
            break;
        case 3:
            self->state = 17;
            break;
        }
        break;
    case 11:
        if (func_00001AF0(self) == 1) {
            self->state = 12;
        }
        break;
    case 12:
        if (func_00001FB4(self) == 1) {
            self->state = 13;
        }
        break;
    case 13:
        if (func_00001D08(self) == 1) {
            self->state = 10;
        }
        break;
    case 14:
        if (func_00001DEC(self) == 1) {
            self->state = 15;
            ehsys_BE3756D0(0x15);
        }
        break;
    case 15:
        if (func_00001F74(self) == 1) {
            self->state = 10;
        }
        break;
    case 16:
        if (func_00001FEC(self) == 1) {
            self->state = 10;
        }
        break;
    case 17:
        ehsys_5998BFD7(1, 0xFF000000, 0x1E);
        ehsys_6367AE39(600);
        self->state = 18;
        func_00000F1C(self->list, self->page, 0xFFFF);
        func_00002650(self);
        break;
    case 18:
        func_00002678(self);
        if (ehsys_450CD6C5()) {
            return 1;
        }
        break;
    default:
        break;
    }
    return 0;
}

/* func_000024D8 — Player: still loading?
 * MATCH 100% */
int func_000024D8(Player *self) {
    return self->pending;
}

/* func_000024E0 — Scene: set flag unk10.
 * MATCH 100% */
void func_000024E0(Scene *self, unsigned char v) {
    self->unk10 = v;
}

/* func_000024E8 — Scene: play movie `idx` ("movie/<name>.pmf", the USA
 * file when the language is not Japanese and one exists) with a 4 MiB
 * work buffer, blocking.
 * MATCH 100% */
void func_000024E8(Scene *self, int idx) {
    char path[0x100];
    char name[0x100];
    char *file;

    ehsys_70CB33BE(0, 1);
    while ((ehsys_DCA0DCE1() != 0) ^ 1) {
        ehsys_frame_sync();
    }
    if (self->is_japanese) {
        file = D_0000A030[idx].jp;
    } else {
        file = D_0000A030[idx].us;
        if (file == 0) {
            file = D_0000A030[idx].jp;
        }
    }
    ehsys_sprintf(name, D_0000A308, file);
    ehsys_68728B41(path, 3, name);
    ehsys_50EA4D71(D_0030A880, 0x400000);
    ehsys_B48A11FB(path, 0xF3F9);
    ehsys_245526EE();
}

/* func_000025D8 — Scene: restart the cursor blink.
 * MATCH 100% */
void func_000025D8(Scene *self) {
    self->unk1A = 0xFF;
    self->unk1C = -10;
}

/* func_000025EC — Scene: cursor to the top-left tile (after a page
 * change).
 * MATCH 100% */
void func_000025EC(Scene *self) {
    self->unk18 = 2;
    self->unk1D = 0;
    self->unk1E = 0;
    self->unk28 = (float) (self->unk1D * 58 + 0x2C);
    self->unk2C = (float) ((self->unk1E + 1) * 36);
    self->unk30 = 0.25f;
}

/* func_00002650 — Scene: send the cursor off screen (exit).
 * MATCH 100% */
void func_00002650(Scene *self) {
    self->unk28 = 224.0f;
    self->unk2C = 280.0f;
    self->unk18 = 2;
    self->unk30 = D_00009A10;
}

/* func_00002678 — Scene: cursor. unk18: 1 = idle (blink, d-pad moves the
 * cell, 7 columns, wrapping), 2 = easing towards the target cell.
 * MATCH 100% */
void func_00002678(Scene *self) {
    Pad *pad;
    signed char row;
    int moved;

    pad = ehsys_F5E3080C();
    switch (self->unk18) {
    case 0:
        break;
    case 1:
        self->unk1A += self->unk1C;
        if (self->unk1A < 0x60) {
            self->unk1A = 0x60;
            self->unk1C = -self->unk1C;
        } else if (self->unk1A > 0xFF) {
            self->unk1A = 0xFF;
            self->unk1C = -self->unk1C;
        }
        row = self->unk1E;
        moved = 0;
        if (pad->trigger & 0x80) {
            func_000025D8(self);
            if (self->unk1D == 0 && self->unk1E == 0) {
                self->unk1D = (D_00009898[self->page] - 1) % 7;
                self->unk1E = (D_00009898[self->page] - 1) / 7;
            } else if (self->unk1D == 0) {
                self->unk1D = 6;
                self->unk1E--;
            } else {
                self->unk1D = (self->unk1D + 6) % 7;
                if (self->unk1D + self->unk1E * 5 >= D_00009898[self->page]) {
                    self->unk1D = (D_00009898[self->page] - 1) % 7;
                }
            }
            moved = 1;
        } else if (pad->trigger & 0x20) {
            func_000025D8(self);
            if (self->unk1D + self->unk1E * 7 >= D_00009898[self->page] - 1) {
                self->unk1D = 0;
                self->unk1E = 0;
            } else {
                self->unk1D = (self->unk1D + 8) % 7;
                if (self->unk1D == 0) {
                    self->unk1D = 0;
                    self->unk1E++;
                }
            }
            moved = 1;
        }
        if (pad->trigger & 0x10) {
            func_000025D8(self);
            if (self->unk1E == 0) {
                self->unk1E = D_00009898[self->page] / 7;
                if (self->unk1D + self->unk1E * 7 >= D_00009898[self->page]) {
                    self->unk1D = D_00009898[self->page] % 7 - 1;
                }
            } else {
                self->unk1E--;
            }
            moved = 1;
        } else if (pad->trigger & 0x40) {
            func_000025D8(self);
            self->unk1E++;
            if (self->unk1D + self->unk1E * 7 >= D_00009898[self->page]) {
                if (self->unk1E * 7 <= D_00009898[self->page]) {
                    self->unk1D = D_00009898[self->page] % 7 - 1;
                } else {
                    self->unk1E = 0;
                }
            }
            moved = 1;
        }
        if (moved) {
            ehsys_2AE3C0AD(0x94C3);
            self->unk18 = 2;
            self->unk28 = (float) (self->unk1D * 58 + 0x2C);
            self->unk2C = (float) ((self->unk1E + 1) * 36);
            self->unk30 = D_00009A0C;
            func_00001168(self->list, row, self->unk1E, self->page);
            func_0000356C(self, 1);
        }
        break;
    case 2:
        self->unk20 = self->unk20 + self->unk30 * (self->unk28 - self->unk20);
        self->unk24 = self->unk24 + self->unk30 * (self->unk2C - self->unk24);
        if ((self->unk28 - self->unk20) * (self->unk28 - self->unk20) + (self->unk2C - self->unk24) * (self->unk2C - self->unk24) < D_00009A08) {
            self->unk18 = 1;
            self->unk20 = self->unk28;
            self->unk24 = self->unk2C;
            self->unk11 = 1;
        }
        func_000025D8(self);
        break;
    }
}

/* func_00002AE8 — Scene: draw a full-screen black quad.
 * MATCH 100% */
void func_00002AE8(Scene *self) {
    int ctx;

    ctx = ehsys_DFCA450B(5, 0);
    ctx = ehsys_4175E3DB(ctx, 1);
    ctx = ehsys_4175E3DB(ctx, 9);
    ctx = ehsys_F462EE6E(ctx, 4);
    ctx = ehsys_5CBB9E43(ctx, 0, 2, 3, 0, 0);
    ctx = ehsys_09794DF0(ctx, 0, 1);
    ctx = ehsys_CABEA834(ctx, 0xFF000000);
    ctx = ehsys_8EA6B1E7(ctx, 0, 0, 0x1E0, 0x110);
    ctx = ehsys_FDD168CA(ctx, 0, 0, 0, 0, 0, 0, 0);
    ehsys_1EC5342B(ctx);
}

/* func_00002BA8 — Scene: full-screen black quad with alpha unk50.
 * MATCH 100% */
void func_00002BA8(Scene *self) {
    int ctx;

    if (self->unk50 == 0) {
        return;
    }
    ctx = ehsys_DFCA450B(4, 0);
    ctx = ehsys_F462EE6E(ctx, 1);
    ctx = ehsys_4175E3DB(ctx, 9);
    ctx = ehsys_F462EE6E(ctx, 4);
    ctx = ehsys_5CBB9E43(ctx, 0, 2, 3, 0, 0);
    ctx = ehsys_09794DF0(ctx, 0, 1);
    ctx = ehsys_CABEA834(ctx, (unsigned char) self->unk50 << 24);
    ctx = ehsys_8EA6B1E7(ctx, 0, 0, 0x1E0, 0x110);
    ctx = ehsys_FDD168CA(ctx, 0, 0, 1, 0, 0, 0, 0);
    ehsys_1EC5342B(ctx);
}

/* func_00002C80 — Scene: draw the zoomed picture (unk5C texture in the
 * rectangle unk38..unk44), for character clips also its two models, then
 * the black overlay with alpha 0xFF - unk4C.
 * MATCH 100% */
void func_00002C80(Scene *self) {
    DrawParam p;
    int ctx;
    int id;
    int m1;
    int m2;
    int a;

    if (self->unk5C == 0) {
        return;
    }
    if (self->unk54 < 0xF) {
        return;
    }
    ctx = ehsys_DFCA450B(4, 0);
    ctx = ehsys_F462EE6E(ctx, 1);
    ctx = ehsys_F462EE6E(ctx, 9);
    ctx = ehsys_F462EE6E(ctx, 4);
    ctx = ehsys_5CBB9E43(ctx, 0, 2, 3, 0, 0);
    ctx = ehsys_CABEA834(ctx, 0xFFFFFFFF);
    ctx = ehsys_09794DF0(ctx, 0, 1);
    ctx = ehsys_9EA6989A(ctx, (int) self->unk5C);
    ctx = ehsys_E6E4401F(ctx, self->unk48, self->unk48);
    ctx = ehsys_8EA6B1E7(ctx, 0x1E0, 0x110, (int) (self->unk40 - self->unk38), (int) (self->unk44 - self->unk3C));
    ctx = ehsys_FDD168CA(ctx, (int) self->unk38, (int) self->unk3C, 2, 0, 0, 0, 0);
    if (self->unk76 >= 0x15 && self->unk76 < 0x5D) {
        id = self->unk76 + 3;
        m1 = func_00004028(self->player, id, 0);
        m2 = func_00004028(self->player, id, 1);
        if (m1) {
            ehsys_memset(&p, 0, sizeof(p));
            p.unk0 = *ehsys_B2B9FEFA();
            p.flags = 0x30004;
            p.x = self->unk38 + (float) func_000041E8(self->player, id);
            p.y = self->unk3C;
            p.sx = (self->unk40 - self->unk38) / 480.0f;
            p.sy = (self->unk44 - self->unk3C) / 272.0f;
            p.color[2] = 0xFF;
            p.color[1] = 0xFF;
            p.color[0] = 0xFF;
            p.color[3] = 0xFF;
            ctx = ehsys_85F89FE8(ctx);
            ctx = ehsys_7187E0F0(ctx, m1, (int *) &p);
            ctx = ehsys_7187E0F0(ctx, m2, (int *) &p);
            ctx = ehsys_89812D8A(ctx);
        }
    }
    a = 0xFF - self->unk4C;
    ctx = ehsys_F462EE6E(ctx, 1);
    ctx = ehsys_4175E3DB(ctx, 9);
    ctx = ehsys_F462EE6E(ctx, 4);
    ctx = ehsys_5CBB9E43(ctx, 0, 2, 3, 0, 0);
    ctx = ehsys_09794DF0(ctx, 0, 1);
    ctx = ehsys_CABEA834(ctx, (unsigned char) a << 24);
    ctx = ehsys_8EA6B1E7(ctx, 0, 0, 0x1E0, 0x110);
    ctx = ehsys_FDD168CA(ctx, 0, 0, 0xA, 0, 0, 0, 0);
    ehsys_1EC5342B(ctx);
}

/* func_00002F94 — Scene: draw the gallery.gim header strip.
 * MATCH 100% */
void func_00002F94(Scene *self) {
    int ctx;

    if (func_00003D84(self->player, 0) == 0) {
        return;
    }
    if (func_00001158(self->list)) {
        ctx = ehsys_DFCA450B(4, 0);
        ctx = ehsys_F462EE6E(ctx, 9);
        ctx = ehsys_F462EE6E(ctx, 4);
        ctx = ehsys_5CBB9E43(ctx, 0, 2, 3, 0, 0);
        ctx = ehsys_09794DF0(ctx, 0, 1);
        ctx = ehsys_CABEA834(ctx, 0xFFFFFFFF);
        ctx = ehsys_9EA6989A(ctx, self->unk6C);
        ctx = ehsys_E6E4401F(ctx, 0, 0);
        ctx = ehsys_8EA6B1E7(ctx, 8, 8, 0x1A8, 8);
        ctx = ehsys_FDD168CA(ctx, 0x28, 0xE0, 0, 0x10, 0x30, 0, 0);
        ehsys_1EC5342B(ctx);
    }
}

/* func_00003094 — Scene: draw the cursor frame (grey level unk1A) at
 * (unk20, unk24) relative to the page's scroll.
 * MATCH 100% */
void func_00003094(Scene *self) {
    int a = (unsigned char) self->unk1A;
    int ctx;

    ctx = ehsys_DFCA450B(4, 0);
    ctx = ehsys_F462EE6E(ctx, 1);
    ctx = ehsys_F462EE6E(ctx, 9);
    ctx = ehsys_F462EE6E(ctx, 4);
    ctx = ehsys_5CBB9E43(ctx, 0, 2, 0xA, 0, -1);
    ctx = ehsys_CABEA834(ctx, a | (a << 8) | (a << 16) | (a << 24));
    ctx = ehsys_09794DF0(ctx, 0, 1);
    ctx = ehsys_20560342(ctx, 0x2C, 0x24, 0x198, 0xB2);
    ctx = ehsys_F462EE6E(ctx, 2);
    ctx = ehsys_9EA6989A(ctx, self->unk6C);
    ctx = ehsys_E6E4401F(ctx, 0, 0);
    ctx = ehsys_FDD168CA(ehsys_8EA6B1E7(ctx, 0x3C, 0x24, 0x3C, 0x24), (int) self->unk20, (int) ((float) (int) self->unk24 + func_00001328(self->list, self->page)), 0, 0x40, 0, 0, 0);
    ctx = ehsys_4175E3DB(ctx, 2);
    ehsys_1EC5342B(ctx);
}

/* func_00003204 — empty.
 * MATCH 100% */
void func_00003204(Scene *self) {
}

/* func_0000320C — Scene: draw the help line (string 0xB3 of strTbl.bin
 * from archive file 0x5E).
 * MATCH 100% */
void func_0000320C(Scene *self) {
    int ctx;
    unsigned short saved;

    if (func_00003D84(self->player, 0) == 0) {
        return;
    }
    if (func_00001158(self->list)) {
        ctx = ehsys_DFCA450B(4, 0);
        saved = ehsys_0D961646();
        ehsys_B8AD96EA(ctx);
        ehsys_C9D9E989(1);
        ehsys_942B03D0(0xE, 0xE);
        ehsys_60B55A50(-1);
        func_000032F0(0x28, 0x10, ehsys_E58C0FDC(ehsys_C6C15111(func_00003D84(self->player, 0x5E), D_0000A318, 0), 0xB3));
        ehsys_06380DFA(saved);
        ehsys_1EC5342B(ehsys_A4AFF8E6());
    }
}

/* func_000032F0 — draw a string from x to the right edge of the screen.
 * MATCH 100% */
void func_000032F0(int x, int y, void *str) {
    ehsys_B89D38DC(x << 6, y << 6, (0x1E0 - x) << 6, str);
}

/* func_00003310 — Scene: draw the page tabs (the current one lit).
 * MATCH 100% */
void func_00003310(Scene *self) {
    int ctx;

    if (func_00003D84(self->player, 0) == 0) {
        return;
    }
    if (func_00001158(self->list)) {
        ctx = ehsys_DFCA450B(4, 0);
        ctx = ehsys_F462EE6E(ctx, 9);
        ctx = ehsys_F462EE6E(ctx, 4);
        ctx = ehsys_5CBB9E43(ctx, 0, 2, 3, 0, 0);
        ctx = ehsys_09794DF0(ctx, 0, 1);
        ctx = ehsys_CABEA834(ctx, 0xFFFFFFFF);
        ctx = ehsys_9EA6989A(ctx, self->unk6C);
        ctx = ehsys_E6E4401F(ctx, 0, 0);
        ctx = ehsys_8EA6B1E7(ctx, 0xC, 8, 0xC, 8);
        ctx = ehsys_FDD168CA(ctx, 0x18C, 0x1A, 0, 0, 0x38, 0, 0);
        ctx = ehsys_8EA6B1E7(ctx, 0xC, 8, 0xC, 8);
        ctx = ehsys_FDD168CA(ctx, 0x1B4, 0x1A, 0, 0xC, 0x38, 0, 0);
        ctx = ehsys_8EA6B1E7(ctx, 8, 8, 8, 8);
        ctx = ehsys_FDD168CA(ctx, 0x19C, 0x1A, 0, self->page == 0 ? 8 : 0, 0x30, 0, 0);
        ctx = ehsys_8EA6B1E7(ctx, 8, 8, 8, 8);
        ctx = ehsys_FDD168CA(ctx, 0x1A8, 0x1A, 0, self->page == 1 ? 8 : 0, 0x30, 0, 0);
        ehsys_1EC5342B(ctx);
    }
}

/* func_000034E0 — Scene: index of the movie under the cursor.
 * MATCH 100% */
int func_000034E0(Scene *self) {
    return self->unk1D + (D_0000989C[self->page] + self->unk1E * 7);
}

/* func_00003510 — Scene: string i of strTbl.bin (archive file 0x5E).
 * MATCH 100% */
void *func_00003510(Scene *self, int i) {
    return ehsys_E58C0FDC(ehsys_C6C15111(func_00003D84(self->player, 0x5E), D_0000A318, 0), D_000098A0[i]);
}

/* func_0000356C — Scene: refresh text `which` (0 page title, 1 the
 * description of the movie under the cursor, or the "locked" line 0xB2).
 * MATCH 100% */
void func_0000356C(Scene *self, int which) {
    switch (which) {
    case 0:
        self->unk78 = func_00003510(self, D_000099E8[self->page]);
        self->fade[0] = 1;
        break;
    case 1:
        if (self->unlocked[func_000034E0(self)] == 1) {
            ehsys_025FE238(self->unk7C, func_00003510(self, func_000034E0(self) + 4));
        } else {
            ehsys_E58C0FDC(ehsys_C6C15111(func_00003D84(self->player, 0x5E), D_0000A318, 0), 0xB2);
        }
        self->fade[1] = 1;
        break;
    }
}

/* func_00003640 — Scene: draw the description text, fading it in.
 * MATCH 100% */
void func_00003640(Scene *self) {
    int ctx;
    unsigned short saved;
    int i;
    int a;

    ctx = ehsys_DFCA450B(4, 0);
    ctx = ehsys_4175E3DB(ctx, 1);
    ctx = ehsys_F462EE6E(ctx, 9);
    ctx = ehsys_F462EE6E(ctx, 4);
    ctx = ehsys_5CBB9E43(ctx, 0, 2, 3, 0, 0);
    ctx = ehsys_09794DF0(ctx, 0, 1);
    ctx = ehsys_CABEA834(ctx, 0xFFFFFFFF);
    saved = ehsys_0D961646();
    ehsys_B8AD96EA(ctx);
    ehsys_C9D9E989(1);
    ehsys_942B03D0(0xE, 0xE);
    for (i = 0; i < 2; i++) {
        if (self->fade[i] < 0xFF) {
            self->fade[i] += 0x20;
            if (self->fade[i] > 0xFF) {
                self->fade[i] = 0xFF;
            }
        }
    }
    if (ehsys_82E739A4(self->unk7C)) {
        a = (unsigned char) self->fade[1];
        ehsys_60B55A50(a | (a << 8) | (a << 16) | (a << 24));
        ehsys_C792FA38(1);
        func_000037C0(0x30, 0xE9, 0x17C, self->unk7C);
        ehsys_C792FA38(0);
    }
    ehsys_06380DFA(saved);
    ehsys_1EC5342B(ehsys_A4AFF8E6());
}

/* func_000037C0 — draw a typed message (x, y, width in pixels).
 * MATCH 100% */
void func_000037C0(int x, int y, int w, char *str) {
    ehsys_BC8E65D7(x << 6, y << 6, w << 6, str, -1);
}

/* func_000037D4 — Scene: the scroll bar (thumb size and position from the
 * page's row count and scroll row) with blinking up/down arrows.
 * MATCH 100% */
void func_000037D4(Scene *self) {
    float s;
    int rows;
    int top;
    int ctx;
    int y;
    int h;

    if (!func_00001158(self->list)) {
        return;
    }
    D_00755880 += 5;
    s = ehsys_A38AA0E9(D_00009A04 * (float) (D_00755880 % 180) / 360.0f);
    rows = D_00009A00[self->page] / 7 + (D_00009A00[self->page] % 7 != 0);
    top = func_00001358(self->list, self->page);
    ctx = ehsys_DFCA450B(4, 0);
    ctx = ehsys_F462EE6E(ctx, 1);
    ctx = ehsys_F462EE6E(ctx, 9);
    ctx = ehsys_F462EE6E(ctx, 4);
    ctx = ehsys_5CBB9E43(ctx, 0, 2, 3, 0, 0);
    ctx = ehsys_CABEA834(ctx, 0xFFFFFFFF);
    ctx = ehsys_09794DF0(ctx, 0, 1);
    ctx = ehsys_9EA6989A(ctx, self->unk6C);
    ctx = ehsys_E6E4401F(ctx, 0, 0);
    ctx = ehsys_8EA6B1E7(ctx, 8, 1, 8, 1);
    ctx = ehsys_FDD168CA(ctx, 0x1C5, 0x26, 0, 0x37, 0, 0, 0);
    ctx = ehsys_8EA6B1E7(ctx, 8, 1, 8, 1);
    ctx = ehsys_FDD168CA(ctx, 0x1C5, 0xCF, 0, 0x37, 0x13, 0, 0);
    ctx = ehsys_8EA6B1E7(ctx, 8, 0x12, 8, 0xA8);
    ctx = ehsys_FDD168CA(ctx, 0x1C5, 0x27, 0, 0x37, 1, 0, 0);
    ctx = ehsys_8EA6B1E7(ctx, 4, 4, 4, 4);
    ctx = ehsys_FDD168CA(ctx, 0x1C7, 0x1E, 0, 0x30, 0x12, 0, 0);
    ctx = ehsys_8EA6B1E7(ctx, 4, 4, 4, 4);
    ctx = ehsys_FDD168CA(ctx, 0x1C7, 0xD4, 0, 0x30, 0x18, 0, 0);
    y = 0;
    h = 0xAA;
    if (rows > 5) {
        y = (int) ((float) top * (170.0f / (float) rows));
        h = (int) (170.0f * (5.0f / (float) rows));
    }
    ctx = ehsys_8EA6B1E7(ctx, 4, 0x12, 4, h);
    ctx = ehsys_FDD168CA(ctx, 0x1C7, y + 0x27, 0, 0x37, 0x14, 0, 0);
    ctx = ehsys_CABEA834(ctx, ((unsigned int) (255.0f * s) << 24) | 0xFFFFFF);
    if (top != 0) {
        ctx = ehsys_8EA6B1E7(ctx, 4, 4, 4, 4);
        ctx = ehsys_FDD168CA(ctx, 0x1C7, 0x1E, 0, 0x30, 2, 0, 0);
    }
    if (top < rows - 5) {
        ctx = ehsys_8EA6B1E7(ctx, 4, 4, 4, 4);
        ctx = ehsys_FDD168CA(ctx, 0x1C7, 0xD4, 0, 0x30, 8, 0, 0);
    }
    ehsys_1EC5342B(ctx);
}

/* func_00003BCC — Scene::draw().
 * MATCH 100% */
void func_00003BCC(Scene *self) {
    int ctx;

    func_000042C0(self);
    if (func_00003D84(self->player, 0) == 0) {
        return;
    }
    ((ItemListV *) self->list)->draw();
    func_000037D4(self);
    func_00003094(self);
    func_00003204(self);
    func_0000320C(self);
    func_00002F94(self);
    func_00003640(self);
    func_00003310(self);
    func_00002BA8(self);
    func_00002C80(self);
    ctx = ehsys_DFCA450B(5, 0);
    ctx = ehsys_4175E3DB(ctx, 1);
    ehsys_878EE1D3(&ctx);
    ehsys_1EC5342B(ctx);
    if (self->unk10 == 1) {
        func_00002AE8(self);
    }
}

/* func_00003CA8 — LoadReq completion callback: fix up EHP archives, store
 * the buffer in its slot, delete the request.
 * The `LoadReq *` must be a local declared INSIDE the if-block (copied
 * from a `void *` parameter): only then does MWCC emit the copy to $s0
 * after the `blez` and put the prologue's `sw $s0` in its delay slot.
 * MATCH 100% */
int func_00003CA8(void *arg, int result) {
    if (result > 0) {
        LoadReq *req = (LoadReq *) arg;

        if (ehsys_memcmp(req->buf, D_0000A778, 3) == 0) {
            ehsys_4B0DABFA(0, req->buf);
        }
        func_00003D6C(req->owner);
        func_00003D7C(req->owner, req->slot, req->buf);
        ehsys_9455886E(req);
        return 1;
    }
    return 0;
}

/* func_00003D24 — Player::init().
 * MATCH 100% */
void func_00003D24(Player *self) {
    ehsys_memset(self->files, 0, sizeof(self->files));
    self->pending = 0;
    self->packed = 0;
    self->mdl[1] = 0;
    self->mdl[0] = 0;
    self->packed_size = 0;
    self->ready = 1;
}

/* func_00003D6C — Player: one load finished.
 * MATCH 100% */
void func_00003D6C(Player *self) {
    self->pending--;
}

/* func_00003D7C — Player: store a loaded file.
 * MATCH 100% */
void func_00003D7C(Player *self, void **slot, void *data) {
    *slot = data;
}

/* func_00003D84 — Player: file in slot i (0 while loading).
 * MATCH 100% */
int func_00003D84(Player *self, int i) {
    return (int) self->files[i];
}

/* func_00003D94 — Player: start loading file `id` (0x5E = the language
 * archive, 0 = database/gallery/basic.ehp, else field/event/<lang>/
 * ev5_NN.gim).
 * MATCH 100% */
void func_00003D94(Player *self, int id) {
    char path[0x50];
    int lang;
    int size;
    LoadReq *req;
    int dev;

    lang = ehsys_get_language();
    if (id == 0x5E) {
        ehsys_sprintf(path, D_0000A77C, D_0000A39C[lang]);
    } else if (id == 0) {
        ehsys_sprintf(path, D_0000A790, D_00009A14[id].name);
    } else {
        ehsys_sprintf(path, D_0000A794, D_0000A414[lang], D_00009A14[id].name);
    }
    dev = D_00009A14[id].dev;
    size = ehsys_D2A768F4(dev, path);
    if (id > 0 && id < 0x5E) {
        self->unk180 = size;
    }
    req = (LoadReq *) ehsys_57345021(sizeof(LoadReq), D_0030A850);
    func_00003F20(req);
    req->slot = &self->files[id];
    req->owner = self;
    req->buf = ehsys_19AC3ED4(size, D_0030A850);
    ehsys_8288EEBC(dev, path, req->buf, size, func_00003CA8, req);
    self->pending++;
}

/* func_00003F20 — LoadReq::LoadReq().
 * MATCH 100% */
void func_00003F20(LoadReq *self) {
    self->slot = 0;
    self->owner = 0;
    self->buf = 0;
}

/* func_00003F30 — Player: free every file.
 * MATCH 100% */
void func_00003F30(Player *self) {
    int i;

    for (i = 0; i < 0x5F; i++) {
        if (self->files[i]) {
            ehsys_5CEDF44D(self->files[i]);
            self->files[i] = 0;
        }
    }
}

/* func_00003F94 — Player::term().
 * MATCH 100% */
void func_00003F94(Player *self) {
    func_00003F30(self);
}

/* func_00003F9C — Player: select and load event picture `id`.
 * MATCH 100% */
void func_00003F9C(Player *self, int id) {
    self->unk188 = id;
    func_00003D94(self, id);
}

/* func_00003FA4 — Player: free the current picture and the bust-up.
 * MATCH 100% */
void func_00003FA4(Player *self) {
    if (self->files[self->unk188]) {
        ehsys_5CEDF44D(self->files[self->unk188]);
        self->files[self->unk188] = 0;
    }
    if (self->packed) {
        ehsys_5CEDF44D(self->packed);
        self->packed = 0;
    }
    if (self->unpacked) {
        ehsys_5CEDF44D(self->unpacked);
        self->unpacked = 0;
    }
    self->packed_size = 0;
}

/* func_00004028 — Player: bust-up model `which` (0 body, 1 mouth) of
 * character `id`; starts the load on first call, returns 0 until ready.
 * MATCH 100% */
int func_00004028(Player *self, int id, int which) {
    char path[0x100];
    int *slot;
    int n;
    int *e;
    int a;
    int b;

    if (self->ready == 0) {
        return 0;
    }
    /* &self->mdl[which], spelled so that MWCC adds index + base in the
     * target's operand order (every natural spelling gives base + index) */
    slot = (int *) ((char *) (which * 4) + (int) self + 0x194);
    if (*slot) {
        return *slot;
    }
    if (self->packed) {
        n = ehsys_7CBFBEC2(self->packed, self->packed_size);
        self->unpacked = ehsys_41AABF28(D_0030A850, n);
        ehsys_C204E9F8(self->unpacked, n, self->packed);
        ehsys_4B0DABFA(0, self->unpacked);
        self->mdl[0] = ehsys_C6C15111((int) self->unpacked, D_0000A7A0, 0);
        self->mdl[1] = ehsys_C6C15111((int) self->unpacked, D_0000A7AC, 0);
        n = id - 0x18;
        if (id >= 0x5A) {
            n -= 7;
        }
        e = ehsys_9E72EF23(id, 0, D_0000A538[n].motion);
        a = 0;
        b = 1;
        if (e) {
            a = e[3];
            b = a + 1;
        }
        ehsys_831C5769(self->mdl[0], a, b);
        ehsys_831C5769(self->mdl[1], a, b);
        return *slot;
    }
    ehsys_sprintf(path, D_0000A7B8, ehsys_7754CFD2(id));
    self->packed_size = ehsys_D2A768F4(3, path);
    self->ready = 0;
    self->packed = ehsys_41AABF28(D_0030A850, self->packed_size);
    ehsys_8288EEBC(3, path, self->packed, self->packed_size, (int (*)(void *, int)) func_000041DC, &self->ready);
    return 0;
}

/* func_000041DC — bust-up load callback: ready = (result > 0).
 * MATCH 100% */
int func_000041DC(int *flag, int result) {
    return *flag = result > 0;
}

/* func_000041E8 — Player: x offset of character id's bust-up model.
 * MATCH 100% */
int func_000041E8(Player *self, int id) {
    int n;

    if (id >= 0x18) {
        n = id - 0x18;
        if (id >= 0x5A) {
            n -= 7;
        }
        return D_0000A538[n].x;
    }
    return 0;
}

/* func_00004220 — create the root task.
 * MATCH 100% */
void func_00004220(void) {
    Task *t = (Task *) ehsys_57345021(sizeof(Task), D_0030A850);

    if (t) {
        t = func_000044AC(t);
    }
    D_00755884 = t;
}

/* func_00004264 — run update() of every enabled task.
 * MATCH 100% */
void func_00004264(void) {
    Task *t;

    for (t = D_00755884->upd.next; t; t = t->upd.next) {
        if (t->upd_on == 1) {
            ((TaskV *) t)->update();
        }
    }
}

/* func_000042B8 — Task::update() (empty).
 * MATCH 100% */
void func_000042B8(Task *self) {
}

/* func_000042C0 — run draw() of every enabled task.
 * MATCH 100% */
void func_000042C0(Scene *scene) {
    Task *t;

    for (t = D_00755884->drw.next; t; t = t->drw.next) {
        if (t->drw_on == 1) {
            ((TaskV *) t)->draw();
        }
    }
}

/* func_00004314 — Task::draw() (empty).
 * MATCH 100% */
void func_00004314(Task *self) {
}

/* func_0000431C — delete every task, then the root.
 * MATCH 100% */
void func_0000431C(Scene *scene) {
    Task *t;

    while ((t = D_00755884->upd.next) != 0) {
        func_0000444C(t, 1);
        delete (TaskV *) t;
    }
    delete (TaskV *) D_00755884;
    D_00755884 = 0;
}

/* func_00004394 — Task vtable slot 3 (empty).
 * MATCH 100% */
void func_00004394(Task *self) {
}

/* func_0000439C — unlink t from the update list.
 * MATCH 100% */
void func_0000439C(Task *t) {
    Task *p;

    for (p = D_00755884->upd.next; p; p = p->upd.next) {
        if (p == t) {
            t->upd.unk0 = 0;
            t->upd.prev->upd.next = t->upd.next;
            if (t->upd.next) {
                t->upd.next->upd.prev = t->upd.prev;
            }
            t->upd.prev = 0;
            t->upd.next = 0;
            return;
        }
    }
}

/* func_000043F4 — unlink t from the draw list.
 * MATCH 100% */
void func_000043F4(Task *t) {
    Task *p;

    for (p = D_00755884->drw.next; p; p = p->drw.next) {
        if (p == t) {
            t->drw.unk0 = 0;
            t->drw.prev->drw.next = t->drw.next;
            if (t->drw.next) {
                t->drw.next->drw.prev = t->drw.prev;
            }
            t->drw.prev = 0;
            t->drw.next = 0;
            return;
        }
    }
}

/* func_0000444C — detach t from both lists; flag 1 also calls its slot 6.
 * MATCH 100% */
void func_0000444C(Task *t, unsigned char flag) {
    func_0000439C(t);
    func_000043F4(t);
    if (flag == 1) {
        ((TaskV *) t)->unk6();
    }
}

/* func_000044A4 — Task vtable slot 6 (empty).
 * MATCH 100% */
void func_000044A4(Task *self) {
}

/* func_000044AC — Task::Task().
 * MATCH 100% */
Task *func_000044AC(Task *self) {
    self->vptr = &D_0000A810;
    func_000044F4(&self->upd);
    func_000044F4(&self->drw);
    self->upd_on = 1;
    self->drw_on = 1;
    return self;
}

/* func_000044F4 — TaskLink::TaskLink().
 * MATCH 100% */
void func_000044F4(TaskLink *self) {
    self->unk0 = 0;
    self->prev = 0;
    self->next = 0;
}

/* func_00004504 — Task::~Task() (deleting).
 * MATCH 100% */
Task *func_00004504(Task *self, short flag) {
    if (self) {
        self->vptr = &D_0000A810;
        if (flag > 0) {
            ehsys_9455886E(self);
        }
    }
    return self;
}

} /* extern "C" */

/*
 * rel_password.prx — reconstructed code (matching decompilation)
 *
 * The "password machine" of the Lab (labo) menu: the player types an 8-digit
 * card password on a 3x5 keypad, the digits light up on eight nixie tubes,
 * and a correct password adds that card to the save data. Module `modpassword`,
 * exports `libpassword_rel` (func_00000000 = draw, func_000015A4 = init,
 * func_00001688 = per-frame update). How a password becomes a card is
 * written up in docs/modules/rel_password.md; the short version is in
 * func_0000241C below.
 *
 * Compiler:     Metrowerks CodeWarrior for PSP — MWCC 1.3 SP7 (mwccpsp_3.0.1_219)
 * Flags:        -O4,s -sdatathreshold 0
 * Verification: every function below is "MATCH 100%": byte-identical after
 *               linking (all 41 / 0x37B8 bytes of .text), and
 *               `make MODULE=rel_password SRC=1` reproduces the shipped
 *               module's sha1.
 *
 * Conventions are those of src/rel_movie_viewer.c (read its header first).
 * What was new in this module:
 *
 *  - ONE STATE STRUCT, NOT LOOSE GLOBALS. Everything lives in a 0x1150-byte
 *    struct at 0x1DC50 (memset as a whole by func_000015A4). The target
 *    re-materialises `lui 0x2` before every field access — that is MWCC's
 *    normal codegen for a plain (non-volatile) global struct, so no
 *    volatile/local-pointer trick is needed here. Where the target DOES keep
 *    the address in a saved register (`lui %hi(D_1DC54)` + `addiu` into $s3
 *    in func_00001D54/func_0000241C, $v0 in func_00003674) the original took
 *    a local pointer to the cursor sub-struct, and so does this file.
 *  - BITFIELDS EVERYWHERE: cursor column/row/animation, digit count, two
 *    flags and the loader state are unsigned-char bitfields, allocated from
 *    the LSB. The `andi $vX, $zero, 0xF` + mask + `or` + `sb` sequences in
 *    the target are MWCC storing a constant into a bitfield.
 *  - SWITCH SHAPE: MWCC tests the cases of a compare-chain switch in REVERSE
 *    source order and lays the bodies out in source order (func_00003674
 *    only matched once its cases were written 12, 11, 0, 10). `x == A ||
 *    x == B` tested as `beq A; beq B; b skip` is a two-label `case A: case
 *    B:` switch, not an `||` (func_000002D0). A run of consecutive cases
 *    lowered to a range test (`bltz` + `slti 10`) is a GNU case range
 *    `case 0 ... 9:`, which this MWCC accepts (func_0000354C, func_0000241C).
 *  - LOCALS: the LATER-declared local gets the LOWER stack address
 *    (func_00001358, func_000027A8, func_000002D0).
 *  - `> 2` and `>= 3` are not interchangeable: the target's `slti $at,
 *    ..., 3` + `bnez $at` comes from `> 2`; `>= 3` puts the result in $v0
 *    (func_000002D0).
 *  - an `(int)` cast on a pointer argument changes argument scheduling; the
 *    target passes the pointer itself through a `...` prototype
 *    (ehsys_E50B9594 in func_000027A8). Likewise `ehsys_E47E40E4() +
 *    0x160000` only schedules as `lui $at` + `addu` when the callee returns
 *    `char *` (func_00000D8C).
 *  - the "pointless" double store in func_00001CE4 (`seed *= A; seed +=
 *    C;`) is real: the LCG updates its state in two statements.
 *  - absolute address 0: func_00001CE4/func_00001D2C read an index and a
 *    table at address 0 with NO relocation in the shipped PRX — two engine
 *    globals that this module's link resolved to 0. They are declared as
 *    D_00000000 / D_0000 so that the Makefile's PROVIDE()s put both at 0
 *    while MWCC still sees two distinct symbols (it re-loads `lui 0` for
 *    each, as the target does).
 *
 * DATA-LAYOUT CONSTRAINTS (why some things are not written the "natural" way).
 * `make SRC=1` links this object's .text in place of the disassembly but
 * carries .data over verbatim from the shipped module, and only places this
 * object's .rodata (NOLOAD) at the first jump table, 0x89D0. Therefore:
 *  - this file must emit NO .data and NO .bss: every global is `extern`, and
 *    no string literal or initialised local array may appear;
 *  - func_00001358's `char lang[6] = {'j','e','g','f','i','s'}` (almost
 *    certainly the original — its anonymous initialiser is exactly the
 *    6 bytes at 0x8C00) would emit .data. `*(LangCodes *) D_00008C00`
 *    produces the identical byte-pair copy loop without emitting anything;
 *  - the shipped .data holds, in order, jtbl_000089D0 (func_000002D0),
 *    0.05f / 0.1f (func_00000D8C), the exported module_sdk_version
 *    (0x06020010), jtbl_000089F8 (func_00001688), jtbl_00008A1C
 *    (func_000027A8). The floats sit between the jump tables, which one TU
 *    cannot produce (MWCC pools float literals at the END of .rodata) — the
 *    original was most likely two or more source files. Here the floats are
 *    read through `extern float D_000089EC/D_000089F0` (same lui/lwc1 as a
 *    literal), and `rodata_shim_89EC` — 12 bytes of const data defined
 *    between func_000002D0 and func_00001688 — keeps the next two jump
 *    tables at 0x89F8 / 0x8A1C. A Makefile that placed each jtbl_XXXXXXXX
 *    at its own address would make the shim unnecessary.
 *
 * No Shift-JIS text in this module: its .data strings are ASCII file names;
 * the UI text comes from strTbl_<lang>.bin inside labo/password.ehp.
 */

/* ---- types ---- */

/* keypad cursor, D_0001DC50+0x4 (2 bytes, copied whole by func_00001D54) */
typedef struct {
    unsigned char state : 4; /* key animation: 0 hover, 1 press, 2 flash, 3 still */
    unsigned char col : 4;   /* column 0..2 */
    unsigned char row : 4;   /* row 0 = OK, 1 = clear/0/back, 2..4 = 1..9 */
} Cursor;

typedef struct {
    unsigned char n : 4; /* digits entered so far, 0..8 */
} Count;

typedef struct {
    unsigned char intro_done : 1; /* keypad is live; stops the intro's random key flashing */
    unsigned char typing : 1;     /* a message is being typed out: caret drawn solid */
} Flags;

/* the module's whole state, D_0001DC50 (0x1150 bytes; .bss) */
typedef struct {
    unsigned short mode;        /* 0x0000 func_00001688's state, 0..8 */
    unsigned short step;        /* 0x0002 sub-state of the current mode */
    Cursor cursor;              /* 0x0004 */
    Count count;                /* 0x0006 */
    unsigned char digits[8];    /* 0x0007 the password, one digit per byte */
    int password;               /* 0x0010 the 8 digits as a decimal number */
    unsigned short text_len;    /* 0x0014 length of the message being typed */
    unsigned short name_pos;    /* 0x0016 typing position in name_text */
    unsigned short text_pos;    /* 0x0018 typing position in text1 */
    unsigned short text_pos2;   /* 0x001A typing position in text2 */
    unsigned short name_out[0x49];  /* 0x001C card-name line as typed so far */
    unsigned short name_text[0x49]; /* 0x00AE card-name line, fully formatted */
    char text1[0x800];          /* 0x0140 first message window */
    char text2[0x800];          /* 0x0940 second message window */
    unsigned short card_id;     /* 0x1140 card granted by the password */
    unsigned short timer;       /* 0x1142 frame counter, reset by every step */
    Flags flags;                /* 0x1144 */
    int heap;                   /* 0x1148 the 3 MB heap at D_0001EDA0 */
    int handle;                 /* 0x114C engine handle shared by the whole module */
} PwState;

/* resource loader state, D_00008C40 */
typedef struct {
    unsigned char ready : 1; /* no request in flight */
    unsigned char step : 7;
    unsigned char pad;
} Loader;

/* one entry of the 17-entry resource table at D_00008ADC (16 bytes) */
typedef struct {
    unsigned short id;  /* 0x1001..0x1020, the table is qsorted on this */
    void *data;         /* loaded data, NULL until first requested */
    char *fmt;          /* file name inside labo/password.ehp, may hold %c */
    void *aux;          /* second file name (nixi.tmf for the tubes) */
} ResEntry;

typedef struct {
    char c[6];
} LangCodes;

/* sprite parameter block for ehsys_7187E0F0 (0x60 bytes) */
typedef struct {
    int f00;
    char pad04[0x8];
    int f0C;
    char pad10[0x4];
    float f14; /* x offset */
    char pad18[0x48];
} SprParam;

typedef struct {
    int seed;
    int pad[2];
} RandSlot;

/* pad state returned by ehsys_F5E3080C */
typedef struct {
    int f0;
    int press;  /* 0x4 newly pressed buttons */
    int f8;
    int repeat; /* 0xC auto-repeat (d-pad) */
} Pad;

/* textured vertex for ehsys_97DD82F2 */
typedef struct {
    float u, v;
    unsigned int color;
    float x, y, z;
} Vtx;

typedef struct {
    Vtx a, b;
} Line;

/* ---- module data (all of it lives in the shipped .data/.bss blob) ---- */
extern PwState D_0001DC50;         /* module state */
extern char D_0001EDA0[];          /* 3 MB heap backing PwState.heap */
extern Loader D_00008C40;
extern void *D_00008C44;           /* labo/password.ehp, once loaded */
extern char D_00008C48[];          /* 0x15000-byte work buffer */
extern ResEntry D_00008ADC[];      /* 17 entries */
extern char D_00008BEC[];          /* "labo/password.ehp" */
extern char D_00008C00[];          /* "jegfis": language letter for %c */
extern unsigned short D_00008C08[]; /* 18 card ids no password may unlock */
extern float D_000089EC;           /* 0.05f */
extern float D_000089F0;           /* 0.1f */
extern int D_00000000;             /* absolute 0, see header */
extern RandSlot D_0000[];          /* absolute 0, see header */

/* ---- imports (libehsys_rel); roles noted where this module shows them ---- */
extern void ehsys_025FE238(char *, void *);             /* start typing a message */
extern int ehsys_04E1CB0F(void);
extern int ehsys_09794DF0(int, int, int);
extern int ehsys_101103B3(int);
extern void ehsys_13A4081A(int);
extern int ehsys_1856E536(void *, int);                 /* create heap */
extern void ehsys_1CC5878C(void *);
extern int ehsys_1D98951B(int, int);
extern void ehsys_1EC5342B(int);                        /* submit display list */
extern int ehsys_20560342(int, int, int, int, int);
extern int ehsys_2717C28C(int);
extern unsigned short ehsys_28BE130B(char *);           /* message length */
extern int ehsys_28F7224B(int);                         /* card id is valid */
extern int ehsys_291D6262(void *, int, int);
extern void ehsys_2AE3C0AD(int);                        /* play sound effect */
extern int ehsys_2FD4863E(void);
extern int ehsys_316D8C28(void *, void *);              /* size of an archive member */
extern void ehsys_33ED567B(int);
extern int ehsys_379BEA78(void);
extern int ehsys_399AC68A(int, int);
extern void ehsys_3BB2BAC6(int, int, int, int, void *, int, int); /* draw text box */
extern int ehsys_3DE5C8DC(int, int, int, int, int, void *);
extern int ehsys_4175E3DB(int, int);
extern void *ehsys_41AABF28(int, int);                  /* heap alloc */
extern int ehsys_42110BF9(int, int);
extern int ehsys_450CD6C5(void);                        /* fade finished */
extern void ehsys_4AA58320(int);
extern void ehsys_4B0DABFA(int, void *);
extern int ehsys_558B2DCB(void *);                      /* animation: current frame */
extern int ehsys_572A577B(int, int, int, int, int);
extern void ehsys_5998BFD7(int, int, int);              /* fade out */
extern int ehsys_5CBB9E43(int, int, int, int, int, int);
extern void ehsys_5DBE93DB(int);
extern int ehsys_5DE4869F(int, int, int, int, int, int);
extern unsigned char ehsys_5F00A362(int, void *);
extern void ehsys_60B55A50(int);                        /* text colour */
extern void ehsys_6367AE39(int);
extern void ehsys_65DA0419(int);                        /* play sound effect */
extern unsigned int ehsys_66F8E113(void *);             /* text width */
extern void ehsys_6E42DDCD(void);
extern int ehsys_7187E0F0(int, void *, void *);
extern void ehsys_7B8EF3EC(char *, void *);
extern unsigned int ehsys_826AE59C(void);
extern void ehsys_8288EEBC(int, char *, void *, int, int (*)(void *, int), void *); /* async load */
extern void ehsys_831C5769(void *, int, int);           /* animation: play frame range */
extern int ehsys_85F89FE8(int);
extern float ehsys_8637BCB2(float);                     /* sine */
extern void ehsys_878EE1D3(int *);
extern int ehsys_89812D8A(int);
extern void ehsys_8C5B148E(int, int, int);              /* give card */
extern int ehsys_8EA6B1E7(int, int, int, int, int);
extern void ehsys_942B03D0(int, int);                   /* font size */
extern int ehsys_97DD82F2(int, int, int, int, int, void *);
extern int ehsys_9EA6989A(int, void *);
extern int ehsys_A4AFF8E6(void);                        /* end text batch */
extern int ehsys_AA9913C7(int);
extern void ehsys_AB962AE7(int);
extern int ehsys_ACAC5C40(int);                         /* password -> card id */
extern void *ehsys_AD0E1E03(unsigned short);            /* card name */
extern void ehsys_AEA7AA02(unsigned short);
extern int *ehsys_B2B9FEFA(void);                       /* frame counter */
extern int ehsys_B3B5D3CF(void);                        /* text cursor y */
extern int ehsys_B401BB25(unsigned short);
extern int ehsys_B4F2178B(int);
extern void ehsys_B8770B92(void *, int *, int *);       /* animation: frame range */
extern void ehsys_B89D38DC(int, int, int, void *);      /* draw text */
extern void ehsys_B8AD96EA(int);                        /* begin text batch */
extern int ehsys_B9064026(int);
extern void ehsys_BBB5D950(unsigned char *, unsigned char *);
extern void ehsys_BC8E65D7(int, int, int, void *, int);
extern void ehsys_BE3756D0(int);
extern unsigned int ehsys_BF4D5B0C(void);               /* text cursor x */
extern void ehsys_BFFBA174(void);
extern int ehsys_C62F9EAC(int, int);
extern void *ehsys_C6C15111(void *, char *, int);       /* find archive member */
extern void ehsys_C859D5FE(int);
extern void ehsys_C9D9E989(int);
extern int ehsys_CABEA834(int, int);                    /* set colour */
extern int ehsys_D2A768F4(int, char *);                 /* file size */
extern void ehsys_D470D0B2(int, int, int);              /* fade in */
extern void ehsys_D853F15C(int);
extern Line *ehsys_DCFC7586(int *, int);                /* display-list alloc */
extern int ehsys_DD69EDFB(void);
extern int ehsys_DFCA450B(int, int);                    /* open display list */
extern char *ehsys_E47E40E4(void);
extern void ehsys_E50B9594(char *, void *, ...);        /* format a message */
extern void *ehsys_E58C0FDC(void *, int);               /* string table lookup */
extern int ehsys_E6E4401F(int, int, int);
extern int ehsys_F462EE6E(int, int);
extern Pad *ehsys_F5E3080C(void);                       /* pad state */
extern int ehsys_FD84C4D3(int, int, int, int);
extern int ehsys_FDD168CA(int, int, int, int, int, int, int, int); /* draw sprite */
extern void ehsys_frame_sync(void);
extern unsigned short ehsys_get_button_code(int);       /* 0 = confirm, 1 = cancel */
extern int ehsys_get_language(void);
extern void ehsys_memcpy(void *, void *, int);
extern void ehsys_memset(void *, int, int);
extern void ehsys_qsort(void *, int, int, int (*)(unsigned short *, unsigned short *));
extern void ehsys_sceKernelChangeCurrentThreadAttr(int, int);
extern void ehsys_sprintf(char *, char *, ...);

/* ---- forward declarations: the call graph is not in address order ---- */
int func_00000150(int h);
int func_000002D0(int h);
void func_00000A10(int x, int y, int w, void *str);
void func_00000A24(int x, int y, int w, void *str);
unsigned char func_00000A34(int x, void *str);
void func_00000A3C(int x, int y, int w, int h, void *str);
void func_00000A58(int x, int y, int w, int h, void *str, int len, int flags);
int func_00000A6C(int h);
int func_00000B08(int h);
int func_00000B8C(int h);
void func_00000C5C(int y);
void func_00000D6C(int x, int y, void *str);
int func_00000D8C(int h, int x, int y, unsigned short card, unsigned short t, unsigned char alpha);
void func_000010C4(void);
int func_00001128(void);
void *func_000012A8(void);
void *func_00001358(unsigned short id);
int func_0000149C(void *buf, int size);
int func_0000152C(unsigned short *a, unsigned short *b);
int func_0000153C(unsigned short id, ResEntry *tbl, int n);
int func_00001864(void);
void func_00001934(void);
void func_00001994(void);
void func_00001A1C(void);
int func_00001CE4(void);
void func_00001D2C(int seed);
void func_00001D54(void);
void func_0000241C(void);
int func_000027A8(void);
int func_00002D68(void);
int func_000030D4(void);
int func_0000337C(unsigned short id);
void func_000033C0(unsigned short key, unsigned short state);
int func_000034D8(void);
void func_0000354C(unsigned short idx, unsigned short glyph);
unsigned char func_00003610(unsigned short col, unsigned short row);
void func_00003674(unsigned short key);

/* func_00000000 — exported draw callback (libpassword_rel NID 0x69B41C27):
 * clears the frame through one display list, then, once the module is past
 * loading (mode != 0), builds the scene in a second one: background, monitor
 * shadow, message window, keypad, nixie tubes. Every builder takes the
 * display-list handle and returns the advanced one; the last call takes its
 * address, which is why `h` lives on the stack.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000000(void) {
    int h;

    h = ehsys_DFCA450B(0, 0);
    h = ehsys_1D98951B(h, 0xFF000000);
    h = ehsys_42110BF9(h, 0);
    h = ehsys_399AC68A(h, 0);
    h = ehsys_C62F9EAC(h, 0x17);
    ehsys_1EC5342B(h);
    if (D_0001DC50.mode != 0) {
        h = ehsys_DFCA450B(4, 0);
        h = ehsys_F462EE6E(h, 9);
        h = ehsys_F462EE6E(h, 4);
        h = ehsys_09794DF0(h, 0, 1);
        h = ehsys_E6E4401F(h, 0, 0);
        h = ehsys_5CBB9E43(h, 0, 2, 3, 0, 0);
        h = ehsys_4175E3DB(h, 1);
        h = func_00000150(h);
        h = func_00000A6C(h);
        h = func_000002D0(h);
        h = func_00000B08(h);
        h = func_00000B8C(h);
        ehsys_878EE1D3(&h);
        ehsys_1EC5342B(h);
    }
}

/* func_00000150 — draw the background (resource 0x1001, bg.gim) as a
 * grid of sprites: a row of seven 64x256 strips, an eighth 32-wide one, then
 * four 64x64 tiles mirrored on both halves of the screen.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000150(int h) {
    void *res = func_00001358(0x1001);
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

/* func_000002D0 — draw the message window for the current mode: the
 * typed-out message(s), the fade of the card picture on success, the card
 * name being typed character by character, the button prompts, and a
 * blinking caret after the text (solid while PwState.flags.typing).
 * Modes 3..6 slide the window up by timer*0x97/30 while they open/close.
 * The two `case 0: case N:` switches are what the target's `beq N; beq 0; b`
 * tests are; see the file header.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000002D0(int h) {
    int caret = 1;
    int y = 0x53;
    int y2;
    unsigned char a, b;
    unsigned char color;
    unsigned char alpha;
    unsigned int cx;
    unsigned int cy;
    unsigned int size;
    unsigned short cw, ch;
    unsigned int base;

    h = ehsys_20560342(h, 0x19, 0x4E, 0x118, 0xA1);
    h = ehsys_F462EE6E(h, 2);
    ehsys_942B03D0(0xE, 0xE);
    ehsys_60B55A50(-1);
    ehsys_B8AD96EA(h);
    switch (D_0001DC50.mode) {
    case 1:
        func_00000A58(0x1E, y, 0x10E, 0x97, D_0001DC50.text1, D_0001DC50.text_pos, 0);
        break;
    case 2:
        func_00000A3C(0x1E, y, 0x10E, 0x97, D_0001DC50.text1);
        break;
    case 3:
        if (D_0001DC50.step == 0) {
            caret = 0;
            y -= (D_0001DC50.timer * 0x97) / 30;
        }
        func_00000A58(0x1E, y, 0x10E, 0x97, D_0001DC50.text1, D_0001DC50.text_pos, 0);
        break;
    case 4:
        if (D_0001DC50.step == 4) {
            ehsys_B8AD96EA(func_00000D8C(ehsys_A4AFF8E6(), 0x6F, y, D_0001DC50.card_id, D_0001DC50.timer, 0));
            caret = 0;
            break;
        } else if (D_0001DC50.step == 5) {
            if (D_0001DC50.timer < 60) {
                alpha = ((60 - D_0001DC50.timer) * 255) / 60;
                if (alpha > 0) {
                    ehsys_B8AD96EA(func_00000D8C(ehsys_A4AFF8E6(), 0x6F, 0x53, D_0001DC50.card_id, 60, alpha));
                }
                caret = 0;
                break;
            }
            ehsys_60B55A50(((unsigned char) (((D_0001DC50.timer - 60) * 255) / 60) << 24) | 0xFFFFFF);
        } else if (D_0001DC50.step == 3) {
            if (D_0001DC50.timer >= 30) {
                ehsys_60B55A50(((unsigned char) (((60 - D_0001DC50.timer) * 255) / 30) << 24) | 0xFFFFFF);
            }
        }
        switch (D_0001DC50.step) {
        case 0:
        case 8:
            caret = 0;
            y -= (D_0001DC50.timer * 0x97) / 30;
            break;
        }
        if (D_0001DC50.step < 4) {
            func_00000A58(0x1E, y, 0x10E, 0x97 - (y - 0x53), D_0001DC50.text1, D_0001DC50.text_pos, 0);
            y = (ehsys_B3B5D3CF() >> 6) + 0x10;
        }
        if (D_0001DC50.step == 2) {
            color = func_00000A34(0x106, D_0001DC50.name_text);
            ehsys_BBB5D950(&a, &b);
            ehsys_942B03D0(color, b);
            func_00000A24(0x22, y, 0x106, D_0001DC50.name_out);
            ehsys_942B03D0(a, b);
        } else if (D_0001DC50.step > 2) {
            func_00000A10(0x22, y, 0x106, D_0001DC50.name_out);
        }
        if (D_0001DC50.step >= 6) {
            y2 = (ehsys_B3B5D3CF() >> 6) + 0x10;
            func_00000A58(0x1E, y2, 0x10E, 0x97 - (y2 - 0x53), D_0001DC50.text2, D_0001DC50.text_pos2, 0);
        }
        if (D_0001DC50.step >= 7) {
            func_00000C5C((ehsys_B3B5D3CF() >> 6) + 4);
        }
        break;
    case 5:
        switch (D_0001DC50.step) {
        case 0:
        case 4:
            caret = 0;
            y -= (D_0001DC50.timer * 0x97) / 30;
            break;
        }
        func_00000A58(0x1E, y, 0x10E, 0x97, D_0001DC50.text1, D_0001DC50.text_pos, 0);
        if (D_0001DC50.step >= 2) {
            y2 = (ehsys_B3B5D3CF() >> 6) + 0xC;
            func_00000A58(0x1E, y2, 0x10E, 0x97 - (y2 - 0x53), D_0001DC50.text2, D_0001DC50.text_pos2, 0);
        }
        if (D_0001DC50.step >= 3) {
            func_00000C5C((ehsys_B3B5D3CF() >> 6) + 4);
        }
        break;
    case 6:
        switch (D_0001DC50.step) {
        case 0:
        case 3:
            caret = 0;
            y -= (D_0001DC50.timer * 0x97) / 30;
            break;
        }
        func_00000A58(0x1E, y, 0x10E, 0x97, D_0001DC50.text1, D_0001DC50.text_pos, 0);
        if (D_0001DC50.step >= 2) {
            func_00000C5C((ehsys_B3B5D3CF() >> 6) + 4);
        }
        break;
    }
    h = ehsys_A4AFF8E6();
    if (caret) {
        ehsys_942B03D0(0xE, 0xE);
        if (D_0001DC50.flags.typing || ((*ehsys_B2B9FEFA() >> 3) & 1)) {
            cx = ehsys_BF4D5B0C();
            base = ehsys_2FD4863E();
            base += ehsys_379BEA78();
            cy = (ehsys_B3B5D3CF() - base) >> 6;
            cx >>= 6;
            size = ehsys_826AE59C();
            cw = (unsigned short) size >> 6;
            ch = (unsigned short) (size >> 16) >> 6;
            if (cx + ch > 300) {
                cx = 0x1E;
                cy += ehsys_2FD4863E() >> 6;
            }
            h = ehsys_4175E3DB(h, 9);
            h = ehsys_09794DF0(h, 4, 0);
            h = ehsys_CABEA834(h, ehsys_DD69EDFB());
            h = ehsys_8EA6B1E7(h, 0, 0, ch - 2, cw);
            h = ehsys_FDD168CA(h, cx + 1, cy, 0, 0, 0, 0, 0);
            h = ehsys_F462EE6E(h, 9);
            h = ehsys_09794DF0(h, 0, 1);
            h = ehsys_CABEA834(h, -1);
        }
    }
    return ehsys_4175E3DB(h, 2);
}

/* func_00000A10 — text helper: coordinates in pixels -> 26.6 fixed point.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000A10(int x, int y, int w, void *str) {
    ehsys_BC8E65D7(x << 6, y << 6, w << 6, str, -1);
}

/* func_00000A24 — text helper (pixels -> 26.6 fixed point).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000A24(int x, int y, int w, void *str) {
    ehsys_B89D38DC(x << 6, y << 6, w << 6, str);
}

/* func_00000A34 — text helper (pixels -> 26.6 fixed point); returns
 * the colour index func_000002D0 uses for the card-name line.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
unsigned char func_00000A34(int x, void *str) {
    return ehsys_5F00A362(x << 6, str);
}

/* func_00000A3C — draw a whole message in a box (pixels -> 26.6).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000A3C(int x, int y, int w, int h, void *str) {
    ehsys_3BB2BAC6(x << 6, y << 6, w << 6, h << 6, str, -1, 0);
}

/* func_00000A58 — draw the first `len` characters of a message in a
 * box (pixels -> 26.6): the typewriter effect.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000A58(int x, int y, int w, int h, void *str, int len, int flags) {
    ehsys_3BB2BAC6(x << 6, y << 6, w << 6, h << 6, str, len, flags);
}

/* func_00000A6C — draw the monitor shadow (resource 0x1002).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000A6C(int h) {
    void *res = func_00001358(0x1002);

    if (res == 0) {
        return h;
    }
    h = ehsys_9EA6989A(h, res);
    h = ehsys_CABEA834(h, -1);
    h = ehsys_E6E4401F(h, 0, 0);
    h = ehsys_8EA6B1E7(h, 0x80, 0x100, 0x118, 0x100);
    return ehsys_FDD168CA(h, 0x19, 0, 0, 0, 0, 0, 0);
}

/* func_00000B08 — draw the keypad (resource 0x1010, key_<lang>.ehf).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000B08(int h) {
    SprParam p;
    void *res = func_00001358(0x1010);

    if (res == 0) {
        return h;
    }
    ehsys_memset(&p, 0, sizeof(p));
    p.f00 = *ehsys_B2B9FEFA();
    h = ehsys_85F89FE8(h);
    return ehsys_89812D8A(ehsys_7187E0F0(h, res, &p));
}

/* func_00000B8C — draw the eight nixie tubes (resources 0x1011..0x1018,
 * nixi.ehf), 22 px apart.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000B8C(int h) {
    SprParam p;
    void *res;
    int i;

    for (i = 0; i < 8; i++) {
        res = func_00001358(i + 0x1011);
        if (res != 0) {
            ehsys_memset(&p, 0, sizeof(p));
            p.f00 = *ehsys_B2B9FEFA();
            p.f0C = 0x10000;
            p.f14 = i * 22;
            h = ehsys_85F89FE8(h);
            h = ehsys_89812D8A(ehsys_7187E0F0(h, res, &p));
        }
    }
    return h;
}

/* func_00000C5C — draw the two button prompts (strings 5/6 left, 7/8
 * right of the wider of the first two) at line y.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000C5C(int y) {
    int x = 0x1E;
    void *s;
    unsigned int w0, w1;

    s = ehsys_E58C0FDC(func_00001358(0x1020), 5);
    w0 = ehsys_66F8E113(s);
    func_00000D6C(x, y, s);
    s = ehsys_E58C0FDC(func_00001358(0x1020), 6);
    w1 = ehsys_66F8E113(s);
    func_00000D6C(x, y + 0x10, s);
    if (w0 < w1) {
        w0 = w1;
    }
    x += (w0 >> 6) + 2;
    func_00000D6C(x, y, ehsys_E58C0FDC(func_00001358(0x1020), 7));
    func_00000D6C(x, y + 0x10, ehsys_E58C0FDC(func_00001358(0x1020), 8));
}

/* func_00000D6C — draw a string from x to the right edge (0x1E0).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000D6C(int x, int y, void *str) {
    ehsys_B89D38DC(x << 6, y << 6, (0x1E0 - x) << 6, str);
}

/* .rodata layout shim: 12 bytes that keep jtbl_000089F8/jtbl_00008A1C at
 * their shipped addresses (see the file header). Never read. */
const unsigned int rodata_shim_89EC[3] = {
    0x3D4CCCCD, /* 0.05f = D_000089EC */
    0x3DCCCCCD, /* 0.1f  = D_000089F0 */
    0x06020010, /* module_sdk_version */
};

/* func_00000D8C — draw the granted card's picture (PwState.card_id).
 * For t < 60 it is drawn as 154 one-pixel lines whose x is offset by
 * amp*sin(0.1*t + 0.05*line), amp shrinking from 24 to 0 and alpha growing,
 * i.e. the picture "tunes in" like a TV; from t = 60 it is one sprite
 * with the given alpha. `card` is passed by the caller but the function
 * reads PwState.card_id itself.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000D8C(int h, int x, int y, unsigned short card, unsigned short t, unsigned char alpha) {
    Line *lines;
    Line *l;
    float amp;
    float phase;
    float off;
    unsigned int color;
    int i;

    h = ehsys_4175E3DB(h, 2);
    ehsys_AB962AE7(0);
    h = ehsys_B4F2178B(h);
    h = ehsys_5DE4869F(h, 0x160000, -1, 0x80, 0x100, 3);
    h = ehsys_FD84C4D3(h, 0, 0, D_0001DC50.card_id);
    h = ehsys_B9064026(h);
    h = ehsys_B4F2178B(h);
    h = ehsys_09794DF0(h, 0, 0);
    h = ehsys_572A577B(h, 3, 0, 0, 0);
    h = ehsys_3DE5C8DC(h, 0, 0x80, 0x100, 0x80, ehsys_E47E40E4() + 0x160000);
    if (t >= 60) {
        if (alpha > 0) {
            h = ehsys_CABEA834(h, (alpha << 24) | 0xFFFFFF);
        } else {
            h = ehsys_CABEA834(h, -1);
        }
        h = ehsys_8EA6B1E7(h, 0x6C, 0x9A, 0x6C, 0x9A);
        h = ehsys_FDD168CA(h, x, y, 0, 0, 0, 0, 0);
    } else {
        lines = ehsys_DCFC7586(&h, 0x3000);
        l = lines;
        phase = D_000089F0 * t;
        color = ((((t * 255) / 60) & 0xFF) << 24) | 0xFFFFFF;
        amp = ((60 - t) * 24) / 60;
        ehsys_memset(lines, 0, 0x3000);
        for (i = 0; i < 0x9A; i++) {
            off = amp * ehsys_8637BCB2(phase + D_000089EC * i);
            l->a.u = 0.0f;
            l->b.u = 108.0f;
            l->a.v = 0.5f + i;
            l->b.v = l->a.v;
            l->a.color = color;
            l->b.color = color;
            l->a.x = x + off;
            l->b.x = 108.0f + l->a.x;
            l->a.y = y + i;
            l->b.y = l->a.y;
            l++;
        }
        h = ehsys_97DD82F2(h, 1, 0x80019F, 0x134, 0, lines);
    }
    h = ehsys_09794DF0(h, 0, 1);
    ehsys_AB962AE7(0x80);
    return h;
}

/* func_000010C4 — reset the resource loader and sort the resource
 * table by id (it is searched with func_0000153C).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000010C4(void) {
    ehsys_memset(&D_00008C40, 0, sizeof(D_00008C40));
    D_00008C40.ready = 1;
    ehsys_qsort(D_00008ADC, 0x11, sizeof(ResEntry), func_0000152C);
}

/* func_00001128 — mode 0 loader, one step per frame: open the work
 * buffer and request labo/password.ehp; wait for it; then hand the
 * card-drawing resources (attribute icons, frame, spell/trap icons,
 * ATK/DEF digits) to the engine. Returns 1 when done.
 * The `if (ready) { switch }` nesting (not an early return) is what gives
 * the target's shared `return 0`.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00001128(void) {
    struct {
        void *res[5];
        int pad[2];
        int handle;
        int pad2;
    } p;

    if (D_00008C40.ready) {
        switch (D_00008C40.step) {
        case 0:
            D_0001DC50.handle = ehsys_291D6262(D_00008C48, 0x15000, 0);
            func_000012A8();
            break;
        case 1:
            if (ehsys_2717C28C(D_0001DC50.handle)) {
                D_00008C40.step++;
            }
            break;
        case 2:
            func_00001934();
            ehsys_memset(&p, 0, sizeof(p));
            p.res[0] = func_00001358(0x1003);
            p.res[1] = func_00001358(0x1004);
            p.res[2] = func_00001358(0x1019);
            p.res[3] = func_00001358(0x1005);
            p.res[4] = func_00001358(0x1006);
            p.handle = D_0001DC50.handle;
            ehsys_1CC5878C(&p);
            D_00008C40.step++;
            break;
        default:
            D_00008C40.step = 0;
            return 1;
        }
    }
    return 0;
}

/* func_000012A8 — return labo/password.ehp, starting its asynchronous
 * load (completion: func_0000149C) on the first call.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void *func_000012A8(void) {
    int size;

    if (D_00008C44 != 0) {
        return D_00008C44;
    }
    if (D_00008C40.ready) {
        D_00008C40.ready = 0;
        size = ehsys_D2A768F4(3, D_00008BEC);
        D_00008C44 = ehsys_41AABF28(D_0001DC50.heap, size);
        ehsys_8288EEBC(3, D_00008BEC, D_00008C44, size, func_0000149C, D_00008C44);
    }
    return 0;
}

/* func_00001358 — get resource `id`: look it up in the table, load it
 * from the archive on first use (file name formatted with the language
 * letter from "jegfis" — Japanese, English, German, French, Italian,
 * Spanish). The eight nixie tubes (0x1011..0x1018) share one file, so each
 * gets a private heap copy and can animate independently.
 * `lang` is a struct copy rather than the original's initialised local
 * array; see the file header.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void *func_00001358(unsigned short id) {
    char path[0x100];
    LangCodes lang;
    ResEntry *e;
    void *arc;
    void *src;
    int size;
    int idx = func_0000153C(id, D_00008ADC, 0x11);

    if (idx < 0) {
        return 0;
    }
    e = &D_00008ADC[idx];
    if (e->data != 0) {
        return e->data;
    }
    arc = func_000012A8();
    lang = *(LangCodes *) D_00008C00;
    if (arc == 0) {
        return 0;
    }
    ehsys_sprintf(path, e->fmt, lang.c[ehsys_get_language()]);
    e->data = ehsys_C6C15111(arc, path, 0);
    if (id >= 0x1011 && id < 0x1019) {
        src = e->data;
        size = ehsys_316D8C28(arc, src);
        e->data = ehsys_41AABF28(D_0001DC50.heap, size);
        ehsys_memcpy(e->data, src, size);
    }
    return e->data;
}

/* func_0000149C — load-completion callback of func_000012A8.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_0000149C(void *buf, int size) {
    int ok = size > 0;

    if (ok) {
        if (buf != 0) {
            ehsys_4B0DABFA(0, buf);
        }
        D_00008C40.step++;
    }
    D_00008C40.ready = 1;
    return ok;
}

/* func_0000152C — qsort comparator on the u16 id at the head of a record.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_0000152C(unsigned short *a, unsigned short *b) {
    return *a - *b;
}

/* func_0000153C — binary search of the resource table by id; -1 if absent.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_0000153C(unsigned short id, ResEntry *tbl, int n) {
    int lo, hi, mid;
    unsigned short v;

    hi = n - 1;
    lo = 0;
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        v = tbl[mid].id;
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

/* func_0000159C — returns 0 (unreferenced in this module).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_0000159C(void) {
    return 0;
}

/* func_000015A4 — exported init (NID 0x932AE9C8): clear the state,
 * create the 3 MB heap, wait for the engine, pick the button layout by
 * language.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000015A4(void) {
    ehsys_memset(&D_0001DC50, 0, sizeof(D_0001DC50));
    D_0001DC50.heap = ehsys_1856E536(D_0001EDA0, 0x300000);
    ehsys_sceKernelChangeCurrentThreadAttr(0, 0x4000);
    func_000010C4();
    ehsys_6E42DDCD();
    ehsys_C859D5FE(8);
    while (!ehsys_AA9913C7(8)) {
        ehsys_frame_sync();
    }
    ehsys_C9D9E989(1);
    ehsys_4AA58320(1);
    if (ehsys_get_language() == 0) {
        ehsys_33ED567B(1);
        ehsys_D853F15C(0);
    } else {
        ehsys_33ED567B(0);
        ehsys_D853F15C(1);
    }
    ehsys_AB962AE7(0x80);
}

/* func_00001688 — exported per-frame update (NID 0x516C75F9); returns 1
 * when the module should exit. Modes: 0 load, 1 intro text, 2 input,
 * 3 check, 4 success, 5 wrong password, 6 quit prompt, 7 fade out, 8 unload.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00001688(void) {
    switch (D_0001DC50.mode) {
    case 0:
        if (func_00001128()) {
            D_0001DC50.mode = 1;
            ehsys_D470D0B2(1, 0xFF000000, 0x10);
            ehsys_BE3756D0(8);
            ehsys_2AE3C0AD(0x9518);
            ehsys_13A4081A(0);
        }
        break;
    case 1:
        func_00001A1C();
        break;
    case 2:
        func_00001D54();
        break;
    case 3:
        func_0000241C();
        break;
    case 4:
        if (func_000027A8()) {
            D_0001DC50.step = 0;
            D_0001DC50.timer = 0;
            D_0001DC50.mode = 7;
            ehsys_5998BFD7(1, 0xFF000000, 0x10);
            ehsys_6367AE39(600);
        }
        break;
    case 5:
        if (func_00002D68()) {
            D_0001DC50.step = 0;
            D_0001DC50.timer = 0;
            D_0001DC50.mode = 7;
            ehsys_5998BFD7(1, 0xFF000000, 0x10);
            ehsys_6367AE39(600);
        }
        break;
    case 6:
        if (func_000030D4()) {
            D_0001DC50.step = 0;
            D_0001DC50.timer = 0;
            D_0001DC50.mode = 7;
            ehsys_5998BFD7(1, 0xFF000000, 0x10);
            ehsys_6367AE39(600);
        }
        break;
    case 7:
        if (ehsys_450CD6C5()) {
            D_0001DC50.mode = 8;
        }
        break;
    case 8:
        if (func_00001864()) {
            return 1;
        }
        break;
    }
    return 0;
}

/* func_00001864 — mode 8: unload, then return 1.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00001864(void) {
    switch (D_0001DC50.step) {
    case 0:
        if (ehsys_04E1CB0F() == 0) {
            ehsys_6E42DDCD();
            D_0001DC50.step++;
        }
        break;
    case 1:
        ehsys_AB962AE7(0);
        ehsys_BFFBA174();
        if (D_0001DC50.handle != 0) {
            ehsys_5DBE93DB(D_0001DC50.handle);
            D_0001DC50.step++;
        }
        break;
    case 2:
        if (ehsys_101103B3(D_0001DC50.handle)) {
            return 1;
        }
        break;
    }
    return 0;
}

/* func_00001934 — clear the entered digits and put the cursor on "0".
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001934(void) {
    D_0001DC50.count.n = 0;
    D_0001DC50.cursor.state = 3;
    func_00003674(0);
    func_00001994();
}

/* func_00001994 — redraw the tubes: entered digits, the blinking cursor
 * glyph (12) after them, blank (10) beyond.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001994(void) {
    int i;
    unsigned short n;

    for (i = 0; i < 8; i++) {
        n = D_0001DC50.count.n;
        if (i < n) {
            func_0000354C(i, D_0001DC50.digits[i]);
        } else if (i == n) {
            func_0000354C(n, 0xC);
        } else {
            func_0000354C(i, 0xA);
        }
    }
}

/* func_00001A1C — mode 1: type out the intro message (string 1) while
 * random keys flash; any of buttons 0x6000 skips it. Then switch to mode 2.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001A1C(void) {
    switch (D_0001DC50.step) {
    case 0:
        ehsys_025FE238(D_0001DC50.text1, ehsys_E58C0FDC(func_00001358(0x1020), 1));
        D_0001DC50.text_len = ehsys_28BE130B(D_0001DC50.text1);
        D_0001DC50.text_pos = 0;
        D_0001DC50.flags.typing = 1;
        D_0001DC50.timer = 0;
        D_0001DC50.step = 1;
        func_00001D2C(3);
        break;
    case 1:
        if (ehsys_F5E3080C()->press & 0x6000) {
            D_0001DC50.text_pos = D_0001DC50.text_len;
            func_00001994();
            D_0001DC50.cursor.state = 0;
            func_000033C0(func_00003610(D_0001DC50.cursor.col, D_0001DC50.cursor.row), 0);
            D_0001DC50.mode = 2;
            ehsys_65DA0419(0x9518);
            D_0001DC50.timer = 0;
            D_0001DC50.flags.intro_done = 1;
            D_0001DC50.flags.typing = 0;
        } else {
            if (!D_0001DC50.flags.intro_done) {
                func_000033C0(func_00001CE4() % 12, 2);
            }
            if (++D_0001DC50.timer > 0) {
                if (++D_0001DC50.text_pos >= D_0001DC50.text_len) {
                    func_00001994();
                    D_0001DC50.cursor.state = 0;
                    func_000033C0(func_00003610(D_0001DC50.cursor.col, D_0001DC50.cursor.row), 0);
                    D_0001DC50.mode = 2;
                    ehsys_65DA0419(0x9518);
                    D_0001DC50.flags.intro_done = 1;
                    D_0001DC50.flags.typing = 0;
                }
                D_0001DC50.timer = 0;
            }
        }
        break;
    }
}

/* func_00001CE4 — rand(): 15-bit LCG (x * 0x41C64E6D + 12345), on an
 * engine-owned seed table reached at absolute address 0 (see header).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00001CE4(void) {
    RandSlot *r = &D_0000[D_00000000];

    r->seed *= 0x41C64E6D;
    r->seed += 0x3039;
    return (r->seed >> 16) & 0x7FFF;
}

/* func_00001D2C — srand().
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001D2C(int seed) {
    D_0000[D_00000000].seed = seed;
}

/* func_00001D54 — mode 2: keypad input. Confirm on a key: digit ->
 * append (at 8 digits the cursor jumps to OK), 10 -> clear all, 11 ->
 * delete last, 12 (OK) -> mode 3. Cancel -> mode 6 (quit prompt). Button
 * 0x8000 deletes the last digit. The d-pad moves the cursor, wrapping; with
 * 8 digits entered only the OK row and the clear/0/back row are reachable.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001D54(void) {
    Pad *pad = ehsys_F5E3080C();
    Cursor *c = &D_0001DC50.cursor;
    unsigned char key = func_00003610(c->col, c->row);
    Cursor old;
    int press;
    int ok;
    int i;
    unsigned char col;

    ehsys_memcpy(&old, c, sizeof(old));
    if (c->state == 1 && func_000034D8()) {
        c->state = 0;
        func_000033C0(key, 0);
    }
    press = pad->press;
    if (press & ehsys_get_button_code(0)) {
        ok = 1;
        switch (key) {
        case 12:
            for (i = 0; i < 8; i++) {
                func_0000354C(i, 0xC);
            }
            D_0001DC50.step = 0;
            D_0001DC50.timer = 0;
            D_0001DC50.mode = 3;
            ehsys_2AE3C0AD(0x951C);
            break;
        case 11:
            if (D_0001DC50.count.n == 0) {
                ok = 0;
            } else {
                func_0000354C(D_0001DC50.count.n, 0xA);
                D_0001DC50.count.n--;
                func_0000354C(D_0001DC50.count.n, 0xC);
                ehsys_2AE3C0AD(0x951B);
            }
            break;
        case 10:
            if (D_0001DC50.count.n == 0) {
                ok = 0;
            } else {
                D_0001DC50.count.n = 0;
                func_0000354C(0, 0xC);
                for (i = 1; i < 8; i++) {
                    func_0000354C(i, 0xA);
                }
                ehsys_2AE3C0AD(0x951A);
            }
            break;
        default:
            func_0000354C(D_0001DC50.count.n, key);
            D_0001DC50.digits[D_0001DC50.count.n++] = key;
            func_0000354C(D_0001DC50.count.n, 0xC);
            ehsys_2AE3C0AD(0x9519);
            if (D_0001DC50.count.n >= 8) {
                c->row = 0;
                c->col &= 0xFE;
            }
            break;
        }
        if (ok) {
            c->state = 1;
            func_000033C0(key, 1);
            return;
        }
    } else if (press & ehsys_get_button_code(1)) {
        D_0001DC50.step = 0;
        D_0001DC50.timer = 0;
        D_0001DC50.mode = 6;
        func_000033C0(key, 3);
    } else if (press & 0x8000) {
        if (D_0001DC50.count.n > 0) {
            c->state = 1;
            func_000033C0(0xB, 1);
            if (D_0001DC50.count.n >= 8 && key == 12) {
                func_00003674(0);
            }
            func_0000354C(D_0001DC50.count.n, 0xA);
            D_0001DC50.count.n--;
            func_0000354C(D_0001DC50.count.n, 0xC);
            return;
        }
    } else if (pad->repeat & 0x10) {
        if (D_0001DC50.count.n >= 8) {
            c->row = (c->row + 1) & 1;
        } else if (c->row == 4) {
            c->row = 1;
        } else {
            c->row++;
        }
    } else if (pad->repeat & 0x40) {
        if (D_0001DC50.count.n >= 8) {
            c->row = (c->row + 1) & 1;
        } else if (c->row == 1) {
            c->row = 4;
        } else {
            c->row--;
        }
    } else if (pad->repeat & 0x80) {
        if (D_0001DC50.count.n >= 8) {
            if (c->row == 1) {
                c->col = (c->col + 2) & 3;
            }
        } else {
            c->col = (c->col + 2) % 3;
        }
    } else if (pad->repeat & 0x20) {
        if (D_0001DC50.count.n >= 8) {
            if (c->row == 1) {
                c->col = (c->col + 2) & 3;
            }
        } else {
            c->col = (c->col + 1) % 3;
        }
    }
    col = c->col;
    if (old.col != col || old.row != c->row) {
        key = func_00003610(col, c->row);
        c->state = 0;
        func_000033C0(key, c->state);
    }
}

/* func_0000241C — mode 3: the password check, i.e. password -> card.
 * Steps 0/1 type the "checking" message (string 0xC); steps 2..9 light the
 * tubes one digit every 10 frames; step 10 decides:
 *   value = d0 d1 ... d7 read as a decimal number;
 *   card  = ehsys_ACAC5C40(value)   (engine lookup; its only caller in the
 *                                    whole game);
 *   accepted iff 0xE10 <= card < 0x23FF, ehsys_28F7224B(card) holds, and
 *   card is not one of the 18 ids at D_00008C08 (func_0000337C);
 *   accepted -> card_id = card, ehsys_8C5B148E(handle, card,
 *               ehsys_B401BB25(card)) gives it, mode 4; otherwise mode 5.
 * The typed value is kept in PwState.password for the messages.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_0000241C(void) {
    Cursor *c = &D_0001DC50.cursor;
    int value;
    int i;
    int card;

    if (c->state == 1 && func_000034D8()) {
        c->state = 0;
        func_000033C0(0, 3);
    }
    switch (D_0001DC50.step) {
    case 0:
        if (++D_0001DC50.timer >= 30) {
            ehsys_025FE238(D_0001DC50.text1, ehsys_E58C0FDC(func_00001358(0x1020), 0xC));
            D_0001DC50.text_len = ehsys_28BE130B(D_0001DC50.text1);
            D_0001DC50.text_pos = 0;
            D_0001DC50.timer = 0;
            D_0001DC50.step = 1;
            D_0001DC50.flags.typing = 1;
            ehsys_2AE3C0AD(0x951D);
        }
        break;
    case 1:
        if (ehsys_F5E3080C()->press & 0x6000) {
            D_0001DC50.text_pos = D_0001DC50.text_len;
            D_0001DC50.step++;
            D_0001DC50.timer = 0;
            D_0001DC50.flags.typing = 0;
        } else if (++D_0001DC50.timer > 0) {
            if (++D_0001DC50.text_pos >= D_0001DC50.text_len) {
                D_0001DC50.step++;
                D_0001DC50.flags.typing = 0;
            }
            D_0001DC50.timer = 0;
        }
        break;
    case 2 ... 9:
        if (++D_0001DC50.timer >= 10) {
            func_0000354C(D_0001DC50.step - 2, D_0001DC50.digits[D_0001DC50.step - 2]);
            D_0001DC50.step++;
            D_0001DC50.timer = 0;
        }
        break;
    case 10:
        ehsys_65DA0419(0x951D);
        value = 0;
        for (i = 0; i < 8; i++) {
            value *= 10;
            value += D_0001DC50.digits[i];
        }
        card = ehsys_ACAC5C40(value);
        if (card >= 0xE10 && card < 0x23FF && ehsys_28F7224B(card) && !func_0000337C(card)) {
            D_0001DC50.card_id = card;
            ehsys_8C5B148E(D_0001DC50.handle, card, ehsys_B401BB25(card));
            D_0001DC50.mode = 4;
        } else {
            D_0001DC50.mode = 5;
        }
        D_0001DC50.password = value;
        D_0001DC50.step = 0;
        D_0001DC50.timer = 0;
        break;
    }
}

/* func_000027A8 — mode 4 (success): message 3 with the password, then
 * the card name typed out ('$' starts a 3-unit colour escape that is copied
 * in one go), the picture fades in (func_000002D0/func_00000D8C), message
 * 4, then confirm -> enter another password, cancel -> exit.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000027A8(void) {
    char buf[0x800];
    char name[0x800];
    unsigned short i;
    unsigned short ch;

    switch (D_0001DC50.step) {
    case 0:
        if (++D_0001DC50.timer >= 30) {
            ehsys_E50B9594(buf, ehsys_E58C0FDC(func_00001358(0x1020), 3), D_0001DC50.password);
            ehsys_025FE238(D_0001DC50.text1, buf);
            D_0001DC50.text_len = ehsys_28BE130B(D_0001DC50.text1);
            D_0001DC50.text_pos = 0;
            D_0001DC50.timer = 0;
            D_0001DC50.step = 1;
            D_0001DC50.flags.typing = 1;
        }
        break;
    case 1:
        if (++D_0001DC50.timer > 0) {
            if (++D_0001DC50.text_pos >= D_0001DC50.text_len) {
                D_0001DC50.step = 2;
                ehsys_7B8EF3EC(name, ehsys_AD0E1E03(D_0001DC50.card_id));
                ehsys_E50B9594((char *) D_0001DC50.name_text, ehsys_E58C0FDC(func_00001358(0x1020), 2), name);
                ehsys_memset(D_0001DC50.name_out, 0, sizeof(D_0001DC50.name_out));
                D_0001DC50.name_pos = 0;
                ehsys_2AE3C0AD(0x951E);
            }
            D_0001DC50.timer = 0;
        }
        break;
    case 2:
        if (++D_0001DC50.timer > 0) {
            i = D_0001DC50.name_pos;
            ch = D_0001DC50.name_text[i];
            if (ch != 0) {
                D_0001DC50.name_out[i] = ch;
                D_0001DC50.name_pos++;
                if (ch == 0x24) {
                    ehsys_memcpy(&D_0001DC50.name_out[i + 1], &D_0001DC50.name_text[i + 1], 6);
                    D_0001DC50.name_pos += 3;
                }
            } else {
                D_0001DC50.step = 3;
                D_0001DC50.flags.typing = 0;
            }
            D_0001DC50.timer = 0;
        }
        break;
    case 3:
        if (++D_0001DC50.timer >= 60) {
            D_0001DC50.step = 4;
            D_0001DC50.timer = 0;
            ehsys_2AE3C0AD(0x9520);
        }
        break;
    case 4:
        D_0001DC50.timer++;
        if (D_0001DC50.timer > 60) {
            if ((ehsys_F5E3080C()->press & 0x6000) || D_0001DC50.timer > 600) {
                ehsys_AEA7AA02(D_0001DC50.card_id);
                D_0001DC50.step = 5;
                D_0001DC50.timer = 0;
            }
        }
        break;
    case 5:
        if (++D_0001DC50.timer >= 120) {
            ehsys_025FE238(D_0001DC50.text2, ehsys_E58C0FDC(func_00001358(0x1020), 4));
            D_0001DC50.text_len = ehsys_28BE130B(D_0001DC50.text2);
            D_0001DC50.text_pos2 = 0;
            D_0001DC50.step = 6;
            D_0001DC50.timer = 0;
            D_0001DC50.flags.typing = 1;
        }
        break;
    case 6:
        if (ehsys_F5E3080C()->press & 0x6000) {
            D_0001DC50.text_pos2 = D_0001DC50.text_len;
            D_0001DC50.step = 7;
            D_0001DC50.flags.typing = 0;
            D_0001DC50.timer = 0;
        } else if (++D_0001DC50.timer > 0) {
            if (++D_0001DC50.text_pos2 >= D_0001DC50.text_len) {
                D_0001DC50.step = 7;
                D_0001DC50.flags.typing = 0;
            }
            D_0001DC50.timer = 0;
        }
        break;
    case 7:
        if (ehsys_F5E3080C()->press & ehsys_get_button_code(1)) {
            return 1;
        }
        if (ehsys_F5E3080C()->press & ehsys_get_button_code(0)) {
            D_0001DC50.timer = 0;
            D_0001DC50.step = 8;
        }
        break;
    case 8:
        if (++D_0001DC50.timer >= 30) {
            D_0001DC50.text_pos = 0;
            D_0001DC50.timer = 0;
            D_0001DC50.step = 0;
            D_0001DC50.mode = 1;
            func_00001934();
            ehsys_memset(D_0001DC50.text1, 0, sizeof(D_0001DC50.text1));
        }
        break;
    }
    return 0;
}

/* func_00002D68 — mode 5 (no card for this password): messages 0xA (with
 * the password) and 0xB, then cancel -> exit, confirm -> try again with the
 * digits cleared.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00002D68(void) {
    char buf[0x800];

    switch (D_0001DC50.step) {
    case 0:
        if (++D_0001DC50.timer >= 30) {
            ehsys_E50B9594(buf, ehsys_E58C0FDC(func_00001358(0x1020), 0xA), D_0001DC50.password);
            ehsys_025FE238(D_0001DC50.text1, buf);
            D_0001DC50.text_len = ehsys_28BE130B(D_0001DC50.text1);
            D_0001DC50.text_pos = 0;
            D_0001DC50.timer = 0;
            D_0001DC50.step = 1;
            D_0001DC50.flags.typing = 1;
        }
        break;
    case 1:
        if (++D_0001DC50.timer > 0) {
            if (++D_0001DC50.text_pos >= D_0001DC50.text_len) {
                D_0001DC50.step = 2;
                ehsys_025FE238(D_0001DC50.text2, ehsys_E58C0FDC(func_00001358(0x1020), 0xB));
                D_0001DC50.text_len = ehsys_28BE130B(D_0001DC50.text2);
                D_0001DC50.text_pos2 = 0;
                ehsys_2AE3C0AD(0x951F);
            }
            D_0001DC50.timer = 0;
        }
        break;
    case 2:
        if (ehsys_F5E3080C()->press & 0x6000) {
            D_0001DC50.text_pos2 = D_0001DC50.text_len;
            D_0001DC50.step = 3;
            D_0001DC50.timer = 0;
            D_0001DC50.flags.typing = 0;
        }
        if (++D_0001DC50.timer > 0) {
            if (++D_0001DC50.text_pos2 >= D_0001DC50.text_len) {
                D_0001DC50.step = 3;
                D_0001DC50.flags.typing = 0;
            }
            D_0001DC50.timer = 0;
        }
        break;
    case 3:
        if (ehsys_F5E3080C()->press & ehsys_get_button_code(1)) {
            return 1;
        }
        if (ehsys_F5E3080C()->press & ehsys_get_button_code(0)) {
            D_0001DC50.timer = 0;
            D_0001DC50.step = 4;
        }
        break;
    case 4:
        if (++D_0001DC50.timer >= 30) {
            D_0001DC50.text_pos = 0;
            D_0001DC50.timer = 0;
            D_0001DC50.step = 0;
            D_0001DC50.mode = 1;
            func_00001934();
            ehsys_memset(D_0001DC50.text1, 0, sizeof(D_0001DC50.text1));
        }
        break;
    }
    return 0;
}

/* func_000030D4 — mode 6 (quit prompt): message 9, then confirm -> exit,
 * cancel -> back to the keypad with the digits kept.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000030D4(void) {
    switch (D_0001DC50.step) {
    case 0:
        if (++D_0001DC50.timer >= 30) {
            ehsys_025FE238(D_0001DC50.text1, ehsys_E58C0FDC(func_00001358(0x1020), 9));
            D_0001DC50.text_len = ehsys_28BE130B(D_0001DC50.text1);
            D_0001DC50.text_pos = 0;
            D_0001DC50.timer = 0;
            D_0001DC50.step = 1;
            D_0001DC50.flags.typing = 1;
        }
        break;
    case 1:
        if (ehsys_F5E3080C()->press & 0x6000) {
            D_0001DC50.text_pos = D_0001DC50.text_len;
            D_0001DC50.step = 2;
            D_0001DC50.timer = 0;
            D_0001DC50.flags.typing = 0;
        } else if (++D_0001DC50.timer > 0) {
            if (++D_0001DC50.text_pos >= D_0001DC50.text_len) {
                D_0001DC50.step = 2;
                D_0001DC50.flags.typing = 0;
            }
            D_0001DC50.timer = 0;
        }
        break;
    case 2:
        if (ehsys_F5E3080C()->press & ehsys_get_button_code(0)) {
            return 1;
        }
        if (ehsys_F5E3080C()->press & ehsys_get_button_code(1)) {
            D_0001DC50.timer = 0;
            D_0001DC50.step = 3;
        }
        break;
    case 3:
        if (++D_0001DC50.timer >= 30) {
            D_0001DC50.text_pos = 0;
            D_0001DC50.timer = 0;
            D_0001DC50.step = 0;
            D_0001DC50.mode = 1;
            func_00001994();
            ehsys_memset(D_0001DC50.text1, 0, sizeof(D_0001DC50.text1));
        }
        break;
    }
    return 0;
}

/* func_0000337C — 1 if `id` is one of the 18 cards at D_00008C08 that a
 * password must never give.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_0000337C(unsigned short id) {
    int i;

    for (i = 0; i < 0x12; i++) {
        if (id == D_00008C08[i]) {
            return 1;
        }
    }
    return 0;
}

/* func_000033C0 — play the keypad animation of `key`: 30 frames per key,
 * +0x5A for keys 10/11 once all 8 digits are in; state 0 = cursor on the
 * key (frames 1..15), 1 = key pressed (16..29), 2/3 = a single frame.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000033C0(unsigned short key, unsigned short state) {
    void *res = func_00001358(0x1010);
    unsigned short base, first, last;

    if (res == 0) {
        return;
    }
    base = key * 30;
    if (D_0001DC50.count.n >= 8 && (key == 10 || key == 11)) {
        base += 0x5A;
    }
    switch (state) {
    case 0:
        first = base + 1;
        last = base + 0xF;
        break;
    case 1:
        first = base + 0x10;
        last = base + 0x1D;
        break;
    case 2:
        last = base + 1;
        first = last;
        break;
    case 3:
        last = base;
        first = last;
        break;
    }
    ehsys_831C5769(res, first, last + 1);
}

/* func_000034D8 — 1 when the keypad animation has reached its last frame.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000034D8(void) {
    void *res = func_00001358(0x1010);
    int cur, first, last;

    if (res != 0) {
        cur = ehsys_558B2DCB(res);
        ehsys_B8770B92(res, &first, &last);
        if (cur < last - 1) {
            return 0;
        }
    }
    return 1;
}

/* func_0000354C — set nixie tube `idx` to `glyph` (0..9 digits, 10 blank,
 * 12 blinking cursor, 13 a 34-frame effect). The digit range is a GNU case
 * range: that is what produces the target's `bltz`/`slti 10` pair instead
 * of a jump table.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_0000354C(unsigned short idx, unsigned short glyph) {
    void *res;
    unsigned short first, last;

    if (idx >= 8) {
        return;
    }
    res = func_00001358(idx + 0x1011);
    if (res == 0) {
        return;
    }
    switch (glyph) {
    case 0 ... 9:
    case 10:
        last = glyph;
        first = last;
        break;
    case 12:
        first = 12;
        last = 0xF;
        break;
    case 13:
        first = 0x10;
        last = 0x32;
        break;
    default:
        return;
    }
    ehsys_831C5769(res, first, last + 1);
}

/* func_00003610 — key under the cursor: row 0 -> 12 (OK), row 1 -> 10 / 0 /
 * 11, rows 2..4 -> 1..9.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
unsigned char func_00003610(unsigned short col, unsigned short row) {
    switch (row) {
    case 0:
        return 12;
    case 1:
        switch (col) {
        case 0:
            return 10;
        case 1:
            return 0;
        case 2:
            return 11;
        default:
            return 0xFF;
        }
    default:
        return col + (row - 2) * 3 + 1;
    }
}

/* func_00003674 — move the cursor onto `key` (inverse of func_00003610)
 * and replay its animation. Case order 12, 11, 0, 10 is required: MWCC
 * tests compare-chain cases in reverse source order.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003674(unsigned short key) {
    Cursor *c = &D_0001DC50.cursor;

    switch (key) {
    case 12:
        c->row = 0;
        break;
    case 11:
        c->col = 2;
        c->row = 1;
        break;
    case 0:
        c->col = 1;
        c->row = 1;
        break;
    case 10:
        c->col = 0;
        c->row = 1;
        break;
    default:
        c->col = (key - 1) % 3;
        c->row = (key - 1) / 3 + 2;
        break;
    }
    func_000033C0(key, c->state);
}

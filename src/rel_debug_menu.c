/*
 * rel_debug_menu.prx - reconstructed code (matching decompilation)
 *
 * The developer test menu ("YUGI OH 5Ds -TAGFORCE-  TEST MODE"): a list of
 * test entry points into the other gmodules, a DEBUG OPTION page (language,
 * clear all data, get all card, save/load data, demonstration duel,
 * delivery test, Sale Zone), a SOUND TEST page, an ad-hoc "matching test"
 * (create/join a group, set rules, exchange decks, start a duel) and a
 * card "delivery test".
 *
 * Compiler:     MWCC 1.3 SP7 (mwccpsp_3.0.1_219), -O4,s -sdatathreshold 0
 * Verification: scripts/mwcc_build.sh + scripts/mwcc_diff.py, and
 *               `make MODULE=rel_debug_menu SRC=1` -> OK (sha1 a989a168...):
 *               ALL 44 functions are byte-identical and the module relinks
 *               to its shipped checksum, its six jump tables included.
 *
 * Source structure. The PRX was linked from (at least) three C files:
 * the same static text helper is compiled three times (func_00000BE4,
 * func_0000289C, func_00003B64). Roughly: 0x0000-0x206C menu/options/sound
 * test ("debug_menu.c", named by the __FILE__ passed at func_000002DC),
 * 0x206C-0x28BC delivery test, 0x28BC-0x4018 matching test.
 *
 * The built-in MenuEntry table (D_000098B0, 0x14 bytes per entry:
 * Japanese name, English name, handler, arg0, arg1). Japanese names are
 * Shift-JIS in .data; translations only here (identifiers stay ASCII):
 *   row  scene  English label        Japanese label (translated)
 *    0     2    story test           "story test"
 *    1     3    effect test          "duel effect drawing test"
 *    2     4    deck construction    "deck editing"
 *    3   4,4    deck const(Partner)  "deck editing (partner)"
 *    4     5    map test             "field test"
 *    5     6    duel test            "duel manager test"
 *    6     7    shop test            "purchase test"
 *    7     8    database test        "laboratory facilities"
 *    8     9    movie test           "movie viewer"
 *    9    10    model test           "cut-in viewer"
 *   10    11    draw test            "graphics drawing test"
 *   11    12    title test           "to the title menu..."
 *   12    13    html viewer          "browser"
 *   13     -    matching test        "matching test" (no handler: state 0x63)
 *   14    14    umd replace test     "UMD swap test"
 *   15    15    gallery              "gallery"
 * Every handler is func_00001D84, i.e. ehsys_551A8EF2(1, scene, arg): the
 * scene numbers line up with the gmodule list, which is the evidence for
 * reading ehsys_551A8EF2 as "request scene change".
 *
 * Conventions (see src/rel_movie_viewer.c and docs/06):
 *  - globals keep their D_XXXXXXXX placeholder names because the Makefile
 *    and mwcc_diff.py derive their addresses from the name. Proposed names
 *    are in config/symbols/rel_debug_menu.extra.txt and in the comments
 *    below; switch to them once config/symbols/rel_debug_menu.txt is
 *    regenerated.
 *  - no string literals: this object must not emit .data (the shipped
 *    .data is linked verbatim), so every string is an extern at its real
 *    address, with its content (or, for Shift-JIS, its bytes as \x escapes
 *    and an English translation) in a comment.
 *  - globals are NOT volatile here, unlike rel_movie_viewer. The few places
 *    where the target re-reads a global that a plain access would cache
 *    use volatile casts (the V() macro) at exactly those accesses.
 *
 * MWCC levers confirmed in this module (each was the last blocker somewhere):
 *  - `switch` tests come out in REVERSE source order, bodies in source
 *    order (func_00000034, func_0000362C, func_00002D98 ...).
 *  - `case 0: break;` vs `case 0: return 0;` changes whether the default
 *    path gets its own `b` + shared tail (func_00001C74).
 *  - a block with more than one predecessor is never stolen into a
 *    branch-likely delay slot; a single-predecessor block is. Unfilled
 *    `beqz`+`nop` pairs therefore mean "this block is a join" - choose
 *    if/else/return shapes accordingly (func_00002A88, func_00003A28).
 *  - saved registers are assigned in declaration order, last local -> $s0
 *    (func_000002DC); stack slots likewise follow declaration order.
 *  - a base address kept in a saved register needs a local pointer or a
 *    struct pointer; otherwise MWCC folds offsets into a fresh %hi/%lo
 *    (func_000028EC, func_00003A28, func_00002D98).
 *  - struct assignment of a nested char-array struct compiles to a 2-byte
 *    unrolled copy loop; separate byte members copy one by one (func_000035C0).
 *  - `(x >> 3) & 1` compiles to the Allegrex `ext` the target uses.
 */

#define V(x) (*(volatile int *)&(x))

/* sceCtrl button bits (pspctrl.h values) */
#define PAD_START    0x0008
#define PAD_UP       0x0010
#define PAD_RIGHT    0x0020
#define PAD_DOWN     0x0040
#define PAD_LEFT     0x0080
#define PAD_LTRIGGER 0x0100
#define PAD_RTRIGGER 0x0200
#define PAD_TRIANGLE 0x1000
#define PAD_CIRCLE   0x2000
#define PAD_CROSS    0x4000

typedef struct PadState {
    char unk00[0x18];
    int held;   /* +0x18 buttons currently down */
    int trg;    /* +0x1C buttons newly pressed this frame */
    int trg2;   /* +0x20 second edge mask, tested against the confirm/cancel codes */
    int rep;    /* +0x24 pressed-with-auto-repeat (cursor movement) */
} PadState;

typedef struct Deck {
    unsigned int n_main;
    unsigned int n_side;
    unsigned int n_extra;
    unsigned short main[60];
    unsigned short side[15];
    unsigned short extra[15];
} Deck;

typedef struct DuelRule {
    unsigned char b[6];
    char name[32];
} DuelRule;

typedef struct Name32 {
    signed char s[32];
} Name32;

typedef struct S28 {
    unsigned char b0, b1, b2, b3, b4, b5;
    Name32 name;
    unsigned short v;
} S28;

typedef struct MenuEntry {
    char *name_jp;
    char *name_en;
    int (*func)(int, int);
    int arg0;
    int arg1;
} MenuEntry;

/* ---- .data: strings and tables (linked verbatim from the shipped module) ---- */
extern char D_000091E0[][8];  /* language names, 8 bytes each: "JPN","ENG","GER","FRA","ITA","SPA" */
extern char D_00009220;  /* region names, 8 bytes each, "JAPAN","U.S.A." then "Europe" x6; [0x9220] = "Europe" */
extern char D_00009250[][32];  /* DEBUG OPTION item names, 32 bytes each: language, clear all data, get all card, save data, load data, demonstration, delivery test, Sale Zone */
extern char D_00009350[][32];  /* SOUND TEST labels, 32 bytes each: "BGM: ","SE PACK: ","SE: ","NPC: ","NPC-OFS: ","DUEL VOICE: ","TITLE VOICE: ","VOLUME BGM: ","VOLUME SE: ","VOLUME VOICE: " */
extern char D_00009490[][32];  /* sound-pack names, 32 bytes each: SEPAC_STATIC, SEPAC_DUEL, SEPAC_VS, SEPAC_STORY */
extern char D_000098B0;  /* default MenuEntry table, 16 entries + terminator (see the table in the header) */
extern char D_00009A04;  /* "debug_menu.c" */
extern char D_00009A14;  /* "YUGI OH 5Ds -TAGFORCE-  TEST MODE\n" */
extern char D_00009A38;  /* Shift-JIS "\x81\x9F" = U+25C6 black diamond (cursor) */
extern char D_00009A3C;  /* Shift-JIS "\x81\xA3" = U+25B2 up triangle (scroll arrow) */
extern char D_00009A40;  /* Shift-JIS "\x81\xA5" = U+25BC down triangle (scroll arrow) */
extern char D_00009A44;  /* "%s\n" */
extern char D_00009A48;  /* "DEBUG OPTION\n" */
extern char D_00009A58;  /* "%s / %s\n" */
extern char D_00009A64;  /* "]" */
extern char D_00009A68;  /* "SOUND TEST\n" */
extern char D_00009A74;  /* "%s%d\n" */
extern char D_00009A7C;  /* "%s EMPTY\n" */
extern char D_00009A88;  /* "%s%d(%d)\n" */
extern char D_00009A94;  /* "%s%s\n" */
extern char D_00009A9C;  /* "%s" */
extern char D_00009AA0;  /* "%s%d(%d)" */
extern char D_00009AAC;  /* Shift-JIS "\x81\xA3" = U+25B2 up triangle (scroll arrow) */
extern char D_00009AB0;  /* Shift-JIS "\x81\xA5" = U+25BC down triangle (scroll arrow) */
extern char D_00009AB4;  /* Shift-JIS "\x81\x9F" = U+25C6 black diamond (cursor) */
extern char D_00009AB8;  /* "No:" */
extern char D_00009ABC;  /* "%04d" */
extern char D_00009AC4;  /* "?? Unknown ??" */
extern char D_00009AD4;  /* "## Entry List ##" */
extern char D_00009AE8;  /* "empty" */
extern char D_00009AF0;  /* "limit list name" */
extern char D_00009B00;  /* "child limit list" */
extern char D_00009B14;  /* "UNKNOWN" */
extern char D_00009B1C;  /* "" */
extern char D_00009B20;  /* "SOLO" */
extern char D_00009B28;  /* "TAG" */
extern char D_00009B2C;  /* "SINGLE" */
extern char D_00009B34;  /* "MATCH" */
extern char D_00009B3C;  /* "8000" */
extern char D_00009B44;  /* "2000" */
extern char D_00009B4C;  /* "4000" */
extern char D_00009B54;  /* "12000" */
extern char D_00009B5C;  /* "16000" */
extern char D_00009B64;  /* "NO LIMIT" */
extern char D_00009B70;  /* "30sec" */
extern char D_00009B78;  /* "60sec" */
extern char D_00009B80;  /* "180sec" */
extern char D_00009B88;  /* "300sec" */
extern char D_00009B90;  /* Shift-JIS "\x81\x9F" = U+25C6 black diamond (cursor) */
extern char D_00009B94;  /* "(%d/%d/%d/%d/%d %s state %d)" */
extern char D_00009BB4;  /* "no group" */
extern char D_00009BC0;  /* "%1dP" */
extern char D_00009BC8;  /* "no member" */
extern char D_00009BD4;  /* "(%d/%d/%d/%d/%d  %s)" */
extern char D_00009BEC;  /* Shift-JIS "\x82\xC9\x8E\x51\x89\xC1\x82\xF0\x90\x5C\x82\xB5\x8D\x9E\x82\xF1\x82\xC5\x82\xA2\x82\xDC\x82\xB7\x81\x42" = "... is applying to join." */
extern char D_00009C08;  /* Shift-JIS "\x83\x66\x83\x62\x83\x4C\x91\x49\x91\xF0" = "Deck select" */
extern char D_00009C14;  /* "%dP:" */
extern char D_00009C1C;  /* "WAITING..." */
extern char D_00009C28;  /* Shift-JIS "\x8E\x51\x89\xC1\x82\xC5\x82\xAB\x82\xDC\x82\xB9\x82\xF1\x82\xC5\x82\xB5\x82\xBD\x81\x42" = "Could not join." */
extern char D_00009C40;  /* Shift-JIS "\x91\xCE\x90\xED\x83\x81\x83\x93\x83\x6F\x81\x5B\x82\xF0\x8A\x6D\x92\xE8\x82\xC5\x82\xAB\x82\xDC\x82\xB9\x82\xF1\x82\xC5\x82\xB5\x82\xBD\x81\x42" = "Could not confirm the duel members." */
extern char D_00009C68;  /* Shift-JIS "\x83\x4F\x83\x8B\x81\x5B\x83\x76\x82\xF0\x89\xF0\x8E\x55\x82\xB5\x82\xDC\x82\xB7\x81\x42" = "Disbanding the group." */

/* ---- .bss ---- */
extern int D_00009C90;  /* menu state: 0 fade-in, 1 menu, 2 fade-out+launch, 3 DEBUG OPTION, 4 savedata wait, 7 SOUND TEST, 8 idle, 0x5A-0x5C boot load, 0x63 matching test, 0xC7 delivery test */
extern int D_00009C94;  /* first visible row of the menu list */
extern int D_00009C98;  /* cursor row (shared by every screen) */
extern int D_00009C9C;  /* fade level 0..7 (colour = (2n+1) * 0x11111111) */
extern int D_00009CA4;  /* frames drawn; bit 3 is the 8-frame blink used for cursors */
extern int D_00009CAC;  /* SOUND TEST value[0] BGM (the 7 values are an int array 0x9CAC..0x9CC4) */
extern int D_00009CB0;  /* SOUND TEST value[1] SE PACK (0..3) */
extern int D_00009CB4;  /* SOUND TEST value[2] SE */
extern int D_00009CB8;  /* SOUND TEST value[3] NPC (2..0x90) */
extern int D_00009CBC;  /* SOUND TEST value[4] NPC-OFS */
extern int D_00009CC0;  /* SOUND TEST value[5] DUEL VOICE (0..0x13E) */
extern int D_00009CC4;  /* SOUND TEST value[6] TITLE VOICE (0x2F..0xA2) */
extern int D_00009CD8;  /* currently loaded duel-voice pack */
extern int D_00009CDC;  /* currently loaded title-voice pack */
extern int D_00009CE4;  /* heap handle returned by ehsys_1856E536(&D_00009CF0, 8 MiB) */
extern char D_00009CF0;  /* 8 MiB heap arena (0x9CF0..0x809CF0) */
extern MenuEntry *D_00809CF0;  /* MenuEntry table in use (defaults to D_000098B0) */
extern int D_00809CF4;  /* number of entries in it (<= 50) */
extern char D_00809CF8;  /* 3rd argument of the module-registration call (same role as rel_html_view g_module_reg) */
extern int D_00809D04;  /* delivery-test screen state (start of a 0xE98-byte block cleared by func_0000206C) */
extern char D_0080A108[];  /* delivery test: entry list, 9 x 0x64 */
extern int D_0080A490;  /* delivery test: row cursor 0/1 */
extern char D_0080A498;  /* delivery test: offer record (0x6C, same layout as D_0080B330) */
extern char D_0080A49C;  /*   .. +0x04 player name */
extern unsigned char D_0080A4DC;  /*   .. +0x44 flag */
extern short D_0080A502;  /*   .. +0x6A card id */
extern int D_0080AB94;  /* delivery test: digit being edited (0..3) */
extern volatile int D_0080AB98;  /* delivery test: card id (0xE10..0x23FE) */
extern int D_0080AB9C;  /* matching-test screen state (start of a 0xE90-byte block cleared by func_000028BC) */
extern int D_0080ABA0;  /* matching test: group-list cursor */
extern int D_0080ABA4;  /* matching test: member-list cursor */
extern int D_0080ABCC;  /* matching test: deck id (0..0x195) */
extern char D_0080ABD0[];  /* matching test: group list, 9 x 0x6C */
extern char D_0080AFA0[];  /* matching test: member list, 9 x 0x64 */
extern int D_0080B328;  /* matching test: rule-edit cursor (0..4) */
extern volatile int D_0080B32C;  /* matching test: rule-edit mode flag */
extern char D_0080B330;  /* matching test: own group record (0x6C) */
extern char D_0080B334;  /*   .. +0x04 player name */
extern unsigned char D_0080B374[];  /*   .. +0x44 DuelRule (b[1] solo/tag, b[2] single/match, b[3] LP, b[4] time limit; +6 name) */
extern unsigned char D_0080B375;  /*   .. DuelRule.b[1] */
extern unsigned char D_0080B376;  /*   .. DuelRule.b[2] */
extern unsigned char D_0080B377;  /*   .. DuelRule.b[3] */
extern unsigned char D_0080B378;  /*   .. DuelRule.b[4] */
extern volatile int D_0080B39C;  /* matching test: handshake step (func_00002D98) */
extern int D_0080B3A0;  /* matching test: start countdown */
extern unsigned char D_0080B3A4[];  /* matching test: 0x200-byte network message buffer */
extern char D_0080B5A4[];  /* matching test: 0x180-byte peer table */
extern char D_0080B664[];  /* matching test: per-peer data */
extern void *D_0080BA24;  /* matching test: heap buffer (0x100) */
extern void *D_0080BA28;  /* matching test: heap buffer (0x400) */
extern char D_0080BA2C;  /* object passed to ehsys_C644C679 before every matching-test draw (just past the 0xE90 block) */

/* ---- engine imports (libehsys_rel; names are NID-derived, shared by all 28 modules) ---- */
extern void ehsys_025FE238(void *, void *);
extern void ehsys_0941BAC6(int);
extern void ehsys_0977A12F(int);
extern int ehsys_0D3E34B5(void);  /* ad-hoc session state code (-1, 0, 5, 7-9, 0xB, 0xE-0x11, 0x15, 0x1E, 0x1F, 0x23) */
extern void ehsys_13A4081A(int);  /* (0/1) toggled around scene changes / reloads */
extern int ehsys_173C81DF(void);
extern int ehsys_1856E536(void *, int);  /* create heap on (arena, size) */
extern void ehsys_1EC5342B(int);
extern void ehsys_1F710553(int);  /* PROPOSED set_language (pair of ehsys_get_language) */
extern void ehsys_20E340D9(int, void *);  /* heap free (heap, ptr) */
extern void ehsys_218CCFB3(int);
extern int ehsys_28F7224B(int);  /* card id is valid */
extern void ehsys_2AE3C0AD(int);  /* play SE / voice id */
extern void ehsys_33ED567B(int);
extern void ehsys_388D8736(int);
extern int ehsys_38DDEDD1(int, int);
extern void ehsys_3A7E4F65(void *, void *, int);  /* bounded wide-string copy (dst, src, n) */
extern void *ehsys_3C1C1F65(void);  /* player name (wide string) */
extern void ehsys_3EC38F85(void *);
extern int ehsys_3F68D487(int, int, int);
extern void *ehsys_41AABF28(int, int);  /* heap alloc (heap, size) */
extern void *ehsys_4510ECA2(int, int);  /* character name (0, character id), 0x32 bytes */
extern unsigned short ehsys_4786AAF9(unsigned short);
extern void ehsys_4DFEDFFA(int);
extern int ehsys_4E3BEA66(void);
extern int ehsys_5471DB90(int);
extern void ehsys_551A8EF2(int, int, int);  /* PROPOSED request_scene(kind, scene, arg): (1, menu id, arg) launches a module */
extern void ehsys_5836AFF1(void);
extern int ehsys_595083E7(int, int);
extern void ehsys_5EB01D69(unsigned short, int);  /* set owned count of a card (id, n) */
extern void ehsys_5F673552(int, int);
extern void ehsys_60B55A50(int);  /* set text/fade colour (0xAABBGGRR) */
extern int ehsys_639AE71F(int);
extern void ehsys_67953A3E(int);
extern void ehsys_6DBE26D9(void);
extern void ehsys_6EEF2E59(void);
extern int ehsys_6F0D36D3(void);
extern void ehsys_6FFA2A28(int, int);  /* start savedata operation (3 boot load, 4 load, 5 save; heap) */
extern int ehsys_70563675(void);
extern void ehsys_71B237CA(int);
extern int ehsys_778817A2(void);  /* poll savedata operation (1 busy, 2/3 finished) */
extern int ehsys_7DFFA500(int, int);
extern void ehsys_7EE06EF8(void);
extern void ehsys_7F9073E5(int, void *);  /* load deck n into a Deck */
extern char *ehsys_82249188(int);  /* peer info (byte 2 == 2 ready, name at +4) */
extern int ehsys_82B715FD(int);  /* first sound id of a sound pack */
extern void ehsys_8517B8C4(int, int);
extern int ehsys_8798050C(int);
extern void ehsys_8852C022(void);
extern unsigned char *ehsys_88B6C7F0(void);  /* duel setup record (players at +0xE8 / +0x2E0) */
extern void ehsys_892F5B43(void *, void *, int);
extern int ehsys_89EB3BA4(int);
extern void *ehsys_8CBC1795(int);
extern void ehsys_8F2316E1(int);
extern void ehsys_9090346C(int, int *, int *);
extern void ehsys_93578062(void *);
extern int ehsys_93EACD81(int);
extern void ehsys_942B03D0(int, int);  /* set font size (w, h) */
extern int ehsys_94A4EC34(int);  /* PROPOSED get_volume(channel 0 BGM / 1 SE / 2 VOICE) */
extern int ehsys_958F298F(void);
extern void ehsys_960D020F(int, int);
extern void ehsys_98CFEF3D(int, void *);
extern int ehsys_99FF9D51(int, int);
extern void ehsys_A0547747(int);
extern void ehsys_A94DF30B(void);
extern int ehsys_AA9913C7(int);  /* is sound pack loaded */
extern void *ehsys_AD0E1E03(int);  /* card name string for a card id */
extern void ehsys_AE84266D(int);
extern void ehsys_B4471B5E(void (*)(void), void (*)(void), void *);  /* module registration (opens every module) */
extern void ehsys_B89D38DC(int, int, int, void *);  /* draw wide string (x<<6, y<<6, width<<6, str) */
extern int ehsys_BA0DA4DE(void);
extern void ehsys_BCEB40ED(void *);
extern void ehsys_BE3756D0(int);  /* play BGM n */
extern void ehsys_C2E59A21(int, void (*)(void), void (*)(void));  /* set per-frame draw callbacks (mode, draw, post) */
extern void ehsys_C4FA973E(unsigned short, int);  /* add one copy of a card (id, n) */
extern void ehsys_C5BDA06E(void);
extern void ehsys_C644C679(void *);
extern void ehsys_C78D825A(int, int, int, int);  /* set deck card (player, section, index, card id) */
extern void ehsys_C792FA38(int);
extern void ehsys_C859D5FE(int);  /* load sound pack */
extern void ehsys_C87BAB32(int, int, void *, ...);  /* PROPOSED debug_printf(x, y, fmt, ...): printf-style text at screen coords */
extern void ehsys_C978255D(int);
extern void ehsys_CA477B34(void *, int);  /* send message (buf, size) */
extern void ehsys_D2B87B2C(int *, int *);
extern void ehsys_D49B6BDD(void *);
extern void ehsys_D5D33185(int);  /* unload sound pack */
extern void ehsys_D853F15C(int);
extern char *ehsys_D8A0DB10(void);  /* system/save record; +0xC = play time in frames */
extern void ehsys_D9F56690(int);
extern void ehsys_DA66B694(int, int, int);  /* set deck section size (player, section 1 main/2 side/3 extra, count) */
extern void ehsys_DF5CB459(int);
extern int ehsys_DFCA450B(int, int *);
extern unsigned short ehsys_E60D8E10(void);
extern void ehsys_E8EB3756(int);
extern void ehsys_EB6FAACF(int, void *);
extern void ehsys_ED2013FB(unsigned short *);
extern void ehsys_EF9EFF0F(void *);
extern void ehsys_F11F0D9A(void *);
extern void ehsys_F1CE9623(int, int);
extern int ehsys_F59F3BD3(void);  /* number of peers */
extern void ehsys_F5D788E4(int);
extern void *ehsys_F5E3080C(void);  /* PROPOSED get_pad: returns PadState (sceCtrl bit masks at +0x18..+0x24) */
extern void ehsys_FC53D253(void);
extern void ehsys_FF141CC8(int, void *);
extern void ehsys_FFA11C10(int, int);  /* PROPOSED set_volume(channel, 0..15) */
extern void ehsys_frame_sync(void);
extern int ehsys_get_button_code(int);  /* 0 = confirm, 1 = cancel button mask (region dependent) */
extern int ehsys_get_language(void);
extern void ehsys_memcpy(void *, void *, int);
extern void ehsys_memset(void *, int, int);
extern int ehsys_sceGuFinish(void);
extern void ehsys_sceGuStart(int, int, int);
extern void ehsys_sceKernelChangeCurrentThreadAttr(int, int);
extern void ehsys_strcat(void *, void *);
extern void ehsys_strcpy(void *, void *);

/* ---- this module ---- */
void func_00000034(void);
void func_00000264(void);
void func_000002DC(void);
void func_00000BE4(int, int, void *);
void func_00000C04(char *, int);
void func_00000C38(int);
void func_00000C78(void);
void func_00000C80(void *);
void func_00000CE4(void);
void func_00000D6C(void);
int func_00001058(void);
int func_000010FC(void);
void func_000015AC(void);
void func_0000160C(void);
void func_00001B84(void);
void func_00001BBC(void);
void func_00001C18(void);
int func_00001C74(void);
int func_00001CFC(void);
void func_00001DE4(void);
int func_00002044(int);
void func_0000206C(void);
int func_000020CC(void);
void func_000024E0(void);
void func_0000289C(int, int, void *);
void func_000028BC(void);
void func_00002A88(void);
void func_00002C58(void);
void func_00002D44(void);
int func_00002D98(void);
S28 *func_000035C0(S28 *, S28 *);
int func_0000362C(void);
void func_000037D0(void *, int);
void func_00003B64(int, int, void *);
void func_00003F40(void);

/* func_00000000 - module entry: registers the menu main loop (func_00000034) and an empty
 * stop callback (func_00000264) with the engine.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000000(void) {
    ehsys_B4471B5E(func_00000034, func_00000264, &D_00809CF8);
    return 0;
}

/* func_00000034 - menu main loop. Creates the 8 MiB heap, installs the draw callback
 * (func_000002DC), then runs one handler per frame on the menu state
 * D_00009C90 until a handler reports that it launched another module.
 * If ehsys_ED2013FB returns three zero halfwords the module first runs the
 * boot savedata load (states 0x5A..0x5C).
 * The dispatch is a `switch` written in ascending case order: MWCC emits
 * the tests in REVERSE source order (0xC7 first) and the bodies in source
 * order, which is exactly the target's layout.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000034(void) {
    unsigned short unk[3];

    ehsys_sceKernelChangeCurrentThreadAttr(0, 0x4000);
    D_00009CE4 = ehsys_1856E536(&D_00009CF0, 0x800000);
    ehsys_13A4081A(0);
    ehsys_C2E59A21(1, func_000002DC, func_00000C78);
    ehsys_60B55A50(0);
    D_00009C9C = 0;
    D_00009CA4 = 0;
    D_00009C98 = 0;
    D_00009C90 = 0;
    ehsys_ED2013FB(unk);
    if (unk[0] == 0 && unk[1] == 0 && unk[2] == 0) {
        D_00009C90 = 0x5A;
    }
    while (1) {
        ehsys_frame_sync();
        switch (D_00009C90) {
        case 0:
            func_00000CE4();
            break;
        case 1:
            func_00000D6C();
            break;
        case 2:
            if (func_00001058()) {
                return;
            }
            break;
        case 3:
            if (func_000010FC()) {
                return;
            }
            break;
        case 4:
            func_000015AC();
            break;
        case 6:
        case 7:
            func_0000160C();
            break;
        case 8:
            break;
        case 0x5A:
            func_00001B84();
            break;
        case 0x5B:
            func_00001BBC();
            break;
        case 0x5C:
            func_00001C18();
            break;
        case 0x63:
            if (func_00001C74()) {
                return;
            }
            break;
        case 0xC7:
            if (func_00001CFC()) {
                return;
            }
            break;
        }
    }
}

/* func_00000264 - empty stop callback registered by func_00000000.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000264(void) {
}

/* func_0000026C - split a frame count into hours/minutes/seconds at 30 fps
 * (108000 frames per hour, 1800 per minute, 30 per second). The remainder
 * has to be formed by subtraction, not `%`, to get the target's
 * multiply-by-constant sequences.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_0000026C(int t, int *out) {
    int h, m;
    out[0] = h = t / 108000;
    t -= h * 108000;
    out[1] = m = t / 1800;
    t -= m * 1800;
    out[2] = t / 30;
}

/* func_000002DC - draw callback for every screen of the module (registered by
 * func_00000034 via ehsys_C2E59A21): starts a GU list, draws the title
 * "YUGI OH 5Ds -TAGFORCE-  TEST MODE" and the menu list (Japanese or
 * English name chosen by `ehsys_get_language() != 0`), the DEBUG OPTION
 * page, the SOUND TEST page, or defers to the matching-test (func_00003F40)
 * and delivery-test (func_000024E0) draw routines.
 * MATCHING NOTES: MWCC hands saved registers out by declaration order (the
 * last declared local gets $s0), so the three scratch locals are declared
 * `int c, b, a;` and reused per case exactly as the target reuses $s2/$s1/$s0.
 * Case 8 is `return;`, case 6 an empty `break;`. func_00000C04 receives
 * __FILE__/__LINE__ ("debug_menu.c", 0x175 = line 373 of the original).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000002DC(void) {
    char *sys = ehsys_D8A0DB10();
    int tm[3];
    char buf[0x10];
    int c, b, a;

    D_00009CA4 = D_00009CA4 + 1;
    func_00000C38(0);
    ehsys_D9F56690(0);
    ehsys_A0547747(0);
    ehsys_8F2316E1(0);
    ehsys_388D8736(0x17);
    func_00000C04(&D_00009A04, 0x175);
    ehsys_942B03D0(0xE, 0xE);

    switch (D_00009C90) {
    case 0:
    case 1:
    case 2:
        func_0000026C(*(int *)(sys + 0xC), tm);
        ehsys_C87BAB32(0x10, 0, &D_00009A14);
        if ((D_00009CA4 >> 3) & 1) {
            ehsys_C87BAB32(0, (D_00009C98 - D_00009C94) * 0xE + 0x1C, &D_00009A38);
        }
        if (D_00009C94 > 0 && ((D_00009CA4 >> 3) & 1)) {
            ehsys_C87BAB32(0x48, 0xE, &D_00009A3C);
        }
        if (D_00009C94 + 0xF < D_00809CF4) {
            if ((D_00009CA4 >> 3) & 1) {
                ehsys_C87BAB32(0x48, 0xEE, &D_00009A40);
            }
        }
        for (a = 0; a < 0xF; a++) {
            b = D_00009C94 + a;
            if (b >= D_00809CF4) {
                break;
            }
            c = ehsys_get_language() != 0;
            ehsys_C87BAB32(0x10, a * 0xE + 0x1C, &D_00009A44, ((char **)&D_00809CF0[b])[c]);
        }
        break;
    case 3:
        ehsys_C87BAB32(0x10, 0, &D_00009A48);
        if ((D_00009CA4 >> 3) & 1) {
            ehsys_C87BAB32(0, D_00009C98 * 0xE + 0x1C, &D_00009A38);
        }
        for (a = 0; a < 8; a++) {
            switch (a) {
            case 0:
                ehsys_C87BAB32(0x10, a * 0xE + 0x1C, &D_00009A58, D_00009250[a], D_000091E0[ehsys_get_language()]);
                break;
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
                ehsys_C87BAB32(0x10, a * 0xE + 0x1C, &D_00009A44, D_00009250[a]);
                break;
            case 7:
                ehsys_C87BAB32(0x10, a * 0xE + 0x1C, &D_00009A58, D_00009250[a], &D_00009220);
                break;
            }
        }
        break;
    case 4:
        ehsys_C5BDA06E();
        break;
    case 6:
        break;
    case 7:
        b = ehsys_38DDEDD1(D_00009CC0, 1);
        a = ehsys_595083E7(D_00009CB8, D_00009CBC);
        if (a) {
            a = D_00009CC0 + ehsys_82B715FD(a);
            ehsys_memset(buf, 0, 0x10);
            b = ehsys_38DDEDD1(a, 1);
        }
        if (b >= 0) {
            for (c = 0; c < b / 256; c++) {
                ehsys_strcat(buf, &D_00009A64);
            }
        }
        ehsys_0977A12F(0);
        ehsys_C87BAB32(0x10, 0, &D_00009A68);
        if ((D_00009CA4 >> 3) & 1) {
            ehsys_C87BAB32(0, D_00009C98 * 0xE + 0x1C, &D_00009A38);
        }
        for (a = 0; a < 10; a++) {
            switch (a) {
            case 0:
            case 4:
                ehsys_C87BAB32(0x10, a * 0xE + 0x1C, &D_00009A74, D_00009350[a], (&D_00009C90)[a + 7]);
                break;
            case 1:
                b = ehsys_595083E7(D_00009CB8, D_00009CBC);
                if (b) {
                    c = D_00009CC0 + ehsys_82B715FD(b);
                    b = ehsys_8798050C(c);
                } else {
                    c = 0;
                }
                if (b == 0 || ehsys_AA9913C7(b) == 0) {
                    ehsys_60B55A50(0xFF0000FF);
                }
                if (b == 0) {
                    ehsys_C87BAB32(0x10, a * 0xE + 0x1C, &D_00009A7C, D_00009350[a]);
                } else {
                    ehsys_C87BAB32(0x10, a * 0xE + 0x1C, &D_00009A88, D_00009350[a], (&D_00009C90)[a + 7], c);
                }
                ehsys_60B55A50(-1);
                break;
            case 2:
                ehsys_C87BAB32(0x10, a * 0xE + 0x1C, &D_00009A94, D_00009350[a], D_00009490[D_00009CB0]);
                break;
            case 3:
                b = D_00009CB4;
                ehsys_C87BAB32(0x10, a * 0xE + 0x1C, &D_00009A88, D_00009350[a], b, b + ehsys_82B715FD(D_00009CB0 + 0x2B7E));
                break;
            case 5:
                b = a * 0xE + 0x1C;
                ehsys_C87BAB32(0x10, b, &D_00009A9C, D_00009350[a]);
                func_00000BE4(0x38, b, ehsys_4510ECA2(0, D_00009CB8));
                break;
            case 6:
                b = D_00009CC4;
                c = ehsys_82B715FD(b);
                {
                    int n = D_00009CC4 - 0x2E;
                    if (ehsys_AA9913C7(b) == 0) {
                        ehsys_60B55A50(0xFF0000FF);
                    }
                    ehsys_C87BAB32(0x10, a * 0xE + 0x1C, &D_00009AA0, D_00009350[a], n, c);
                }
                ehsys_60B55A50(-1);
                break;
            case 7:
                ehsys_C87BAB32(0x10, a * 0xE + 0x1C, &D_00009A74, D_00009350[a], ehsys_94A4EC34(0));
                break;
            case 8:
                ehsys_C87BAB32(0x10, a * 0xE + 0x1C, &D_00009A74, D_00009350[a], ehsys_94A4EC34(1));
                break;
            case 9:
                ehsys_C87BAB32(0x10, a * 0xE + 0x1C, &D_00009A74, D_00009350[a], ehsys_94A4EC34(2));
                break;
            }
        }
        break;
    case 8:
        return;
    case 0x5B:
        ehsys_C5BDA06E();
        break;
    case 0x63:
        func_00003F40();
        break;
    case 0xC7:
        func_000024E0();
        break;
    }
}

/* func_00000BE4 - draw a wide string at (x, y) clipped to the right screen edge
 * (480 px); coordinates are 26.6 fixed point. Identical copies exist at
 * func_0000289C and func_00003B64, i.e. a static helper repeated in three
 * source files of this module.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000BE4(int arg0, int arg1, void *arg2) {
    ehsys_B89D38DC(arg0 << 6, arg1 << 6, (0x1E0 - arg0) << 6, arg2);
}

/* func_00000C04 - finish the GU display list and submit it. Takes __FILE__ and
 * __LINE__ (see func_000002DC) and ignores them.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000C04(char *file, int line) {
    int a = ehsys_6F0D36D3();
    ehsys_1EC5342B(a + ehsys_sceGuFinish() - 8);
}

/* func_00000C38 - start a GU display list on the buffer returned by ehsys_DFCA450B.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000C38(int arg0) {
    int size;
    int buf;
    ehsys_4DFEDFFA(buf = ehsys_DFCA450B(arg0, &size));
    ehsys_sceGuStart(2, buf, size);
}

/* func_00000C78 - empty second callback passed to ehsys_C2E59A21.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000C78(void) {
}

/* func_00000C80 - select the MenuEntry table to show (NULL = the built-in one at
 * D_000098B0), count its entries (up to 50, terminated by a NULL first
 * field) and reset the cursor.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000C80(void *table) {
    int i;
    if (table == 0) {
        table = &D_000098B0;
    }
    D_00809CF0 = table;
    i = 0;
    while (i < 0x32 && *(int *)((char *)table + i * 0x14) != 0) {
        i++;
    }
    D_00809CF4 = i;
    D_00009C98 = 0;
    D_00009C94 = 0;
}

/* func_00000CE4 - state 0: fade the menu in over 7 frames, then go to state 1.
 * The menu table is set up on first entry.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000CE4(void) {
    int v;
    if (D_00809CF0 == 0) {
        func_00000C80(0);
    }
    if (D_00009C9C < 7) {
        v = D_00009C9C + 1;
        D_00009C9C = v;
        ehsys_60B55A50((v * 2 + 1) * 0x11111111);
    } else {
        D_00009C90 = D_00009C90 + 1;
    }
}

/* func_00000D6C - state 1: menu input. Up/down move the cursor (wrapping), left/right
 * page by 15 rows, the visible window follows the cursor, R opens DEBUG
 * OPTION (state 3), L opens SOUND TEST (state 7, loading the four SEPAC_*
 * packs 0x2B7E..0x2B81), confirm advances to state 2 (fade-out + launch)
 * except for row 13 "matching test" which has no handler and goes to
 * state 0x63 instead; cancel resets the table.
 * MATCHING NOTE: the target reloads the cursor and scroll globals for the
 * second window-follow test but not inside the first one. That is
 * reproduced by a local copy for the first test and volatile reads (V())
 * for the second; every other access in the function is a plain global.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000D6C(void) {
    PadState *pad = ehsys_F5E3080C();

    if (pad->rep & PAD_UP) {
        D_00009C98 = D_00009C98 - 1;
        if (D_00009C98 < 0) {
            D_00009C98 = D_00809CF4 - 1;
        }
    }
    if (pad->rep & PAD_LEFT) {
        D_00009C98 = D_00009C98 - 0xF;
        if (D_00009C98 < 0) {
            D_00009C98 = 0;
        }
        D_00009C94 = D_00009C94 - 0xF;
        if (D_00009C94 < 0) {
            D_00009C94 = 0;
        }
    }
    if (pad->rep & PAD_DOWN) {
        D_00009C98 = D_00009C98 + 1;
        if (D_00009C98 > D_00809CF4 - 1) {
            D_00009C98 = 0;
        }
    }
    if (pad->rep & PAD_RIGHT) {
        D_00009C98 = D_00009C98 + 0xF;
        if (D_00009C98 > D_00809CF4 - 1) {
            D_00009C98 = D_00809CF4 - 1;
        }
        D_00009C94 = D_00009C94 + 0xF;
        if (D_00009C94 > D_00809CF4 - 0xF) {
            D_00009C94 = D_00809CF4 - 0xF;
            if (D_00009C94 < 0) {
                D_00009C94 = 0;
            }
        }
    }
    {
        int cur = D_00009C98;
        if (cur >= D_00009C94 + 0xF) {
            D_00009C94 = cur - 0xE;
        }
    }
    {
        int cur = V(D_00009C98);
        if (cur < V(D_00009C94)) {
            D_00009C94 = cur;
        }
    }
    if (pad->trg & PAD_RTRIGGER) {
        D_00009C90 = 3;
        D_00009C98 = 0;
    }
    if (pad->trg & PAD_LTRIGGER) {
        ehsys_F5D788E4(D_00009CE4);
        D_00009C90 = 7;
        D_00009C98 = 0;
        D_00009CAC = 0;
        D_00009CB8 = 2;
        D_00009CBC = 0;
        D_00009CD8 = 0;
        D_00009CC4 = 0x2F;
        D_00009CDC = 0;
        ehsys_C859D5FE(0x2B7E);
        ehsys_C859D5FE(0x2B7F);
        ehsys_C859D5FE(0x2B81);
        ehsys_C859D5FE(0x2B80);
    }
    if (pad->trg2 & (ehsys_get_button_code(0) & 0xFFFF)) {
        D_00009C90 = D_00009C90 + 1;
        if (D_00009C98 == 0xD) {
            D_00009C90 = 0x63;
            func_000028BC();
            return;
        }
    }
    if (pad->trg2 & (ehsys_get_button_code(1) & 0xFFFF)) {
        func_00000C80(0);
    }
}

/* func_00001058 - state 2: fade out, then call the selected MenuEntry handler
 * (func(arg0, arg1)); returns 1 when the handler launched a module.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00001058(void) {
    MenuEntry *e;

    if (D_00009C9C--) {
        ehsys_60B55A50((D_00009C9C * 2 + 1) * 0x11111111);
        return 0;
    }
    e = &D_00809CF0[D_00009C98];
    if (e->func && e->func(e->arg0, e->arg1)) {
        return 1;
    }
    return 0;
}

/* func_000010FC - state 3: DEBUG OPTION page (8 rows, names at D_00009250):
 *   0 language        cycle ehsys_get_language() 1..5 via ehsys_1F710553 and reload
 *   1 clear all data  reset the save and build a fresh profile (0x9A4-byte block)
 *   2 get all card    func_00001DE4
 *   3 save data       ehsys_6FFA2A28(5, heap), then state 4
 *   4 load data       ehsys_6FFA2A28(4, heap), then state 4
 *   5 demonstration   fill the duel setup record with two CPU players and
 *                     request scene 6 (duel); returns 1
 *   6 delivery test   state 0xC7
 *   7 Sale Zone       display only (shows "Europe")
 * MATCHING NOTES: locals are declared `char prm[0x9A4]; Deck deck;
 * char buf[0x20];` to get the target frame layout, the wrap test is
 * written `> 7` (slti $at) rather than `>= 8`, and case 7 needs its own
 * explicit `break`.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000010FC(void) {
    char prm[0x9A4];
    Deck deck;
    char buf[0x20];
    PadState *pad = ehsys_F5E3080C();
    int lang;
    int side, i;
    unsigned int j;
    char *dp;
    char *pl;

    if (pad->trg & PAD_RTRIGGER) {
        D_00009C90 = 1;
        D_00009C98 = 0;
        return 0;
    }
    if (pad->rep & PAD_UP) {
        D_00009C98 = D_00009C98 - 1;
        if (D_00009C98 < 0) {
            D_00009C98 = 7;
        }
        return 0;
    }
    if (pad->rep & PAD_DOWN) {
        D_00009C98 = D_00009C98 + 1;
        if (D_00009C98 > 7) {
            D_00009C98 = 0;
        }
        return 0;
    }
    if (pad->trg2 & (ehsys_get_button_code(0) & 0xFFFF)) {
    switch (D_00009C98) {
    case 0:
        lang = ehsys_get_language() + 1;
        if (lang == 6) {
            lang = 1;
        }
        ehsys_1F710553(lang);
        ehsys_5836AFF1();
        ehsys_13A4081A(1);
        while (!ehsys_4E3BEA66()) {
            ehsys_frame_sync();
        }
        ehsys_13A4081A(0);
        break;
    case 1:
        ehsys_6EEF2E59();
        D_00009C90 = 1;
        D_00009C98 = 0;
        ehsys_memset(prm, 0, 0x9A4);
        prm[0xD4] = 1;
        prm[0xD5] = 1;
        prm[0xD6] = 1;
        *(short *)(prm + 0xE8) = 1;
        *(short *)(prm + 0x2E8) = 1;
        *(short *)(prm + 0x2E0) = 2;
        *(short *)(prm + 0xEA) = 0;
        *(short *)(prm + 0xF0) = 0;
        *(short *)(prm + 0x2E2) = 0;
        *(int *)(prm + 0xCC) = 0;
        *(int *)(prm + 0xD0) = 0;
        ehsys_7F9073E5(0, &deck);
        for (i = 0; i < 2; i++) {
            side = i != 0;
            ehsys_DA66B694(side, 1, deck.n_main);
            for (j = 0; j < deck.n_main; j++) {
                ehsys_C78D825A(side, 1, j, deck.main[j]);
            }
            ehsys_DA66B694(side, 2, deck.n_side);
            for (j = 0; j < deck.n_side; j++) {
                ehsys_C78D825A(side, 2, j, deck.side[j]);
            }
            ehsys_DA66B694(side, 3, deck.n_extra);
            for (j = 0; j < deck.n_extra; j++) {
                ehsys_C78D825A(side, 3, j, deck.extra[j]);
            }
        }
        ehsys_025FE238(buf, ehsys_8CBC1795(0));
        ehsys_98CFEF3D(0, buf);
        ehsys_F11F0D9A(prm);
        ehsys_E8EB3756(2);
        ehsys_AE84266D(1);
        ehsys_960D020F(0, 1);
        break;
    case 2:
        func_00001DE4();
        D_00009C90 = 1;
        D_00009C98 = 0;
        break;
    case 3:
        ehsys_6FFA2A28(5, D_00009CE4);
        D_00009C90 = 4;
        break;
    case 4:
        ehsys_6FFA2A28(4, D_00009CE4);
        D_00009C90 = 4;
        break;
    case 5:
        dp = (char *)ehsys_88B6C7F0();
        dp[0xD4] = 5;
        dp[0xD5] = 1;
        dp[0xD6] = 1;
        dp[0xD8] = 2;
        *(int *)(dp + 0xCC) = 3;
        *(int *)(dp + 0xD0) = 0;
        *(unsigned short *)(dp + 0xE8) = 2;
        *(unsigned short *)(dp + 0xEA) = 0;
        *(unsigned short *)(dp + 0xF0) = 2;
        pl = dp + 0xE8;
        ehsys_memcpy(pl + 0x20, ehsys_4510ECA2(0, *(unsigned short *)(dp + 0xE8)), 0x32);
        ehsys_memcpy(pl + 0x52, ehsys_8CBC1795(*(unsigned short *)(pl + 8)), 0x80);
        ehsys_7F9073E5(*(unsigned short *)(pl + 8), pl + 0xD4);
        *(unsigned short *)(dp + 0x2E0) = 4;
        *(unsigned short *)(dp + 0x2E2) = 0;
        *(unsigned short *)(dp + 0x2E8) = 0xA;
        pl = dp + 0x2E0;
        ehsys_memcpy(pl + 0x20, ehsys_4510ECA2(0, *(unsigned short *)(dp + 0x2E0)), 0x32);
        ehsys_memcpy(pl + 0x52, ehsys_8CBC1795(*(unsigned short *)(pl + 8)), 0x80);
        ehsys_7F9073E5(*(unsigned short *)(pl + 8), pl + 0xD4);
        ehsys_C2E59A21(0, 0, 0);
        ehsys_13A4081A(1);
        ehsys_frame_sync();
        ehsys_551A8EF2(2, 6, 0);
        return 1;
    case 6:
        D_00009C90 = 0xC7;
        D_00009C98 = 0;
        func_0000206C();
        break;
    case 7:
        break;
    }
    }
    return 0;
}

/* func_000015AC - state 4: wait for the savedata operation, then back to the menu.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000015AC(void) {
    switch (ehsys_778817A2()) {
    case 1:
        break;
    case 2:
    case 3:
        D_00009C90 = 0;
        D_00009C98 = 0;
        D_00009C9C = 0;
        break;
    }
}

/* func_0000160C - state 7: SOUND TEST input. Ten rows: BGM, SE PACK, SE, NPC, NPC-OFS,
 * DUEL VOICE, TITLE VOICE and three volumes; left/right edit the value,
 * confirm plays it, cancel stops all sound, L leaves. Voice packs are
 * (un)loaded on the fly whenever left/right are released.
 * MATCHING NOTES: each clamp is `if ((V(x) += d) < lo) ...; if (V(x) >= hi)`
 * - the target reloads x only for the second test, which only volatile
 * compound assignment reproduces; a `> N-1` test gives slti $at where the
 * target has it, `>=` gives slti $v0.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_0000160C(void) {
    PadState *pad = ehsys_F5E3080C();
    int d;
    int v;
    int h;

    if (pad->trg & PAD_LTRIGGER) {
        D_00009C90 = 1;
        D_00009C98 = 0;
        ehsys_7EE06EF8();
        return;
    }
    if (pad->rep & PAD_UP) {
        D_00009C98 = D_00009C98 - 1;
        if (D_00009C98 < 0) {
            D_00009C98 = 9;
        }
        return;
    }
    if (pad->rep & PAD_DOWN) {
        D_00009C98 = D_00009C98 + 1;
        if (D_00009C98 > 9) {
            D_00009C98 = 0;
        }
        return;
    }
    if (pad->rep & (PAD_LEFT | PAD_RIGHT)) {
        if (pad->rep & PAD_LEFT) {
            d = -1;
        }
        if (pad->rep & PAD_RIGHT) {
            d = 1;
        }
        switch (D_00009C98) {
        case 0:
            D_00009CAC = D_00009CAC + d;
            if (((volatile int *)&D_00009CAC)[D_00009C98] < 0) {
                ((volatile int *)&D_00009CAC)[D_00009C98] = 0;
            }
            if (((volatile int *)&D_00009CAC)[D_00009C98] > 0x2E) {
                ((volatile int *)&D_00009CAC)[D_00009C98] = 0x2E;
            }
            break;
        case 1:
            if ((V(D_00009CB0) += d) < 0) {
                D_00009CB0 = 3;
            }
            if (V(D_00009CB0) >= 4) {
                D_00009CB0 = 0;
            }
            D_00009CB4 = 0;
            break;
        case 2:
            if ((V(D_00009CB4) += d) < 0) {
                D_00009CB4 = 0;
            }
            break;
        case 3:
            if ((V(D_00009CB8) += d) < 2) {
                D_00009CB8 = 2;
            }
            if (V(D_00009CB8) >= 0x91) {
                D_00009CB8 = 0x90;
            }
            D_00009CC0 = 0;
            break;
        case 4:
            break;
        case 5:
            if ((V(D_00009CC0) += d) < 0) {
                D_00009CC0 = 0x13E;
            }
            if (V(D_00009CC0) >= 0x13F) {
                D_00009CC0 = 0;
            }
            break;
        case 6:
            if ((V(D_00009CC4) += d) < 0x2F) {
                D_00009CC4 = 0xA2;
            }
            if (V(D_00009CC4) > 0xA2) {
                D_00009CC4 = 0x2F;
            }
            break;
        case 7:
        case 8:
        case 9:
            v = ehsys_94A4EC34(D_00009C98 - 7);
            v += d;
            if (v < 0) {
                v = 0;
            }
            if (v > 0xF) {
                v = 0xF;
            }
            ehsys_FFA11C10(D_00009C98 - 7, v);
            break;
        }
    }
    if (!(pad->held & (PAD_LEFT | PAD_RIGHT))) {
        v = ehsys_595083E7(D_00009CB8, D_00009CBC);
        if (v) {
            h = ehsys_8798050C(D_00009CC0 + ehsys_82B715FD(v));
            if (h != D_00009CD8) {
                if (D_00009CD8) {
                    ehsys_D5D33185(D_00009CD8);
                    D_00009CD8 = 0;
                }
                if (h) {
                    ehsys_C859D5FE(h);
                    while (ehsys_AA9913C7(h) == 0) {
                        ehsys_frame_sync();
                    }
                    D_00009CD8 = h;
                }
            }
        }
        h = D_00009CC4;
        if (h && h != D_00009CDC) {
            if (D_00009CDC) {
                ehsys_D5D33185(D_00009CDC);
                D_00009CDC = 0;
            }
            ehsys_C859D5FE(h);
            while (ehsys_AA9913C7(h) == 0) {
                ehsys_frame_sync();
            }
            D_00009CDC = h;
        }
    }
    if (pad->trg & (ehsys_get_button_code(1) & 0xFFFF)) {
        ehsys_DF5CB459(400);
    }
    if (pad->trg & (ehsys_get_button_code(0) & 0xFFFF)) {
        switch (D_00009C98) {
        case 0:
            ehsys_BE3756D0(D_00009CAC);
            break;
        case 2:
            ehsys_2AE3C0AD(D_00009CB4 + ehsys_82B715FD(D_00009CB0 + 0x2B7E));
            break;
        case 5:
            v = ehsys_595083E7(D_00009CB8, D_00009CBC);
            if (v) {
                h = D_00009CC0 + ehsys_82B715FD(v);
                if (ehsys_AA9913C7(ehsys_8798050C(h))) {
                    ehsys_2AE3C0AD(h);
                }
            }
            break;
        case 6:
            h = D_00009CC4;
            if (ehsys_AA9913C7(h)) {
                ehsys_2AE3C0AD(ehsys_82B715FD(h));
            }
            break;
        }
    }
}

/* func_00001B84 - state 0x5A: start the boot savedata load (operation 3), state 0x5B.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001B84(void) {
    ehsys_6FFA2A28(3, D_00009CE4);
    D_00009C90 = D_00009C90 + 1;
}

/* func_00001BBC - state 0x5B: wait for it, then state 0x5C.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001BBC(void) {
    switch (ehsys_778817A2()) {
    case 1:
        break;
    case 2:
        D_00009C90 = 0x5C;
        break;
    case 3:
        D_00009C90 = 0x5C;
        break;
    }
}

/* func_00001C18 - state 0x5C: reload the engine resources and return to state 0.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001C18(void) {
    ehsys_13A4081A(1);
    ehsys_5836AFF1();
    while (!ehsys_4E3BEA66()) {
        ehsys_frame_sync();
    }
    ehsys_13A4081A(0);
    D_00009C90 = 0;
}

/* func_00001C74 - state 0x63: matching test frame. 1 = a duel was arranged: request
 * scene 6 and leave; 2 = session closed: back to the menu.
 * MATCHING NOTE: `case 0: break;` (not `return 0;`) is what gives the
 * target's unfilled `b` + shared `return 0` tail.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00001C74(void) {
    switch (func_0000362C()) {
    case 0:
        break;
    case 1:
        ehsys_C2E59A21(0, 0, 0);
        ehsys_13A4081A(1);
        ehsys_frame_sync();
        ehsys_551A8EF2(0xC, 6, 0);
        return 1;
    case 2:
        D_00009C9C = 0;
        D_00009C90 = 0;
        break;
    }
    return 0;
}

/* func_00001CFC - state 0xC7: delivery test frame, same shape as func_00001C74.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00001CFC(void) {
    switch (func_000020CC()) {
    case 0:
        break;
    case 1:
        ehsys_C2E59A21(0, 0, 0);
        ehsys_13A4081A(1);
        ehsys_frame_sync();
        ehsys_551A8EF2(0xC, 6, 0);
        return 1;
    case 2:
        D_00009C9C = 0;
        D_00009C90 = 0;
        break;
    }
    return 0;
}

/* func_00001D84 - MenuEntry handler used by every row of the built-in table:
 * request scene `arg0` (2 story ... 15 gallery) with argument `arg1`.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00001D84(int arg0, int arg1) {
    ehsys_C2E59A21(0, 0, 0);
    ehsys_13A4081A(1);
    ehsys_frame_sync();
    ehsys_551A8EF2(1, (unsigned short)arg0, (unsigned short)arg1);
    return 1;
}

/* func_00001DE4 - "get all card": zero the count of every card id 0xE10..0x23FE, rebuild
 * player 0's deck from deck 0, add three more copies of every card when
 * the cursor is on row 2 (the "get all card" row, i.e. always when reached
 * from DEBUG OPTION), then walk the 0x91 (145) character ids - the same
 * range as the SOUND TEST NPC row - registering their decks/cards.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001DE4(void) {
    Deck deck;
    int a, b;
    int i;
    unsigned int j;
    int k, n, x;

    for (i = 0xE10; i < 0x23FF; i++) {
        ehsys_5EB01D69(i, 0);
    }
    ehsys_7F9073E5(0, &deck);
    ehsys_218CCFB3(0);
    ehsys_DA66B694(0, 1, deck.n_main);
    for (j = 0; j < deck.n_main; j++) {
        ehsys_C78D825A(0, 1, j, deck.main[j]);
        ehsys_C4FA973E(deck.main[j], 1);
    }
    ehsys_DA66B694(0, 2, deck.n_side);
    for (j = 0; j < deck.n_side; j++) {
        ehsys_C78D825A(0, 2, j, deck.side[j]);
        ehsys_C4FA973E(deck.side[j], 1);
    }
    ehsys_DA66B694(0, 3, deck.n_extra);
    for (j = 0; j < deck.n_extra; j++) {
        ehsys_C78D825A(0, 3, j, deck.extra[j]);
        ehsys_C4FA973E(deck.extra[j], 1);
    }
    if (D_00009C98 == 2) {
        for (i = 0xE10; i < 0x23FF; i++) {
            ehsys_C4FA973E(i, 1);
            ehsys_C4FA973E(i, 1);
            ehsys_C4FA973E(i, 1);
        }
    }
    for (i = 0; i < 0x91; i++) {
        if (ehsys_99FF9D51(i, 0) && ehsys_639AE71F(i)) {
            n = ehsys_89EB3BA4(i);
            for (k = 0; k < n; k++) {
                x = ehsys_7DFFA500(i, k);
                ehsys_8517B8C4(x, i);
                ehsys_F1CE9623(i, x);
            }
        }
    }
    for (i = 0; i < 0x91; i++) {
        ehsys_9090346C(i, &a, &b);
        ehsys_5F673552(a, b);
    }
}

/* func_00002044 - 10 to the power n (digit step for the delivery-test card id).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00002044(int n) {
    int r = 1;
    while (n--) {
        r *= 10;
    }
    return r;
}

/* func_0000206C - delivery test init: clear the 0xE98-byte state block and the offer record.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_0000206C(void) {
    ehsys_memset(&D_00809D04, 0, 0xE98);
    ehsys_FC53D253();
    ehsys_memset(&D_0080A498, 0, 0x6C);
    ehsys_3A7E4F65(&D_0080A49C, ehsys_3C1C1F65(), 0xF);
}

/* func_000020CC - delivery test update, driven by the ad-hoc session state
 * (ehsys_0D3E34B5): 0xB = edit the card id digit by digit (L/R add the
 * digit's power of ten) or step through the card list, START offers the
 * card; 0x1E = entry list; -1 ends the screen (returns 2).
 * MATCHING NOTE: the card id D_0080AB98 must be volatile - the clamp
 * reloads it between its two tests.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000020CC(void) {
    PadState *pad = ehsys_F5E3080C();
    int v, n, i, changed, r;

    switch (ehsys_0D3E34B5()) {
    case -1:
        ehsys_A94DF30B();
        return 2;
    case 0:
        ehsys_67953A3E(4);
        break;
    case 0xB:
        D_00809D04 = 1;
        if (pad->rep & PAD_UP) {
            if (D_0080A490 != 0) {
                D_0080A490 = D_0080A490 - 1;
            }
        }
        if (pad->rep & PAD_DOWN) {
            if (D_0080A490 <= 0) {
                D_0080A490 = D_0080A490 + 1;
            }
        }
        switch (D_0080A490) {
        case 0:
            if (pad->rep & PAD_RIGHT) {
                if (D_0080AB94 > 0) {
                    D_0080AB94 = D_0080AB94 - 1;
                }
            }
            if (pad->rep & PAD_LEFT) {
                if (D_0080AB94 < 3) {
                    D_0080AB94 = D_0080AB94 + 1;
                }
            }
            if (pad->rep & PAD_LTRIGGER) {
                D_0080AB98 = D_0080AB98 - func_00002044(D_0080AB94);
            }
            if (pad->rep & PAD_RTRIGGER) {
                D_0080AB98 = D_0080AB98 + func_00002044(D_0080AB94);
            }
            if (D_0080AB98 < 0xE10) {
                D_0080AB98 = 0xE10;
            }
            if (D_0080AB98 >= 0x23FF) {
                D_0080AB98 = 0x23FE;
            }
            break;
        case 1:
            r = pad->rep;
            if (r & PAD_LTRIGGER) {
                v = ehsys_5471DB90(D_0080AB98);
                if (v != 0) {
                    v--;
                }
                D_0080AB98 = ehsys_4786AAF9(v);
            } else if (r & PAD_RTRIGGER) {
                v = ehsys_5471DB90(D_0080AB98);
                if (v < ehsys_E60D8E10() - 1) {
                    v++;
                }
                D_0080AB98 = ehsys_4786AAF9(v);
            } else {
                n = ehsys_93EACD81(D_0080AB98);
                r = pad->rep;
                changed = 0;
                if (r & PAD_LEFT) {
                    changed = 1;
                    n--;
                }
                if (r & PAD_RIGHT) {
                    changed = 1;
                    n++;
                }
                if (changed) {
                    if (n < 0) {
                        n = 0;
                    }
                    if (n >= 0x12C0) {
                        n = 0x12BF;
                    }
                    for (i = 0xE10; i < 0x23FF; i++) {
                        if (n == ehsys_93EACD81(i)) {
                            D_0080AB98 = i;
                            break;
                        }
                    }
                }
            }
            break;
        }
        if ((pad->rep & PAD_START) && ehsys_28F7224B(D_0080AB98)) {
            D_0080A4DC = 1;
            D_0080A502 = D_0080AB98;
            ehsys_D49B6BDD(&D_0080A498);
        } else if (pad->trg & PAD_CROSS) {
            ehsys_67953A3E(0x21);
        }
        break;
    case 0x1E:
        D_00809D04 = 3;
        ehsys_93578062(D_0080A108);
        if (pad->trg & PAD_CROSS) {
            ehsys_67953A3E(0x13);
            D_00809D04 = 0;
        }
        break;
    case 5:
    case 7:
    case 8:
    case 9:
        D_00809D04 = 0;
        break;
    }
    return 0;
}

/* func_000024E0 - delivery test draw: card-number entry (arrows, "No:", "%04d", card
 * name or "?? Unknown ??") and the 9-row "## Entry List ##".
 * MATCHING NOTE: locals declared `char c[0x14]; int a, b;` in that order.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000024E0(void) {
    char c[0x14];
    int a, b;
    int lo, hi, i, y;
    char *list, *e, *p;

    switch (D_00809D04) {
    case 1:
        ehsys_C644C679(c);
        switch (D_0080A490) {
        case 0:
            ehsys_942B03D0(0xC, 0xC);
            y = 3 - ((D_00009CA4 >> 2) & 3);
            ehsys_C87BAB32((3 - D_0080AB94) * 10 + 0x38, 0x28 - y, &D_00009AAC);
            ehsys_C87BAB32((3 - D_0080AB94) * 10 + 0x38, y + 0x3D, &D_00009AB0);
            if ((D_00009CA4 >> 3) & 1) {
                ehsys_C87BAB32(8, 0x32, &D_00009AB4);
            }
            break;
        case 1:
            ehsys_942B03D0(0xC, 0xC);
            if ((D_00009CA4 >> 3) & 1) {
                ehsys_C87BAB32(8, 0x58, &D_00009AB4);
            }
            break;
        }
        ehsys_942B03D0(0xE, 0xE);
        ehsys_C87BAB32(0x1C, 0x32, &D_00009AB8);
        ehsys_C87BAB32(0x38, 0x32, &D_00009ABC, D_0080AB98);
        if (ehsys_28F7224B(D_0080AB98)) {
            if (ehsys_get_language() == 0) {
                lo = 1;
                hi = 0;
                ehsys_33ED567B(lo);
                ehsys_D853F15C(hi);
            } else {
                lo = 0;
                hi = 1;
                ehsys_33ED567B(lo);
                ehsys_D853F15C(hi);
            }
            ehsys_C792FA38(lo);
            ehsys_0941BAC6(hi);
            ehsys_942B03D0(0x10, 0x10);
            func_0000289C(0x1C, 0x50, ehsys_AD0E1E03(D_0080AB98));
            ehsys_C792FA38(0);
            ehsys_0941BAC6(0);
        } else {
            ehsys_C87BAB32(0x1C, 0x58, &D_00009AC4);
        }
        break;
    case 3:
        ehsys_C644C679(c);
        if ((D_00009CA4 >> 3) & 1) {
            ehsys_942B03D0(0xE, 0xE);
            if (ehsys_get_language() == 0) {
                lo = 1;
                hi = 0;
                ehsys_33ED567B(lo);
                ehsys_D853F15C(hi);
            } else {
                lo = 0;
                hi = 1;
                ehsys_33ED567B(lo);
                ehsys_D853F15C(hi);
            }
            ehsys_C792FA38(lo);
            ehsys_0941BAC6(hi);
            ehsys_942B03D0(0x10, 0x10);
            func_0000289C(0x1C, 0x32, ehsys_AD0E1E03(D_0080AB98));
            ehsys_C792FA38(0);
            ehsys_0941BAC6(0);
        }
        ehsys_942B03D0(0xE, 0xE);
        ehsys_C87BAB32(0x18, 0x60, &D_00009AD4);
        list = D_0080A108;
        for (i = 0; i < 9; i++) {
            e = list + i * 0x64;
            p = e + 4;
            if (*(int *)(e + 4) != 0) {
                func_0000289C(0x3C, (i + 9) * 0xC, p + 0x10);
            } else {
                ehsys_C87BAB32(0x3C, (i + 9) * 0xC, &D_00009AE8);
            }
        }
        ehsys_D2B87B2C(&a, &b);
        break;
    }
}

/* func_0000289C - second copy of func_00000BE4 (different source file).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_0000289C(int arg0, int arg1, void *arg2) {
    ehsys_B89D38DC(arg0 << 6, arg1 << 6, (0x1E0 - arg0) << 6, arg2);
}

/* func_000028BC - matching test init: clear the 0xE90-byte state block.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000028BC(void) {
    ehsys_memset(&D_0080AB9C, 0, 0xE90);
    ehsys_FC53D253();
}

/* func_000028EC - matching test: edit the rules of the group being created (solo/tag,
 * single/match, life points, time limit); START creates the group, X
 * leaves. MATCHING NOTE: the rule record must be accessed through a struct
 * pointer (DuelRule) so that strcpy gets `p->name` from the cached base
 * register instead of a folded constant address.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000028EC(void) {
    PadState *pad = ehsys_F5E3080C();
    DuelRule *p;

    D_0080AB9C = 2;
    if (pad->rep & PAD_CROSS) {
        D_0080B32C = 0;
        return;
    }
    if (pad->rep & PAD_START) {
        D_0080B32C = 0;
        ehsys_D49B6BDD(&D_0080B330);
        return;
    }
    if (pad->rep & PAD_UP) {
        if (D_0080B328 != 0) {
            D_0080B328 = D_0080B328 - 1;
        }
    }
    if (pad->rep & PAD_DOWN) {
        if (D_0080B328 < 4) {
            D_0080B328 = D_0080B328 + 1;
        }
    }
    p = (DuelRule *)D_0080B374;
    ehsys_strcpy(p->name, &D_00009AF0);
    if (pad->rep & (PAD_LEFT | PAD_RIGHT)) {
        switch (D_0080B328) {
        case 0:
            break;
        case 1:
            if (p->b[1] == 1) {
                p->b[1] = 2;
            } else {
                p->b[1] = 1;
            }
            break;
        case 2:
            if (p->b[2] == 1) {
                p->b[2] = 2;
            } else {
                p->b[2] = 1;
            }
            break;
        case 3:
            p->b[3]++;
            if (p->b[3] >= 6) {
                p->b[3] = 0;
            }
            break;
        case 4:
            p->b[4]++;
            if (p->b[4] >= 5) {
                p->b[4] = 0;
            }
            break;
        }
    }
}

/* func_00002A88 - matching test state 1: group list. Triangle opens the rule editor
 * (func_000028EC), up/down pick a group, O asks to join it, X leaves.
 * MATCHING NOTE: the O branch ends in `return;` and the X test is a
 * separate `if`, not `else if`. That makes the X-test block a join in
 * MWCC's CFG, which is what stops it being stolen into a branch-likely
 * delay slot (the target's unfilled `beqz`/`nop` pairs and the dead `andi`
 * left behind at 0x2C30). Join blocks are never stolen; single-predecessor
 * blocks are - see also func_00003A28.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002A88(void) {
    char buf[0x64];
    PadState *pad = ehsys_F5E3080C();
    char *list;

    if (pad->trg & PAD_TRIANGLE) {
        D_0080B32C = 1;
        ehsys_memset(&D_0080B330, 0, 0x6C);
        D_0080B375 = 1;
        D_0080B376 = 1;
        D_0080B377 = 0;
        D_0080B378 = 0;
        ehsys_3A7E4F65(&D_0080B334, ehsys_3C1C1F65(), 0xF);
    } else if (D_0080B32C) {
        func_000028EC();
    } else {
        D_0080B39C = 0;
        D_0080AB9C = 1;
        if (pad->rep & PAD_UP) {
            if (D_0080ABA0 != 0) {
                D_0080ABA0 = D_0080ABA0 - 1;
            }
        }
        if (pad->rep & PAD_DOWN) {
            if (D_0080ABA0 < 8) {
                D_0080ABA0 = D_0080ABA0 + 1;
            }
        }
        list = D_0080ABD0;
        ehsys_BCEB40ED(list);
        if (pad->trg & PAD_CIRCLE) {
            if (*(int *)(list + D_0080ABA0 * 0x6C + 4)) {
                ehsys_3A7E4F65(buf + 0x10, ehsys_3C1C1F65(), 0xF);
                ehsys_strcpy(buf + 0x42, &D_00009B00);
                ehsys_FF141CC8(D_0080ABA0, buf);
            }
            return;
        }
        if (pad->trg & PAD_CROSS) {
            ehsys_67953A3E(0x21);
        }
    }
}

/* func_00002C58 - matching test state 3: member list of the joined group.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002C58(void) {
    PadState *pad = ehsys_F5E3080C();

    D_0080AB9C = 3;
    if ((pad->rep & PAD_UP) && D_0080ABA4 != 0) {
        D_0080ABA4 = D_0080ABA4 - 1;
        return;
    }
    if ((pad->rep & PAD_DOWN) && D_0080ABA4 < 8) {
        D_0080ABA4 = D_0080ABA4 + 1;
        return;
    }
    ehsys_93578062(D_0080AFA0);
    if (pad->trg & PAD_CIRCLE) {
        ehsys_C978255D(D_0080ABA4);
    } else if (pad->trg & PAD_TRIANGLE) {
        ehsys_6DBE26D9();
    } else if (pad->trg & PAD_CROSS) {
        ehsys_67953A3E(0x13);
        D_0080AB9C = 0;
    }
}

/* func_00002D44 - matching test state 4: waiting for the host.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00002D44(void) {
    PadState *pad = ehsys_F5E3080C();

    D_0080AB9C = 4;
    ehsys_EF9EFF0F(D_0080AFA0);
    if (pad->trg & PAD_CROSS) {
        ehsys_8852C022();
    }
}

/* func_00002D98 - matching test state 7: the multiplayer handshake, stepped by
 * D_0080B39C: 0 hello with the player name, 1-4 wait for peers and pick a
 * deck, 5-6 exchange decks, 7-8 start message and an 8-frame countdown,
 * 0xB fill the duel setup record for up to 4 players and return 1.
 * MATCHING NOTES: `msg = D_0080B3A4;` once before the switch (kept in $s1);
 * failure paths `break`, not `return 0`; a single `return 1`.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00002D98(void) {
    PadState *pad = ehsys_F5E3080C();
    unsigned char *msg;
    unsigned char info[0x6C];
    char *heap;
    unsigned char *game;
    char *rec;
    char *slot;
    int i, j;

    D_0080AB9C = 7;
    msg = D_0080B3A4;
    switch (D_0080B39C) {
    case 0:
        D_0080ABCC = 0;
        ehsys_memset(D_0080B5A4, 0, 0x180);
        ehsys_memset(D_0080B664, 0, 0x180);
        ehsys_memset(msg, 0, 0x200);
        msg[0] = 1;
        msg[2] = 1;
        msg[3] = 0;
        ehsys_3A7E4F65(msg + 4, ehsys_3C1C1F65(), 0xF);
        ehsys_CA477B34(msg, 0x200);
        D_0080B39C = D_0080B39C + 1;
        break;
    case 1:
        if (ehsys_3F68D487(1, 4, 0xFF) == 0) {
            break;
        }
        for (i = 0; i < ehsys_F59F3BD3(); i++) {
            char *r = ehsys_82249188(i);
            ehsys_3A7E4F65((char *)&D_0080AB9C + i * 0x30 + 0xA08, r + 4, 0xF);
        }
        msg[2] = 2;
        ehsys_CA477B34(msg, 0x200);
        D_0080B39C = D_0080B39C + 1;
        break;
    case 2:
        if (ehsys_BA0DA4DE() == 0) {
            if (ehsys_3F68D487(1, 2, 0xFF) == 0) {
                break;
            }
            msg[0] = 2;
            msg[2] = 1;
            ehsys_CA477B34(msg, 0x200);
            D_0080B39C = D_0080B39C + 1;
        } else if (*(signed char *)ehsys_82249188(0) == 2) {
            msg[0] = 2;
            msg[2] = 1;
            ehsys_CA477B34(msg, 0x200);
            D_0080B39C = D_0080B39C + 1;
        }
        break;
    case 3:
        if ((pad->rep & PAD_LEFT) && D_0080ABCC > 0) {
            D_0080ABCC = D_0080ABCC - 1;
        } else if ((pad->rep & PAD_RIGHT) && D_0080ABCC < 0x195) {
            D_0080ABCC = D_0080ABCC + 1;
        }
        if (ehsys_173C81DF() == 2) {
            D_0080B39C = D_0080B39C + 1;
        } else {
            ehsys_3A7E4F65(msg + 4, ehsys_8CBC1795(D_0080ABCC), 0x18);
            msg[2] = 2;
            ehsys_CA477B34(msg, 0x200);
        }
        break;
    case 4:
        if (ehsys_BA0DA4DE() == 0) {
            if (ehsys_3F68D487(2, 2, 0xFF) && (pad->trg & PAD_CIRCLE)) {
                msg[3] = 1;
                ehsys_CA477B34(msg, 0x200);
                D_0080B39C = D_0080B39C + 1;
                break;
            }
        } else if (ehsys_958F298F() == 1) {
            msg[3] = 1;
            ehsys_CA477B34(msg, 0x200);
            D_0080B39C = D_0080B39C + 1;
            break;
        }
        if (ehsys_173C81DF() == 1) {
            D_0080B39C = D_0080B39C - 1;
        } else if (pad->rep & PAD_CROSS) {
            msg[2] = 1;
            ehsys_CA477B34(msg, 0x200);
        } else if (pad->rep & PAD_TRIANGLE) {
            ehsys_71B237CA(0);
        }
        break;
    case 5:
        D_0080BA24 = ehsys_41AABF28(D_00009CE4, 0x100);
        D_0080BA28 = ehsys_41AABF28(D_00009CE4, 0x400);
        ehsys_7F9073E5(D_0080ABCC, D_0080BA24);
        ehsys_892F5B43(D_0080BA28, D_0080BA24, 0x100);
        D_0080B39C = D_0080B39C + 1;
        break;
    case 6:
        if (ehsys_70563675()) {
            for (i = 0; i < ehsys_F59F3BD3(); i++) {
                ehsys_memcpy((char *)&D_0080AB9C + i * 0xC0 + 0xB88, (char *)D_0080BA28 + (i << 8), 0xC0);
            }
            ehsys_20E340D9(D_00009CE4, D_0080BA24);
            ehsys_20E340D9(D_00009CE4, D_0080BA28);
            D_0080B39C = D_0080B39C + 1;
        }
        break;
    case 7:
        ehsys_memset(msg, 0, 0x200);
        msg[0] = 0xD;
        msg[2] = 1;
        msg[3] = 0;
        ehsys_CA477B34(msg, 0x200);
        D_0080B39C = D_0080B39C + 1;
        D_0080B3A0 = 8;
        break;
    case 8:
        if (D_0080B3A0 != 0) {
            D_0080B3A0 = D_0080B3A0 - 1;
        } else {
            D_0080B39C = 0xB;
        }
        break;
    case 0xB:
        heap = ehsys_D8A0DB10();
        game = ehsys_88B6C7F0();
        game[0xD4] = 1;
        if (ehsys_BA0DA4DE() == 0) {
            game[0xD6] = D_0080B375;
            game[0xD5] = D_0080B376;
            game[0xDA] = D_0080B377;
            game[0xDB] = D_0080B378;
        } else {
            ehsys_3EC38F85(info);
            func_000035C0((S28 *)&D_0080B374, (S28 *)(info + 0x44));
            game[0xD6] = D_0080B375;
            game[0xD5] = D_0080B376;
            game[0xDA] = D_0080B377;
            game[0xDB] = D_0080B378;
        }
        game[0xD7] = 0;
        game[0xD8] = 2;
        game[0xD9] = 2;
        for (i = 0; i < ehsys_F59F3BD3(); i++) {
            rec = (char *)game + i * 0x1F8 + 0xE8;
            slot = (char *)&D_0080AB9C + i * 0x30;
            ehsys_memcpy(rec + 0x20, slot + 0xA08, 0x32);
            ehsys_memcpy(rec + 0x52, slot + 0xAC8, 0x80);
            ehsys_memcpy(rec + 0xD4, (char *)&D_0080AB9C + i * 0xC0 + 0xB88, 0xC0);
            *(short *)(rec + 0x2) = 0;
            *(short *)(rec + 0x8) = 0x196;
            *(short *)(rec + 0xA) = 0;
            *(short *)(rec + 0xC) = i;
            *(short *)(rec + 0x4) = 1;
            *(short *)(rec + 0x6) = i;
            if (i == ehsys_BA0DA4DE()) {
                *(short *)(rec + 0x0) = 1;
                if (D_0080ABCC == 0x199) {
                    ehsys_EB6FAACF(0, game);
                    ehsys_memcpy(game + 0xC0, heap + 0x2BA78, 10);
                } else {
                    ehsys_memcpy(game, rec + 0xD4, 0xC0);
                    for (j = 0; j < 5; j++) {
                        *(short *)(game + j * 2 + 0xC0) = 4;
                    }
                }
            } else {
                *(short *)(rec + 0x0) = 0;
            }
        }
        if (game[0xD6] == 2) {
            for (i = ehsys_F59F3BD3(); i < 4; i++) {
                rec = (char *)game + i * 0x1F8 + 0xE8;
                if (i == 2) {
                    *(short *)(rec + 0x0) = 2;
                    *(short *)(rec + 0xC) = 0;
                } else {
                    *(short *)(rec + 0x0) = 2;
                    *(short *)(rec + 0xC) = 1;
                }
                *(short *)(rec + 0x2) = 0;
                *(short *)(rec + 0x8) = 2;
                *(short *)(rec + 0xA) = 0;
                ehsys_3A7E4F65(rec + 0x52, ehsys_8CBC1795(*(unsigned short *)(rec + 0x8)), 0x40);
                ehsys_7F9073E5(*(unsigned short *)(rec + 0x8), rec + 0xD4);
            }
        }
        return 1;
    }
    return 0;
}

/* func_000035C0 - copy a peer record (6 bytes, a 32-byte name struct, a halfword).
 * The byte fields are separate members and the name is a nested struct:
 * that is what produces the target's 6 unrolled lbu/sb plus a 16 x 2-byte
 * struct-copy loop.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
S28 *func_000035C0(S28 *dst, S28 *src) {
    dst->b0 = src->b0;
    dst->b1 = src->b1;
    dst->b2 = src->b2;
    dst->b3 = src->b3;
    dst->b4 = src->b4;
    dst->b5 = src->b5;
    dst->name = src->name;
    dst->v = src->v;
    return dst;
}

/* func_0000362C - matching test update: dispatch on the ad-hoc session state. Cases
 * are in source order -1, 0, 0xB, 0x1E, 0x15, 0xE-0x11, 0x23, 0x1F, 5-9;
 * the target tests them in exactly the reverse order.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_0000362C(void) {
    PadState *pad = ehsys_F5E3080C();

    switch (ehsys_0D3E34B5()) {
    case -1:
        ehsys_A94DF30B();
        return 2;
    case 0:
        ehsys_67953A3E(4);
        break;
    case 0xB:
        func_00002A88();
        break;
    case 0x1E:
        func_00002C58();
        break;
    case 0x15:
        func_00002D44();
        break;
    case 0xE:
    case 0xF:
    case 0x10:
    case 0x11:
        D_0080AB9C = 5;
        if (pad->trg & PAD_CIRCLE) {
            ehsys_67953A3E(0xB);
        }
        break;
    case 0x23:
        D_0080AB9C = 6;
        if (pad->trg & PAD_CROSS) {
            ehsys_67953A3E(0x13);
            D_0080AB9C = 0;
        }
        break;
    case 0x1F:
        return func_00002D98();
    case 5:
    case 7:
    case 8:
    case 9:
        D_0080AB9C = 0;
        break;
    }
    return 0;
}

/* func_000037D0 - format the five rule fields of a group record into a local buffer
 * ("UNKNOWN", "SOLO"/"TAG", "SINGLE"/"MATCH", "8000".."16000",
 * "NO LIMIT".."300sec"). The buffer is never used - the debug print that
 * consumed it was compiled out.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000037D0(void *arg0, int unused) {
    char buf[0x40];
    unsigned char *rule = arg0;
    int i;

    for (i = 0; i < 5; i++) {
        ehsys_strcpy(buf, &D_00009B14);
        switch (i) {
        case 0:
            ehsys_strcpy(buf, &D_00009B1C);
            break;
        case 1:
            switch (rule[0x45]) {
            case 1:
                ehsys_strcpy(buf, &D_00009B20);
                break;
            case 2:
                ehsys_strcpy(buf, &D_00009B28);
                break;
            }
            break;
        case 2:
            switch (rule[0x46]) {
            case 1:
                ehsys_strcpy(buf, &D_00009B2C);
                break;
            case 2:
                ehsys_strcpy(buf, &D_00009B34);
                break;
            }
            break;
        case 3:
            switch (rule[0x47]) {
            case 0:
                ehsys_strcpy(buf, &D_00009B3C);
                break;
            case 1:
                ehsys_strcpy(buf, &D_00009B44);
                break;
            case 2:
                ehsys_strcpy(buf, &D_00009B4C);
                break;
            case 3:
                ehsys_strcpy(buf, &D_00009B54);
                break;
            case 4:
                ehsys_strcpy(buf, &D_00009B5C);
                break;
            }
            break;
        case 4:
            switch (rule[0x48]) {
            case 0:
                ehsys_strcpy(buf, &D_00009B64);
                break;
            case 1:
                ehsys_strcpy(buf, &D_00009B70);
                break;
            case 2:
                ehsys_strcpy(buf, &D_00009B78);
                break;
            case 3:
                ehsys_strcpy(buf, &D_00009B80);
                break;
            case 4:
                ehsys_strcpy(buf, &D_00009B88);
                break;
            }
            break;
        }
    }
}

/* func_00003A28 - matching test draw, state 1: group list (9 rows).
 * MATCHING NOTE: the list base must be a local (`list = D_0080ABD0;`) and
 * the non-empty case written first.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003A28(void) {
    int i, y;
    char *list;
    char *e, *p;

    ehsys_C644C679(&D_0080BA2C);
    if ((D_00009CA4 >> 3) & 1) {
        ehsys_C87BAB32(0xC, (D_0080ABA0 + 4) * 0xC, &D_00009B90);
    }
    list = D_0080ABD0;
    for (i = 0; i < 9; i++) {
        e = list + i * 0x6C;
        p = e + 4;
        if (*(int *)(e + 4) != 0) {
            y = (i + 4) * 0xC;
            func_00003B64(0x18, y, p + 4);
            ehsys_C87BAB32(0xB4, y, &D_00009B94, ((unsigned char *)p)[0x44], ((unsigned char *)p)[0x45], ((unsigned char *)p)[0x46], ((unsigned char *)p)[0x47], ((unsigned char *)p)[0x48], p + 0x4A, *(int *)(p + 0x3C));
        } else {
            ehsys_C87BAB32(0x18, (i + 4) * 0xC, &D_00009BB4);
        }
    }
}

/* func_00003B64 - third copy of func_00000BE4 (different source file).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003B64(int arg0, int arg1, void *arg2) {
    ehsys_B89D38DC(arg0 << 6, arg1 << 6, (0x1E0 - arg0) << 6, arg2);
}

/* func_00003B84 - matching test draw, state 2: rule editor.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003B84(void) {
    ehsys_C644C679(&D_0080BA2C);
    func_000037D0(&D_0080B330, 5);
    func_000037D0(&D_0080B330, 5);
}

/* func_00003BC4 - matching test draw, state 3: member list (same notes as func_00003A28).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003BC4(void) {
    int i, y;
    char *list;
    char *e, *p;

    ehsys_C644C679(&D_0080BA2C);
    func_000037D0(&D_0080B330, 5);
    if ((D_00009CA4 >> 3) & 1) {
        ehsys_C87BAB32(0xC, (D_0080ABA4 + 7) * 0xC, &D_00009B90);
    }
    list = D_0080AFA0;
    for (i = 0; i < 9; i++) {
        e = list + i * 0x64;
        p = e + 4;
        if (*(int *)(e + 4) != 0) {
            if (*(int *)(p + 4)) {
                ehsys_C87BAB32(0x24, (i + 7) * 0xC, &D_00009BC0, *(int *)(p + 8) + 1);
            }
            y = (i + 7) * 0xC;
            func_00003B64(0x3C, y, p + 0x10);
            ehsys_C87BAB32(0xB4, y, p + 0x42);
        } else {
            ehsys_C87BAB32(0x3C, (i + 7) * 0xC, &D_00009BC8);
        }
    }
}

/* func_00003D1C - matching test draw, state 4: own join request.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003D1C(void) {
    unsigned char info[0x6C];

    ehsys_C644C679(&D_0080BA2C);
    func_000037D0(&D_0080B330, 5);
    ehsys_3EC38F85(info);
    func_00003B64(0x78, 0x6C, info + 4);
    ehsys_C87BAB32(0xF0, 0x6C, &D_00009BD4, info[0x44], info[0x45], info[0x46], info[0x47], info[0x48], info + 0x4A);
    ehsys_C87BAB32(0x78, 0x7A, &D_00009BEC);
}

/* func_00003DA8 - matching test draw, state 7: handshake / peer list.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003DA8(void) {
    int i, y;
    char *slot, *r;

    ehsys_C644C679(&D_0080BA2C);
    for (i = 0; i < ehsys_F59F3BD3(); i++) {
        ehsys_82249188(i);
    }
    switch (D_0080B39C) {
    case 3:
        ehsys_C87BAB32(0xC, 0xC0, &D_00009C08);
        func_00003B64(0xC, 0xCC, ehsys_8CBC1795(D_0080ABCC));
    case 4:
    case 5:
        for (i = 0; i < ehsys_F59F3BD3(); i++) {
            y = (i + 7) * 0xC;
            ehsys_C87BAB32(0xC, y, &D_00009C14, i + 1);
            slot = (char *)&D_0080AB9C + i * 0x30;
            func_00003B64(0x24, y, slot + 0xA08);
            r = ehsys_82249188(i);
            if (((signed char *)r)[2] == 2) {
                char *name = slot + 0xAC8;
                ehsys_3A7E4F65(name, r + 4, 0x18);
                func_00003B64(0x9C, y, name);
            } else if ((D_00009CA4 >> 3) & 1) {
                ehsys_C87BAB32(0x9C, y, &D_00009C1C);
            }
        }
        break;
    }
}

/* func_00003F40 - matching test draw: dispatch on the screen state D_0080AB9C
 * (jump table jtbl_00009674). States 5 and 6 print the Japanese status
 * messages "Could not join." / "Could not confirm the duel members." and
 * "Disbanding the group.".
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00003F40(void) {
    switch (D_0080AB9C) {
    case 0:
        break;
    case 1:
        func_00003A28();
        break;
    case 2:
        func_00003B84();
        break;
    case 3:
        func_00003BC4();
        break;
    case 4:
        func_00003D1C();
        break;
    case 5:
        ehsys_C87BAB32(0x84, 0x7A, &D_00009C28);
        break;
    case 6:
        ehsys_C87BAB32(0x84, 0x7A, &D_00009C40);
        ehsys_C87BAB32(0x84, 0x86, &D_00009C68);
        break;
    case 7:
        func_00003DA8();
        break;
    }
}

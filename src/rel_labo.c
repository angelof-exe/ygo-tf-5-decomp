/*
 * rel_labo.prx — reconstructed code (matching decompilation)
 *
 * The "Labo" hub: a debug-style menu ("Labo Debug Menu") whose 12 entries
 * launch other gmodules (password machine, card list/rental/trade, card
 * convert machine, deck swap, card album, duelist list, duel ranking,
 * forbidden/limited list, tutorial, partner select) plus an in-module
 * "Voice & Subtitle" test screen that plays character voice lines and shows
 * their subtitle text.
 *
 * Compiler:     Metrowerks CodeWarrior for PSP — MWCC 1.3 SP7 (mwccpsp_3.0.1_219)
 * Flags:        -O4,s -sdatathreshold 0
 * Verification: scripts/mwcc_build.sh + scripts/mwcc_diff.py (docs/06), and
 *               `make MODULE=rel_labo SRC=1`, which prints OK: the relinked
 *               module is byte-identical to the shipped one (sha1 1a0b57a3...).
 *
 * STATUS: 14/14 functions byte-identical. 12 of them are clean in
 * mwcc_diff.py; the other two show ONLY relocation-NAME differences that the
 * tool cannot see through (the bytes are proven identical by SRC=1):
 *  - func_00000034: its `switch` compiles to a jump table. In our object the
 *    table is an anonymous compiler symbol (`@NNN` in .rodata); the target asm
 *    calls it jtbl_000067E4. The Makefile places our .rodata at 0x67E4 as
 *    NOLOAD, and all 15 table entries were checked by hand against the shipped
 *    .data (0x94, 0xCC, 0x260, ... 0x92C — each = our case offset + 0x34).
 *  - func_00000BC8: the voice-test state is a STRUCT (see VoiceTest below),
 *    so `D_0000AE18.val[...]` relocates against D_0000AE18 with addend 4,
 *    where splat's asm names the same address D_0000AE1C. Same address, same
 *    bytes; the struct is required, not cosmetic (see the function comment).
 *
 * Imports are named (docs/nids/README.md). Calls into other gmodules use the
 * module-qualified names from config/symbols/rel_labo.txt (password_*,
 * select_card_*, conv_machine_*, deck_swap_*, cardalbum_*, charalist_*,
 * duelrecord_*, limitlist_*, tutoriallist_*).
 *
 * Data: every string and table this code touches lives in the shipped .data
 * blob, which is linked verbatim, so they are `extern` here and NOT string
 * literals (a literal would land in our object and shift the layout). Several
 * of them are Shift-JIS; their translations are in comments only. Proposed
 * names for the globals are in config/symbols/rel_labo.extra.txt; this file
 * keeps the D_XXXXXXXX spellings until rel_labo.txt is regenerated and the
 * module re-split, because mwcc_diff.py compares relocation symbol names.
 *
 * MWCC levers that mattered here (in addition to docs/06 §3c):
 *  - struct vs separate globals changes ALIAS ANALYSIS, not just spelling.
 *    With `extern int sel; extern int val[];` MWCC proves a store to
 *    val[sel] cannot change sel and keeps sel in a register. With both in
 *    one struct object, the store may alias .sel, so sel and the element are
 *    reloaded after every store — exactly the target's shape in
 *    func_00000BC8 (+11 words). A cast `((VoiceTest *) &x)->...` is NOT
 *    equivalent: it produced a third, wrong forwarding shape.
 *  - whether the "return 0" of an early block is a `return` or a fall-through
 *    into the shared tail decides whether a LATER branch to that tail gets a
 *    branch-likely fill (func_00000BC8: `bgez` vs `bgezl`). MWCC fills the
 *    first eligible branch to a shared block and not the following ones.
 *  - declaration order of locals is visible: of two stack arrays the one
 *    declared LATER gets the lower address (func_00000F58), and swapping two
 *    `int` declarations swaps their $s0/$s1 assignment (func_00000BC8).
 *    func_00000F58's two out-params also had to be `int`, read back through
 *    `(unsigned short)` casts: as `unsigned short` locals MWCC packs them at
 *    the top of the frame instead of below the buffers.
 *  - `if (a == 0 || b == 0) return;` vs two separate ifs: different block
 *    layout (func_00000F58, +3 words).
 *  - loading a global into a named local BEFORE a call pins the load before
 *    the call (func_000012A4); written inline in the return expression MWCC
 *    sinks the load after the call.
 */

typedef struct Pad {
    int unk0;
    int buttons;      /* +0x04: held/pressed mask tested against button codes */
    int unk8;
    int trigger;      /* +0x0C: edge-triggered mask (0x10 up, 0x20 right, 0x40 down, 0x80 left, 0x100 L, 0x200 R) */
    int unk10[5];
    int repeat;       /* +0x24: auto-repeat mask (0x10 up, 0x40 down) */
} Pad;

extern void ehsys_B4471B5E(void (*)(void), void (*)(void), void *);
extern void ehsys_13A4081A(int);
extern void ehsys_C2E59A21(int, void (*)(void), void (*)(void));
extern void ehsys_D470D0B2(int, int, int);
extern void ehsys_5998BFD7(int, int, int);
extern void ehsys_memset(void *, int, int);
extern void ehsys_memcpy(void *, void *, int);
extern void ehsys_frame_sync(void);
extern Pad *ehsys_F5E3080C(void);
extern int ehsys_get_button_code(int);
extern void ehsys_597CDADD(int, int, void *, int, int);
extern int ehsys_4E8B47D4(void);
extern void ehsys_D767C29F(int);
extern int ehsys_450CD6C5(void);
extern void ehsys_08813E19(int, int);
extern void *ehsys_88B6C7F0(void);
extern char *ehsys_D8A0DB10(void);
extern void ehsys_EB6FAACF(int, void *);
extern int ehsys_A4B2A31B(void);
extern int ehsys_DFCA450B(int, int);
extern int ehsys_1D98951B(int, int);
extern int ehsys_42110BF9(int, int);
extern int ehsys_399AC68A(int, int);
extern int ehsys_C62F9EAC(int, int);
extern void ehsys_1EC5342B(int);
extern void ehsys_4878E980(int);
extern void ehsys_942B03D0(int, int);
extern void ehsys_60B55A50(int);
extern void ehsys_C87BAB32(int, int, void *);
extern unsigned int *ehsys_B2B9FEFA(void);
extern void ehsys_878EE1D3(int *);
extern int ehsys_D2A768F4(int, void *);
extern void ehsys_BF3E9066(int, void *, void *, int);
extern int ehsys_8798050C(int);
extern void ehsys_74CA1190(void);
extern void ehsys_C859D5FE(int);
extern int ehsys_AA9913C7(int);
extern void ehsys_80E7F36C(int);
extern int ehsys_99FF9D51(int, int);
extern int ehsys_7DAC692D(int, int, int);
extern void ehsys_B89D38DC(int, int, int, int);
extern int ehsys_916286F3(int);
extern void ehsys_strcpy(void *, void *);
extern void ehsys_strcat(void *, void *);
extern int ehsys_strlen(void *);
extern void *ehsys_BAB403B8(int, int);
extern int ehsys_DF62D899(int, int);
extern int ehsys_99F81E55(int);
extern int ehsys_sprintf(char *, const char *, ...);
extern int ehsys_C6C15111(void *, char *, int);
extern int ehsys_get_language(void);
extern void ehsys_1B4AF0F5(int, int, int, int *, int *);
extern int ehsys_E167E1C0(int, int, int, int);
extern void ehsys_C792FA38(int);
extern int ehsys_E58C0FDC(int, int);
extern void ehsys_3BB2BAC6(int, int, int, int, int, int, int);
extern int ehsys_82B715FD(int);

extern void password_932AE9C8(void);
extern int password_516C75F9(void);
extern void select_card_BF2B46D7(int, int, int, int);
extern int select_card_5B395307(void);
extern void conv_machine_0BB25540(void);
extern int conv_machine_CD9BCE9B(void);
extern void deck_swap_02A2E785(void *, int, void *, int);
extern int deck_swap_C5289073(void);
extern void cardalbum_EE554159(int, int);
extern int cardalbum_43AB5667(void);
extern void charalist_0B63A183(int, int, int);
extern int charalist_FD65B042(void);
extern void duelrecord_6ADB95DB(int);
extern int duelrecord_420F9405(void);
extern void limitlist_E69A45D2(int, int);
extern int limitlist_56E1CA7A(void);
extern void tutoriallist_41015B0F(int, int);
extern int tutoriallist_CF2830FF(void);

/* voice-test state at 0xAE18, 0x1C bytes (cleared as a whole by
 * func_00000B58). Being ONE object is what the target's codegen in
 * func_00000BC8 requires; see that function. */
typedef struct VoiceTest {
    int sel;     /* +0x00: selected row 0..2 */
    int val[3];  /* +0x04: character id, sub id, voice index */
    int unk10;
    int unk14;
    void *work;  /* +0x18: = D_0000AE40, the voicetest archive buffer */
} VoiceTest;

/* .data */
extern int D_00006820[];            /* [12] module id per menu entry, loaded via ehsys_597CDADD */
extern void (*D_00006850[])(void);  /* [15] draw routine per hub state, NULL = clear only */
extern char D_0000688C[];           /* 12 menu labels, 0x40 bytes each */
extern char D_00006B8C[];           /* "Labo Debug Menu" */
extern char D_00006B9C[];           /* SJIS "\x81\x9f" = "◆" menu cursor */
extern char D_00006BA0[];           /* "labo/voicetest.ehp" */
extern char *D_00006BE4;            /* -> "ChrID : %03d - " */
extern char *D_00006BE8;            /* -> "SubID : %1d" */
extern char *D_00006BEC;            /* -> "VOICE : %03d : %s" */
extern char *D_0000A868[];          /* [318] ID_VCM_* enum names + SJIS comments */
extern char D_0000AD60[];           /* "jegfis": language suffix by ehsys_get_language() */
extern char D_0000AD68[];           /* "chr" */
extern char D_0000AD6C[];           /* "chrzakov%2d" */
extern char D_0000AD78[];           /* "%s_voiceidchgtbl.bin" */
extern char D_0000AD90[];           /* "%s_wordstbl_%c.bin" */
extern char D_0000ADA4[];           /* SJIS "台詞ID : %d" = "line ID : %d" */
extern char D_0000ADB0[];           /* SJIS "\x81\x9e" = "◇" row cursor */
extern char D_0000ADB4[];           /* SJIS help line: "O: play  X: back  L: -10  R: +10" */
extern char D_0000AE0C[];           /* 3rd arg of the module-registration call */

/* .bss */
/* hub state, 0x1C bytes zeroed by func_00000034 (separate symbols suffice:
 * no function here needs the struct's alias behaviour) */
extern int D_0000ADF0;  /* state, 0..14 */
extern int D_0000ADF4;  /* sub-module phase: 1 = start, 2 = running */
extern int D_0000ADF8;  /* menu cursor, 0..11 */
extern int D_0000ADFC;  /* id of the loaded sub-module (D_00006820[cursor]) */
extern VoiceTest D_0000AE18;
/* the same object's fields under splat's per-address names, used wherever the
 * target relocates against them directly (everything except func_00000BC8) */
extern int D_0000AE1C[];   /* = D_0000AE18.val[0], character id */
extern int D_0000AE20;     /* = D_0000AE18.val[1], sub id */
extern int D_0000AE24;     /* = D_0000AE18.val[2], voice index */
extern void *D_0000AE30;   /* = D_0000AE18.work */
extern char D_0000AE40[];  /* work buffer the voicetest archive is loaded into */

/* forward declarations: the call graph is not in address order */
void func_00000034(void);
void func_0000097C(void);
void func_00000984(void);
void func_00000A0C(void);
void func_00000A14(void);
void func_00000B58(void);
int func_00000BC8(void);
void func_00000EC0(void);
void func_00000F38(int, int, int);
void func_00000F58(void);
void func_00001164(int, int, int, int, int);
void func_00001180(void);
int func_000012A4(void);

/* func_00000000 — module entry point: registers the hub's main loop
 * (func_00000034) and an empty teardown hook (func_0000097C) with the
 * engine's module-registration call.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000000(void) {
    ehsys_B4471B5E(func_00000034, func_0000097C, D_0000AE0C);
    return 0;
}

/* func_00000034 — hub main loop (proposed name: labo_main).
 * Sets up the per-frame draw callback (func_00000984), fades in, and runs a
 * state machine on D_0000ADF0 until the player backs out:
 *   1      menu: cancel button -> fade out, state 14; confirm -> start the
 *          entry under the cursor (D_0000ADF8, 0..11); up/down auto-repeat
 *          (Pad +0x24 bits 0x10/0x40) wrap the cursor modulo 12;
 *   2..12  "run sub-module": cursor + 2. Sub-state D_0000ADF4 == 1 calls the
 *          sub-module's start export, == 2 polls its update export and on a
 *          nonzero return unloads it (ehsys_D767C29F on the id loaded from
 *          D_00006820[cursor]), fades back in and returns to state 1. States
 *          8 and 12 are the same charalist module started in mode 0 (duelist
 *          list) or 3 (partner select);
 *   13     the in-module voice test (func_00000B58 / func_00000BC8);
 *   14     wait for the fade-out, unregister the draw callback, and leave
 *          (ehsys_08813E19(8, 0) — 8 is presumably the next game mode).
 * Entry 11 ("Voice & Subtitle") is special-cased: it needs no module load.
 *
 * The outer dispatch compiles to a 15-entry jump table (0..14, 0 = idle);
 * every inner dispatch is a two-case `switch` (MWCC tests 2 then 1 and lays
 * out case 1 first, which is the target's order). The cursor's modulo is a
 * real `div`/`mfhi` because -O4,s optimizes for size.
 * MATCH 100% — byte-identical (make MODULE=rel_labo SRC=1). mwcc_diff.py
 * reports 2 relocation-name diffs on the jump-table lui/addiu: our object
 * names the table `@NNN` (anonymous, .rodata), the target jtbl_000067E4.
 * All 15 entries were verified by hand; see the file header. */
void func_00000034(void) {
    int arg;
    Pad *pad;
    int mode;

    ehsys_13A4081A(0);
    ehsys_C2E59A21(8, func_00000984, func_00000A0C);
    ehsys_D470D0B2(1, 0xFF000000, 0x10);
    ehsys_memset(&D_0000ADF0, 0, 0x1C);
    D_0000ADF0 = 1;

    while (1) {
        ehsys_frame_sync();
        switch (mode = D_0000ADF0) {
        case 1:
            pad = ehsys_F5E3080C();
            if (pad->buttons & (unsigned short) ehsys_get_button_code(1)) {
                ehsys_5998BFD7(1, 0xFF000000, 0x10);
                D_0000ADF0 = 14;
            } else if (pad->buttons & (unsigned short) ehsys_get_button_code(0)) {
                arg = 1;
                if (D_0000ADF8 == 11) {
                    func_00000B58();
                    D_0000ADF0 = 13;
                    D_0000ADF4 = 2;
                } else {
                    D_0000ADF0 = 0;
                    D_0000ADF4 = 0;
                    ehsys_13A4081A(1);
                    D_0000ADFC = D_00006820[D_0000ADF8];
                    ehsys_597CDADD(D_0000ADFC, 4, &arg, 0, 0);
                    while (!ehsys_4E8B47D4()) {
                        ehsys_frame_sync();
                    }
                    D_0000ADF4 = 1;
                    D_0000ADF0 = D_0000ADF8 + 2;
                }
            } else if (pad->repeat & 0x10) {
                D_0000ADF8 = (D_0000ADF8 + 11) % 12;
            } else if (pad->repeat & 0x40) {
                D_0000ADF8 = (D_0000ADF8 + 1) % 12;
            }
            break;
        case 2:
            switch (D_0000ADF4) {
            case 1:
                password_932AE9C8();
                D_0000ADF4 = 2;
                break;
            case 2:
                if (password_516C75F9()) {
                    ehsys_D767C29F(D_0000ADFC);
                    ehsys_D470D0B2(1, 0xFF000000, 0x10);
                    ehsys_13A4081A(0);
                    D_0000ADF0 = 1;
                    D_0000ADF4 = 0;
                    D_0000ADFC = 0;
                }
                break;
            }
            break;
        case 3:
            switch (D_0000ADF4) {
            case 1:
                select_card_BF2B46D7(0, 0, 0, 0);
                D_0000ADF4 = 2;
                break;
            case 2:
                if (select_card_5B395307()) {
                    ehsys_D767C29F(D_0000ADFC);
                    ehsys_D470D0B2(1, 0xFF000000, 0x10);
                    ehsys_13A4081A(0);
                    D_0000ADF0 = 1;
                    D_0000ADF4 = 0;
                    D_0000ADFC = 0;
                }
                break;
            }
            break;
        case 4:
            switch (D_0000ADF4) {
            case 1:
                select_card_BF2B46D7(2, 0, 0, 0);
                D_0000ADF4 = 2;
                break;
            case 2:
                if (select_card_5B395307()) {
                    ehsys_D767C29F(D_0000ADFC);
                    ehsys_D470D0B2(1, 0xFF000000, 0x10);
                    ehsys_13A4081A(0);
                    D_0000ADF0 = 1;
                    D_0000ADF4 = 0;
                    D_0000ADFC = 0;
                }
                break;
            }
            break;
        case 5:
            switch (D_0000ADF4) {
            case 1:
                conv_machine_0BB25540();
                D_0000ADF4 = 2;
                break;
            case 2:
                if (conv_machine_CD9BCE9B()) {
                    ehsys_D767C29F(D_0000ADFC);
                    ehsys_D470D0B2(1, 0xFF000000, 0x10);
                    ehsys_13A4081A(0);
                    D_0000ADF0 = 1;
                    D_0000ADF4 = 0;
                    D_0000ADFC = 0;
                }
                break;
            }
            break;
        case 6:
            switch (D_0000ADF4) {
            case 1: {
                char *deck = ehsys_88B6C7F0();
                char *save = ehsys_D8A0DB10();
                ehsys_EB6FAACF(0, deck);
                ehsys_memcpy(deck + 0xC0, save + 0x2BA78, 10);
                deck_swap_02A2E785(deck, ehsys_A4B2A31B(), deck + 0xC0, 1);
                D_0000ADF4 = 2;
                break;
            }
            case 2:
                if (deck_swap_C5289073()) {
                    ehsys_D767C29F(D_0000ADFC);
                    ehsys_D470D0B2(1, 0xFF000000, 0x10);
                    ehsys_13A4081A(0);
                    D_0000ADF0 = 1;
                    D_0000ADF4 = 0;
                    D_0000ADFC = 0;
                }
                break;
            }
            break;
        case 7:
            switch (D_0000ADF4) {
            case 1:
                cardalbum_EE554159(0, 0);
                D_0000ADF4 = 2;
                break;
            case 2:
                if (cardalbum_43AB5667()) {
                    ehsys_D767C29F(D_0000ADFC);
                    ehsys_D470D0B2(1, 0xFF000000, 0x10);
                    ehsys_13A4081A(0);
                    D_0000ADF0 = 1;
                    D_0000ADF4 = 0;
                    D_0000ADFC = 0;
                }
                break;
            }
            break;
        case 8:
        case 12:
            switch (D_0000ADF4) {
            case 1:
                if (mode == 8) {
                    charalist_0B63A183(0, 0, -1);
                } else {
                    charalist_0B63A183(3, 0, -1);
                }
                D_0000ADF4 = 2;
                break;
            case 2:
                if (charalist_FD65B042()) {
                    ehsys_D767C29F(D_0000ADFC);
                    ehsys_D470D0B2(1, 0xFF000000, 0x10);
                    ehsys_13A4081A(0);
                    D_0000ADF0 = 1;
                    D_0000ADF4 = 0;
                    D_0000ADFC = 0;
                }
                break;
            }
            break;
        case 9:
            switch (D_0000ADF4) {
            case 1:
                duelrecord_6ADB95DB(0);
                D_0000ADF4 = 2;
                break;
            case 2:
                if (duelrecord_420F9405()) {
                    ehsys_D767C29F(D_0000ADFC);
                    ehsys_D470D0B2(1, 0xFF000000, 0x10);
                    ehsys_13A4081A(0);
                    D_0000ADF0 = 1;
                    D_0000ADF4 = 0;
                    D_0000ADFC = 0;
                }
                break;
            }
            break;
        case 10:
            switch (D_0000ADF4) {
            case 1:
                limitlist_E69A45D2(0, 0);
                D_0000ADF4 = 2;
                break;
            case 2:
                if (limitlist_56E1CA7A()) {
                    ehsys_D767C29F(D_0000ADFC);
                    ehsys_D470D0B2(1, 0xFF000000, 0x10);
                    ehsys_13A4081A(0);
                    D_0000ADF0 = 1;
                    D_0000ADF4 = 0;
                    D_0000ADFC = 0;
                }
                break;
            }
            break;
        case 11:
            switch (D_0000ADF4) {
            case 1:
                tutoriallist_41015B0F(0, 0);
                D_0000ADF4 = 2;
                break;
            case 2:
                if (tutoriallist_CF2830FF()) {
                    ehsys_D767C29F(D_0000ADFC);
                    ehsys_D470D0B2(1, 0xFF000000, 0x10);
                    ehsys_13A4081A(0);
                    D_0000ADF0 = 1;
                    D_0000ADF4 = 0;
                    D_0000ADFC = 0;
                }
                break;
            }
            break;
        case 13:
            switch (D_0000ADF4) {
            case 1:
                func_00000B58();
                D_0000ADF4 = 2;
                break;
            case 2:
                if (func_00000BC8()) {
                    ehsys_D470D0B2(1, 0xFF000000, 0x10);
                    ehsys_13A4081A(0);
                    D_0000ADF0 = 1;
                    D_0000ADF4 = 0;
                    D_0000ADFC = 0;
                }
                break;
            }
            break;
        case 14:
            if (ehsys_450CD6C5()) {
                ehsys_C2E59A21(0, 0, 0);
                ehsys_13A4081A(1);
                ehsys_frame_sync();
                ehsys_08813E19(8, 0);
                return;
            }
            break;
        }
    }
}

/* func_0000097C — empty teardown hook registered by func_00000000.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_0000097C(void) {
}

/* func_00000984 — per-frame draw callback, registered by func_00000034
 * through ehsys_C2E59A21(8, ...). Calls the state's draw routine from the
 * table D_00006850[state]: func_00000A14 (menu) for states 1 and 14,
 * func_00001180 (voice test) for 13, and the sub-modules' own draw exports
 * otherwise. With no routine (state 0) it builds an opaque-black clear via
 * the same ehsys_DFCA450B -> ... -> ehsys_1EC5342B chain func_00000A14 uses.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000984(void) {
    void (*draw)(void) = D_00006850[D_0000ADF0];

    if (draw != 0) {
        draw();
    } else {
        ehsys_1EC5342B(ehsys_C62F9EAC(
            ehsys_399AC68A(ehsys_42110BF9(ehsys_1D98951B(ehsys_DFCA450B(0, 0), 0xFF000000), 0), 0),
            0x17));
    }
}

/* func_00000A0C — empty second callback paired with func_00000984.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000A0C(void) {
}

/* func_00000A14 — draws the hub menu: a 0xFF404000-coloured backdrop, the
 * title "Labo Debug Menu" (D_00006B8C) at (12, 2), the cursor glyph
 * D_00006B9C (Shift-JIS "\x81\x9f" = U+25C6 BLACK DIAMOND) at the selected
 * row when bit 3 of *ehsys_B2B9FEFA() is set (a blink phase, judging by its
 * use), and the 12 0x40-byte labels at D_0000688C, rows 12 px apart from
 * y = 0x18. The bit test compiles to Allegrex `ext` from plain `(x >> 3) & 1`.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000A14(void) {
    int h;
    int i;

    h = ehsys_DFCA450B(0, 0);
    h = ehsys_1D98951B(h, 0xFF404000);
    h = ehsys_42110BF9(h, 0);
    h = ehsys_399AC68A(h, 0);
    h = ehsys_C62F9EAC(h, 0x17);
    ehsys_1EC5342B(h);
    ehsys_4878E980(4);
    ehsys_942B03D0(10, 10);
    ehsys_60B55A50(-1);
    ehsys_C87BAB32(12, 2, D_00006B8C);
    if ((*ehsys_B2B9FEFA() >> 3) & 1) {
        ehsys_C87BAB32(0, D_0000ADF8 * 12 + 0x18, D_00006B9C);
    }
    for (i = 0; i < 12; i++) {
        ehsys_C87BAB32(12, i * 12 + 0x18, D_0000688C + i * 0x40);
    }
    h = ehsys_DFCA450B(4, 0);
    ehsys_878EE1D3(&h);
    ehsys_1EC5342B(h);
}

/* func_00000B58 — voice-test init: clears the VoiceTest state (0x1C
 * bytes), starts on character id 2, points the work buffer at D_0000AE40,
 * and loads "labo/voicetest.ehp" (D_00006BA0): ehsys_D2A768F4 looks like a
 * size/handle query for the archive, ehsys_BF3E9066 the load into the buffer.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000B58(void) {
    ehsys_memset(&D_0000AE18, 0, 0x1C);
    D_0000AE1C[0] = 2;
    D_0000AE30 = D_0000AE40;
    ehsys_BF3E9066(3, D_00006BA0, D_0000AE30, ehsys_D2A768F4(3, D_00006BA0));
}

/* func_00000BC8 — voice-test update; returns 1 to leave the screen.
 * Row 0 = character id (0..0x60), row 1 = sub id / costume (0..3), row 2 =
 * voice line (0..0x13D, i.e. the 318 entries of D_0000A868). Confirm plays
 * the selected line (func_000012A4 -> ehsys_8798050C, then ehsys_C859D5FE
 * and a frame_sync loop until ehsys_AA9913C7 reports it finished; tentative
 * reading: open / play / is-done, then ehsys_80E7F36C releases it).
 * L/R step the value by 10 and clamp, left/right by 1, up/down move the row
 * cursor with wrap — matching the on-screen help string D_0000ADB4:
 * "O: play  X: back  L: -10  R: +10".
 *
 * MATCHING NOTES:
 *  - the state must be accessed as ONE struct object (D_0000AE18.val[...],
 *    D_0000AE18.sel). The target reloads .sel and the element after every
 *    store to val[sel], which is MWCC's may-alias behaviour for two members
 *    of the same object; with separate `sel`/`val[]` globals MWCC proves
 *    they do not alias and the function comes out 11 words short.
 *  - the confirm branch must FALL THROUGH to the shared `return 0` rather
 *    than `return 0` itself: with an early return, MWCC fills the L-button
 *    clamp's `bgez` with a likely-branch (`bgezl` + `move v0,zero`).
 *  - `int w, h;` declared before `Pad *pad;` puts the voice id in $s1 and the
 *    handle in $s0, as in the target.
 * MATCH 100% — byte-identical (make MODULE=rel_labo SRC=1). mwcc_diff.py
 * reports 8 relocation-name diffs: the four `lui/addiu &D_0000AE18.val`
 * pairs relocate against D_0000AE18 (+4) and the target asm calls that
 * address D_0000AE1C. Same address, same bytes. */
int func_00000BC8(void) {
    int limit = 0;
    int w, h;
    Pad *pad;

    switch (D_0000AE18.sel) {
    case 0:
        limit = 0x61;
        break;
    case 1:
        limit = 4;
        break;
    case 2:
        limit = 0x13E;
        break;
    }

    pad = ehsys_F5E3080C();
    if (pad->buttons & (unsigned short) ehsys_get_button_code(0)) {
        w = func_000012A4();
        h = ehsys_8798050C(w);
        ehsys_74CA1190();
        ehsys_C859D5FE(h);
        while (!ehsys_AA9913C7(h)) {
            ehsys_frame_sync();
        }
        ehsys_80E7F36C(w);
    } else if ((pad = ehsys_F5E3080C())->buttons & (unsigned short) ehsys_get_button_code(1)) {
        return 1;
    } else if (ehsys_F5E3080C()->trigger & 0x100) {
        D_0000AE18.val[D_0000AE18.sel] -= 10;
        if (D_0000AE18.val[D_0000AE18.sel] < 0) {
            D_0000AE18.val[D_0000AE18.sel] = 0;
        }
    } else if (ehsys_F5E3080C()->trigger & 0x200) {
        D_0000AE18.val[D_0000AE18.sel] += 10;
        if (D_0000AE18.val[D_0000AE18.sel] >= limit) {
            D_0000AE18.val[D_0000AE18.sel] = limit - 1;
        }
    } else if (ehsys_F5E3080C()->trigger & 0x80) {
        if (D_0000AE18.val[D_0000AE18.sel] > 0) {
            D_0000AE18.val[D_0000AE18.sel]--;
        }
    } else if (ehsys_F5E3080C()->trigger & 0x20) {
        if (D_0000AE18.val[D_0000AE18.sel] < limit - 1) {
            D_0000AE18.val[D_0000AE18.sel]++;
        }
    } else if (ehsys_F5E3080C()->trigger & 0x10) {
        if (D_0000AE18.sel > 0) {
            D_0000AE18.sel--;
        } else {
            D_0000AE18.sel = 2;
        }
    } else if (ehsys_F5E3080C()->trigger & 0x40) {
        if (D_0000AE18.sel < 2) {
            D_0000AE18.sel++;
        } else {
            D_0000AE18.sel = 0;
        }
    }
    return 0;
}

/* func_00000EC0 — voice test: draws the selected character's name
 * (ehsys_7DAC692D(chr, sub, 0)) at (0x7F, 10), right after the
 * "ChrID : %03d - " label, if ehsys_99FF9D51(chr, sub) says the pair exists.
 * Character ids 0 and 1 are skipped. Both ids are read as halfwords (`lhu`),
 * hence the unsigned short locals.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000EC0(void) {
    unsigned short sub = D_0000AE20;
    unsigned short chr = D_0000AE1C[0];

    if (chr < 2) {
        return;
    }
    if (ehsys_99FF9D51(chr, sub)) {
        func_00000F38(0x7F, 10, ehsys_7DAC692D(chr, sub, 0));
    }
}

/* func_00000F38 — draw text at (x, y) in a box reaching the right edge of
 * the 480-px screen (width 0x1E0 - x), coordinates in 26.6 fixed point.
 * Identical to rel_movie_viewer's func_00000894.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000F38(int x, int y, int text) {
    ehsys_B89D38DC(x << 6, y << 6, (0x1E0 - x) << 6, text);
}

/* func_00000F58 — voice test: subtitle preview for the selected line.
 * Builds the character's file stem — "chr" + ehsys_BAB403B8(chr, sub) minus
 * its 4-char extension, or "chrzakov%2d" (generic/"zako" voice set) with
 * ehsys_99F81E55(chr) + 50 when ehsys_916286F3(chr) is nonzero — then loads
 * "<stem>_voiceidchgtbl.bin" and "<stem>_wordstbl_<lang>.bin" from the
 * voicetest archive, where <lang> is "jegfis"[ehsys_get_language()]
 * (Japanese, English, German, French, Italian, Spanish). If both load it
 * maps the voice id to a line id (ehsys_E167E1C0), prints
 * "\x91\xe4\x8e\x8cID : %d" (D_0000ADA4, "台詞ID" = "line ID") at
 * (0x1E, 0x46) and draws the subtitle text in a 0x17C x 0x82 box at
 * (0x32, 0x78) via func_00001164 — with ehsys_C792FA38(1) and a 16 px font
 * when the language is Japanese.
 * Frame: buffers at sp+0x20 (path) and sp+0x120 (stem), ehsys_1B4AF0F5's two
 * halfword out-params at sp+0x1C / sp+0x18 — see the file header for the
 * declaration-order rules that reproduce it.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000F58(void) {
    char name[0x100];
    char path[0x100];
    int w;
    int h;
    int words;
    int table;
    int id;

    if (D_0000AE1C[0] < 2) {
        return;
    }
    if (!ehsys_99FF9D51(D_0000AE1C[0], D_0000AE20)) {
        return;
    }
    if (ehsys_916286F3(D_0000AE1C[0]) == 0) {
        ehsys_strcpy(name, D_0000AD68);
        ehsys_strcat(name, ehsys_BAB403B8(D_0000AE1C[0], D_0000AE20));
        name[ehsys_strlen(name) - 4] = 0;
    } else {
        ehsys_DF62D899(D_0000AE1C[0], D_0000AE20);
        ehsys_sprintf(name, D_0000AD6C, ehsys_99F81E55(D_0000AE1C[0]) + 0x32);
    }
    ehsys_sprintf(path, D_0000AD78, name);
    table = ehsys_C6C15111(D_0000AE30, path, 0);
    ehsys_sprintf(path, D_0000AD90, name, D_0000AD60[ehsys_get_language()]);
    words = ehsys_C6C15111(D_0000AE30, path, 0);
    if (table == 0 || words == 0) {
        return;
    }
    ehsys_1B4AF0F5(D_0000AE1C[0], D_0000AE20, func_000012A4(), &w, &h);
    id = (unsigned short) ehsys_E167E1C0(0, (unsigned short) w, (unsigned short) h, table);
    ehsys_sprintf(path, D_0000ADA4, id - 1);
    ehsys_C87BAB32(0x1E, 0x46, path);
    if (ehsys_get_language() == 0) {
        ehsys_C792FA38(1);
        ehsys_942B03D0(0x10, 0x10);
    }
    func_00001164(0x32, 0x78, 0x17C, 0x82, ehsys_E58C0FDC(words, (unsigned short) id));
    ehsys_C792FA38(0);
}

/* func_00001164 — draw text in an explicit (x, y, w, h) box, 26.6 fixed
 * point, colour -1 (all-ones, white) and flags 0.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001164(int x, int y, int w, int h, int text) {
    ehsys_3BB2BAC6(x << 6, y << 6, w << 6, h << 6, text, -1, 0);
}

/* func_00001180 — voice-test draw callback (D_00006850[13]). Row cursor
 * D_0000ADB0 ("\x81\x9e" = U+25C7 WHITE DIAMOND) at y = sel*20 + 10, then
 * "ChrID : %03d - ", "SubID : %1d", "VOICE : %03d : %s" with the voice id's
 * enum name from D_0000A868 (the stringified ID_VCM_* enum, comments and
 * all), the character name (func_00000EC0), the subtitle preview
 * (func_00000F58), and the help line D_0000ADB4 at (10, 0xFA). The three
 * format strings are reached through pointer variables D_00006BE4/E8/EC,
 * not directly.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00001180(void) {
    char buf[0x400];

    ehsys_60B55A50(-1);
    ehsys_942B03D0(14, 14);
    ehsys_C87BAB32(10, D_0000AE18.sel * 20 + 10, D_0000ADB0);
    ehsys_sprintf(buf, D_00006BE4, D_0000AE1C[0]);
    ehsys_C87BAB32(0x1E, 10, buf);
    ehsys_sprintf(buf, D_00006BE8, D_0000AE20);
    ehsys_C87BAB32(0x1E, 0x1E, buf);
    ehsys_sprintf(buf, D_00006BEC, D_0000AE24, D_0000A868[D_0000AE24]);
    ehsys_942B03D0(12, 12);
    ehsys_C87BAB32(0x1E, 0x32, buf);
    ehsys_942B03D0(14, 14);
    func_00000EC0();
    func_00000F58();
    ehsys_942B03D0(14, 14);
    ehsys_C87BAB32(10, 0xFA, D_0000ADB4);
}

/* func_000012A4 — the playable voice id of the current selection: the
 * character/sub base id (ehsys_DF62D899(chr, sub), mapped through
 * ehsys_82B715FD) plus the voice index val[2].
 * Reading val[2] into a local before the second call is what keeps the load
 * ahead of that call (and in $s0); inline in the return expression MWCC
 * sinks it past the call, 2 words shorter.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000012A4(void) {
    int t = ehsys_DF62D899(D_0000AE1C[0], D_0000AE20);
    int base = D_0000AE24;
    return base + ehsys_82B715FD(t);
}

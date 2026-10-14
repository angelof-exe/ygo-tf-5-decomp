/*
 * rel_soundtest.prx — reconstructed code (matching decompilation)
 *
 * The "Sound Test" screen: a scrolling list of the game's 41 BGM tracks
 * (8 rows visible, 34 scroll positions), a scrollbar, an auto-advance
 * toggle, and a "now playing" marker.
 *
 * Compiler:     Metrowerks CodeWarrior for PSP — MWCC 1.3 SP7 (mwccpsp_3.0.1_219)
 * Flags:        -O4,s -sdatathreshold 0   (see docs/09)
 * Verification: scripts/mwcc_build.sh + scripts/mwcc_diff.py per function,
 *               and `make MODULE=rel_soundtest SRC=1` for the whole module.
 *               All 12 functions are "MATCH 100%"; `make MODULE=rel_soundtest
 *               SRC=1` relinks the module to its shipped sha1.
 *
 * Conventions follow src/rel_movie_viewer.c (read its header for the MWCC
 * codegen levers). Placeholder names `func_XXXXXXXX` / `D_XXXXXXXX` encode
 * the address; readable aliases are #defined below where the evidence is
 * solid, and the same names are proposed in
 * config/symbols/rel_soundtest.extra.txt.
 *
 * DATA. Everything this module reads from .data is linked verbatim from the
 * data blob, so the C only declares it `extern`; no string literal or
 * initialised object appears here (that would emit bytes of our own). There
 * is no Shift-JIS text in this module: its .data strings are all ASCII file
 * names ("database/soundtest.ehp", "sound.gim", "bg.ehf", "bg.tmf",
 * "soundpda.ehf", "soundpda.tmf", "%s%s%s", "strTbl_", ".bin") plus the
 * per-language suffix table "j","e","g","f","i","s". Track titles live in
 * the localised string table strTbl_<lang>.bin inside soundtest.ehp.
 *
 * STATE. All mutable state is ONE 0xF4-byte struct at 0x6960 (.bss),
 * zero-filled by func_00000008 with exactly that size. It has to be a
 * struct, not separate globals: several accesses address a field through
 * the struct base (`&D_00006960` + 0x24 + i*4), which separate objects can
 * not produce.
 *
 * FLOAT CONSTANTS. The three floats at 0x67C4/0x67C8/0x67CC are MWCC
 * literal-pool entries (the module has no .rodata; the pool landed at the
 * start of .data). They are declared here as extern floats so our object
 * references the shipped pool instead of emitting its own copy; the
 * generated load sequence (lui %hi / lwc1 %lo) is the same either way.
 */

/* ---- state (0x6960, size 0xF4) ---- */
typedef struct SoundTest {
    int state;          /* 0x00  0x6960: screen state 0..4 (func_000000FC) */
    int autoplay;       /* 0x04  0x6964: advance to next track at end */
    int end_wait;       /* 0x08  0x6968: frames waited at end of track */
    int cursor;         /* 0x0C  0x696C: highlighted row, 0..7 */
    int scroll;         /* 0x10  0x6970: first visible track, 0..0x21 */
    int playing;        /* 0x14  0x6974: track being played, -1 = none */
    int play_pos;       /* 0x18  0x6978: last playback position seen */
    int select_timer;   /* 0x1C  0x697C: frames since a track was picked */
    int pda_anim;       /* 0x20  0x6980: open/close animation phase 0..3 */
    int unlocked[41];   /* 0x24  0x6984: per track, nonzero = selectable */
    int frame;          /* 0xC8  0x6A28: frame counter (blink/wave) */
    int ehp;            /* 0xCC  0x6A2C: loaded soundtest.ehp archive */
    int gim;            /* 0xD0  0x6A30: sound.gim */
    int strtbl;         /* 0xD4  0x6A34: strTbl_<lang>.bin */
    int bg_ehf;         /* 0xD8  0x6A38 */
    int bg_tmf;         /* 0xDC  0x6A3C */
    int pda_ehf;        /* 0xE0  0x6A40 */
    int pda_tmf;        /* 0xE4  0x6A44 */
    int heap;           /* 0xE8  0x6A48: work memory for the archive */
    int heap_owner;     /* 0xEC  0x6A4C: >0 when we allocated it ourselves */
    int heap_info;      /* 0xF0  0x6A50 */
} SoundTest;

extern SoundTest D_00006960;
#define g_soundtest D_00006960

/* pad state returned by ehsys_F5E3080C */
typedef struct Pad {
    int unk0;
    unsigned int trigger; /* 0x04: newly pressed */
    char pad8[0x24 - 0x8];
    unsigned int repeat;  /* 0x24: pressed with key repeat */
} Pad;

/* ---- .data (linked verbatim) ---- */
extern float D_000067C4;           /* literal pool: 33.17073 (scrollbar knob height) */
extern float D_000067C8;           /* literal pool: 4.146341 (scrollbar px per row) */
extern float D_000067CC;           /* literal pool: 2*pi */
extern char D_000067D0;            /* "" */
extern char D_000067D4[][2];       /* language suffixes "j","e","g","f","i","s" */
extern unsigned char D_000067E0[]; /* 61-entry alpha pulse table for the cursor */
extern int D_00006820[];           /* 41 BGM ids */
extern char D_000068C4;            /* "database/soundtest.ehp" */
extern char D_000068DC;            /* "sound.gim" */
extern char D_000068E8;            /* "bg.ehf" */
extern char D_000068F0;            /* "bg.tmf" */
extern char D_000068F8;            /* "soundpda.ehf" */
extern char D_00006908;            /* "soundpda.tmf" */
extern char D_00006918;            /* "%s%s%s" */
extern char D_00006920;            /* "strTbl_" */
extern char D_00006928;            /* ".bin" */
typedef struct { int v[4][2]; } BlinkTbl;
extern BlinkTbl D_00006930;        /* now-playing icon frames: {u,v} x 4 */

/* ---- imports ---- */
extern void ehsys_memset(void *, int, int);
extern int ehsys_sprintf(char *, const char *, ...);
extern int ehsys_get_language(void);
extern unsigned int ehsys_get_button_code(int);
extern void ehsys_frame_sync(void);
extern void ehsys_sceKernelChangeCurrentThreadAttr(int, int);
extern int ehsys_C07BB470(void *, int, int, int, int, int);
extern void ehsys_D979E9BF(int, int *, int);
extern int ehsys_1856E536(int, int);
extern void ehsys_6E42DDCD(void);
extern void ehsys_C9D9E989(int);
extern void ehsys_4AA58320(int);
extern void ehsys_33ED567B(int);
extern void ehsys_D853F15C(int);
extern void ehsys_13A4081A(int);
extern void ehsys_5998BFD7(int, int, int);
extern void ehsys_D470D0B2(int, int, int);
extern int ehsys_450CD6C5(void);
extern void ehsys_6367AE39(int);
extern int ehsys_06A1D1D8(void);
extern void ehsys_F6414A71(int, int);
extern void ehsys_ED1410E0(int);
extern int ehsys_D2A768F4(int, void *);
extern int ehsys_41AABF28(int, int);
extern void ehsys_BF3E9066(int, void *, int, int);
extern void ehsys_4B0DABFA(int, int);
extern int ehsys_C6C15111(int, void *, int);
extern void ehsys_FDAFCF3A(int, int);
extern int ehsys_251128FD(int);
extern int ehsys_592F970A(int);
extern Pad *ehsys_F5E3080C(void);
extern void ehsys_2AE3C0AD(int);
extern void ehsys_831C5769(int, int, int);
extern int ehsys_DFCA450B(int, int);
extern int *ehsys_B2B9FEFA(void);
extern int ehsys_85F89FE8(int);
extern int ehsys_7187E0F0(int, int, void *);
extern int ehsys_89812D8A(int);
extern int ehsys_558B2DCB(int);
extern int ehsys_F462EE6E(int, int);
extern int ehsys_9EA6989A(int, int);
extern int ehsys_CABEA834(int, int);
extern float ehsys_A38AA0E9(float);
extern int ehsys_5CBB9E43(int, int, int, int, int, int);
extern int ehsys_09794DF0(int, int, int);
extern int ehsys_8EA6B1E7(int, int, int, int, int);
extern int ehsys_FDD168CA(int, int, int, int, int, int, int, int);
extern void ehsys_B8AD96EA(int);
extern void ehsys_942B03D0(int, int);
extern void ehsys_60B55A50(int);
extern int ehsys_E58C0FDC(int, int);
extern int ehsys_A4AFF8E6(void);
extern void ehsys_C792FA38(int);
extern void ehsys_878EE1D3(int *);
extern void ehsys_1EC5342B(int);
extern void ehsys_BC8E65D7(int, int, int, int, int);
extern void ehsys_088F768A(int);
extern void ehsys_65DA0419(int);
extern void ehsys_D5D33185(int);
extern int ehsys_AA9913C7(int);
extern void ehsys_C859D5FE(int);
extern void ehsys_F6905EFA(int, int);
extern void ehsys_BE3756D0(int);
extern void ehsys_20E340D9(int, int);

/* forward declarations */
int func_00000000(void);
void func_00000008(int heap);
int func_000000FC(void);
void func_00000234(void);
void func_0000023C(void);
void func_00000280(void);
int func_000004AC(void);
void func_00000908(void);
void func_000013FC(int x, int y, int z, int str);
int func_00001410(int ctx, int x, int y, int u, int v, int w, int h, int tw, int th);
void func_0000148C(int from, int to);
void func_000015B8(void);

/* func_00000000 — trivial callback, returns 0.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000000(void) {
    return 0;
}

/* func_00000008 — init: clear the state, get work memory (caller-supplied,
 * or allocate 3 MB ourselves), set thread attributes and the per-language
 * setup flags.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000008(int heap) {
    ehsys_memset(&g_soundtest, 0, sizeof(SoundTest));
    if (heap == 0) {
        g_soundtest.heap_owner = ehsys_C07BB470(&D_000067D0, 2, 0, 0x300000, 1, 0);
        ehsys_D979E9BF(g_soundtest.heap_owner, &g_soundtest.heap_info, 0);
        g_soundtest.heap = ehsys_1856E536(g_soundtest.heap_info, 0x300000);
    } else {
        g_soundtest.heap = heap;
    }
    ehsys_sceKernelChangeCurrentThreadAttr(0, 0x4000);
    ehsys_6E42DDCD();
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

/* func_000000FC — per-frame update: state machine
 * 0 load+fade in, 1 wait fade, 2 run the menu, 3 fade out/stop BGM, 4 exit.
 * Returns 0 once the screen is finished. A `switch`: the target tests the
 * cases descending (4,3,2,1,0) and lays the bodies out ascending.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_000000FC(void) {
    switch (g_soundtest.state) {
    case 0:
        func_00000280();
        ehsys_13A4081A(0);
        ehsys_5998BFD7(1, 0xFF000000, 0);
        ehsys_D470D0B2(1, 0xFF000000, 0x10);
        g_soundtest.state = 1;
        break;
    case 1:
        if (ehsys_450CD6C5()) {
            g_soundtest.state = 2;
        }
        break;
    case 2:
        if (func_000004AC() == 0) {
            ehsys_6367AE39(600);
            ehsys_5998BFD7(1, 0xFF000000, 0x1E);
            g_soundtest.state = 3;
        }
        break;
    case 3:
        if (ehsys_450CD6C5() && ehsys_06A1D1D8() == 0) {
            g_soundtest.state = 4;
        }
        break;
    case 4:
        func_0000023C();
        return 0;
    }
    return 1;
}

/* func_00000234 — per-frame draw: tail call into the renderer.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000234(void) {
    func_00000908();
}

/* func_0000023C — shutdown: release the archive, then the heap if we
 * allocated it ourselves in func_00000008.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_0000023C(void) {
    func_000015B8();
    if (g_soundtest.heap_owner > 0) {
        ehsys_F6414A71(g_soundtest.heap_owner, g_soundtest.heap_info);
        ehsys_ED1410E0(g_soundtest.heap_owner);
    }
}

/* func_00000280 — load database/soundtest.ehp, pick its members, build
 * "strTbl_<lang>.bin", reset the menu, and query each track's unlock flag.
 * The language clamp must be ONE `||` condition: the `else if` form gives a
 * `bgezl` skip instead of the target's `bltzl` with the store in its slot.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_00000280(void) {
    char name[0x40];
    int size;
    int lang;
    int i;

    size = ehsys_D2A768F4(3, &D_000068C4);
    g_soundtest.ehp = ehsys_41AABF28(g_soundtest.heap, size);
    ehsys_BF3E9066(3, &D_000068C4, g_soundtest.ehp, size);
    ehsys_4B0DABFA(0, g_soundtest.ehp);
    lang = ehsys_get_language();
    if (lang < 0 || lang >= 6) {
        lang = 0;
    }
    g_soundtest.gim = ehsys_C6C15111(g_soundtest.ehp, &D_000068DC, 0);
    g_soundtest.bg_ehf = ehsys_C6C15111(g_soundtest.ehp, &D_000068E8, 0);
    g_soundtest.bg_tmf = ehsys_C6C15111(g_soundtest.ehp, &D_000068F0, 0);
    g_soundtest.pda_ehf = ehsys_C6C15111(g_soundtest.ehp, &D_000068F8, 0);
    g_soundtest.pda_tmf = ehsys_C6C15111(g_soundtest.ehp, &D_00006908, 0);
    ehsys_sprintf(name, &D_00006918, &D_00006920, D_000067D4[lang], &D_00006928);
    g_soundtest.strtbl = ehsys_C6C15111(g_soundtest.ehp, name, 0);
    ehsys_FDAFCF3A(g_soundtest.bg_ehf, g_soundtest.bg_tmf);
    ehsys_FDAFCF3A(g_soundtest.pda_ehf, g_soundtest.pda_tmf);
    g_soundtest.autoplay = 1;
    g_soundtest.playing = -1;
    g_soundtest.play_pos = 0;
    g_soundtest.cursor = 0;
    g_soundtest.scroll = 0;
    g_soundtest.select_timer = 0;
    g_soundtest.pda_anim = 0;
    for (i = 0; i < 41; i++) {
        g_soundtest.unlocked[i] = ehsys_251128FD(D_00006820[i]);
    }
}

/* func_000004AC — menu logic for one frame. Returns 0 when the player
 * backs out, 1 otherwise.
 *
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0).
 *
 * The "locked" branch of confirm has NO `return 1`. It falls through to the
 * shared return. That matters for the "down" handler's `++cursor >= 8` test.
 * MWCC fills an empty delay slot of a branch to the common `li v0,1;
 * epilogue` block by stealing the `li` (branch-likely) for only ONE branch
 * per target, the first in layout order. The fall-through from the locked
 * branch takes that steal (`b; li v0,1`, which looks the same as an explicit
 * `return 1`), so the down handler's `bnez` gets the target's `nop`. With
 * an explicit `return 1` there, the steal goes to the `bnez` instead
 * (`bnezl`+`li`, 2 words off).
 * Other shapes that matter: `x > K` gives `slti $at` with the store in the
 * delay slot; `x >= K` gives `slti $v0` with the store before it; the loop
 * `while ((x != 0) ^ 1)` (see func_0000148C) behind an explicit `if` guard;
 * and the order of the then/else bodies (target puts the 94C6 "locked"
 * sound in the else, the end-of-track wait in the else). */
int func_000004AC(void) {
    Pad *pad;
    unsigned int trig;
    int pos;
    int next;

    g_soundtest.frame++;
    if (g_soundtest.playing >= 0 && g_soundtest.autoplay != 0) {
        pos = ehsys_592F970A(D_00006820[g_soundtest.playing]);
        if (pos < g_soundtest.play_pos) {
            /* the position wrapped: the track ended */
            if (g_soundtest.end_wait > 0) {
                next = g_soundtest.playing + 1;
                if (next >= 41) {
                    next = 0;
                }
                if (g_soundtest.unlocked[next] == 0) {
                    do {
                        next++;
                        if (next >= 41) {
                            next = 0;
                        }
                    } while ((g_soundtest.unlocked[next] != 0) ^ 1);
                }
                func_0000148C(g_soundtest.playing, next);
                g_soundtest.playing = next;
                g_soundtest.play_pos = 0;
                g_soundtest.end_wait = 0;
            } else {
                g_soundtest.play_pos = pos;
                g_soundtest.end_wait++;
            }
        } else {
            g_soundtest.play_pos = pos;
        }
    }

    if (g_soundtest.select_timer != 0) {
        if (++g_soundtest.select_timer > 16) {
            g_soundtest.select_timer = 0;
            func_0000148C(g_soundtest.playing, g_soundtest.scroll + g_soundtest.cursor);
            g_soundtest.playing = g_soundtest.scroll + g_soundtest.cursor;
            g_soundtest.play_pos = 0;
            g_soundtest.end_wait = 0;
        }
        return 1;
    }

    pad = ehsys_F5E3080C();
    trig = pad->trigger;
    if (trig & (ehsys_get_button_code(1) & 0xFFFF)) {
        /* cancel: close the PDA and leave */
        ehsys_2AE3C0AD(0x94C5);
        g_soundtest.pda_anim = 2;
        ehsys_831C5769(g_soundtest.pda_ehf, 0x8D, 0x93);
        return 0;
    }
    if (trig & (ehsys_get_button_code(0) & 0xFFFF)) {
        /* confirm: play the highlighted track */
        if (g_soundtest.unlocked[g_soundtest.scroll + g_soundtest.cursor] != 0) {
            ehsys_2AE3C0AD(0x94C4);
            g_soundtest.select_timer = 1;
        } else {
            ehsys_2AE3C0AD(0x94C6);
        }
    } else if (trig & 0x300) {
        /* L/R: toggle auto-advance */
        ehsys_2AE3C0AD(0x94CC);
        if (g_soundtest.autoplay) {
            g_soundtest.autoplay = 0;
        } else {
            g_soundtest.autoplay = 1;
        }
        g_soundtest.play_pos = 0;
    } else if (pad->repeat & 0x10) {
        /* up */
        ehsys_2AE3C0AD(0x94C3);
        if (--g_soundtest.cursor < 0) {
            if (--g_soundtest.scroll < 0) {
                g_soundtest.cursor = 7;
                g_soundtest.scroll = 0x21;
            } else {
                g_soundtest.cursor = 0;
            }
        }
    } else if (pad->repeat & 0x40) {
        /* down */
        ehsys_2AE3C0AD(0x94C3);
        if (++g_soundtest.cursor >= 8) {
            if (++g_soundtest.scroll > 0x21) {
                g_soundtest.cursor = 0;
                g_soundtest.scroll = 0;
            } else {
                g_soundtest.cursor = 7;
            }
        }
    } else if (trig & 0x80) {
        /* left: page up */
        if (g_soundtest.scroll == 0) {
            ehsys_2AE3C0AD(0x94C6);
        } else {
            ehsys_2AE3C0AD(0x94C3);
            if ((g_soundtest.scroll -= 8) < 0) {
                g_soundtest.scroll = 0;
            }
        }
    } else if (trig & 0x20) {
        /* right: page down */
        if (g_soundtest.scroll >= 0x21) {
            ehsys_2AE3C0AD(0x94C6);
        } else {
            ehsys_2AE3C0AD(0x94C3);
            if ((g_soundtest.scroll += 8) > 0x21) {
                g_soundtest.scroll = 0x21;
            }
        }
    }
    return 1;
}

/* func_00000908 — draw the screen: background, PDA open/close animation,
 * cursor bar (alpha pulse from D_000067E0), scrollbar, auto-advance icon,
 * the blinking "now playing" icon, then the title, the 8 visible rows and
 * the selected track's two text lines from strTbl_<lang>.bin. Japanese
 * (language 0) draws text with ehsys_C792FA38(1) around it and 4 px lower
 * (yofs -5 vs -1).
 *
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0).
 *
 * `yofs` and the loop counter `i` (also the -2 y nudge) MUST be fields of
 * one local aggregate (`t`). As two plain ints MWCC colours them the other
 * way round ($s3/$s2 instead of the target's $s2/$s3). That costs 18
 * register fields and nothing else. Declaration order, scoping, separate
 * variables, `register` and loop forms all fail to fix it. A one-element
 * array for only one of them fails too. A struct or `int[2]` holding both
 * works, in either field order. Found by cut-out permuting a 40-line copy
 * with the same live ranges (see docs/modules/rel_soundtest.md). Also the
 * row y is `t.yofs + (t.i * 0x18 + 0x28)` (the parenthesised sum is
 * computed first). Everything else follows the source as written, including:
 * the 27-int attribute buffer (memset only 0x60 of it — the 12-byte gap in
 * the frame proves the size), the `int[4][2]` local copied from the .data
 * template at 0x6930, `(g.frame >> 2) % 4` (sra + ext + fixup),
 * `(unsigned int)(255.0f * wave)`, the int global loaded straight into an
 * FPR for `(float)g.scroll`, and `&g.unlocked[sel]` as a CSE'd address
 * (written as the repeated subscript, not as a pointer local, which costs
 * one word). */
void func_00000908(void) {
    int ctx;
    BlinkTbl blink;
    int attr[27];
    int sel;
    struct {
        int i;      /* row loop counter; first the -2 Japanese y nudge */
        int yofs;   /* text y offset: -5 Japanese, -1 otherwise */
    } t;            /* see the comment above: must be an aggregate */
    int frame;
    int phase;
    int row;
    int knob;
    float wave;

    t.yofs = -5;
    sel = g_soundtest.scroll + g_soundtest.cursor;
    if (ehsys_get_language() != 0) {
        t.yofs = -1;
    }
    ctx = ehsys_DFCA450B(4, 0);
    ehsys_memset(attr, 0, 0x60);
    attr[0] = *ehsys_B2B9FEFA();
    ctx = ehsys_85F89FE8(ctx);
    ctx = ehsys_7187E0F0(ctx, g_soundtest.bg_ehf, attr);
    ctx = ehsys_89812D8A(ctx);
    ehsys_memset(attr, 0, 0x60);
    attr[0] = *ehsys_B2B9FEFA();
    frame = ehsys_558B2DCB(g_soundtest.pda_ehf);
    switch (g_soundtest.pda_anim) {
    case 0:
        if (frame > 7) {
            g_soundtest.pda_anim = 1;
            ehsys_831C5769(g_soundtest.pda_ehf, 8, 0x8C);
        }
        break;
    case 1:
        break;
    case 2:
        if (frame >= 0x92) {
            g_soundtest.pda_anim = 3;
        }
        break;
    }
    if (g_soundtest.pda_anim < 3) {
        ctx = ehsys_85F89FE8(ctx);
        ctx = ehsys_7187E0F0(ctx, g_soundtest.pda_ehf, attr);
        ctx = ehsys_89812D8A(ctx);
    }
    if (g_soundtest.pda_anim == 1) {
        ctx = ehsys_F462EE6E(ctx, 9);
        ctx = ehsys_9EA6989A(ctx, g_soundtest.gim);
        if (g_soundtest.select_timer == 0 || (g_soundtest.select_timer & 2)) {
            ctx = ehsys_CABEA834(ctx, (D_000067E0[g_soundtest.frame % 61] << 24) | 0xFFFFFF);
            ctx = func_00001410(ctx, 0x30, g_soundtest.cursor * 0x18 + 0x20, 0, 0, 2, 0x10, 2, 0x18);
            ctx = func_00001410(ctx, 0x32, g_soundtest.cursor * 0x18 + 0x20, 2, 0, 0xC, 0x10, 0x18C, 0x18);
            ctx = func_00001410(ctx, 0x1BE, g_soundtest.cursor * 0x18 + 0x20, 0xE, 0, 2, 0x10, 2, 0x18);
        }
        ctx = ehsys_CABEA834(ctx, -1);
        ctx = func_00001410(ctx, 0x28, 0xE0, 0x10, 0x30, 8, 8, 0x1A8, 8);
        wave = ehsys_A38AA0E9(D_000067CC * (float) ((g_soundtest.frame * 5) % 180) / 360.0f);
        ctx = ehsys_F462EE6E(ctx, 1);
        ctx = ehsys_F462EE6E(ctx, 9);
        ctx = ehsys_F462EE6E(ctx, 4);
        ctx = ehsys_5CBB9E43(ctx, 0, 2, 3, 0, 0);
        ctx = ehsys_CABEA834(ctx, -1);
        ctx = ehsys_09794DF0(ctx, 0, 1);
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
        knob = D_000067C8 * (float) g_soundtest.scroll;
        ctx = ehsys_8EA6B1E7(ctx, 4, 0x12, 4, (int) D_000067C4);
        ctx = ehsys_FDD168CA(ctx, 0x1C7, knob + 0x27, 0, 0x37, 0x14, 0, 0);
        ctx = ehsys_CABEA834(ctx, ((unsigned int) (255.0f * wave) << 24) | 0xFFFFFF);
        if (g_soundtest.scroll != 0) {
            ctx = ehsys_8EA6B1E7(ctx, 4, 4, 4, 4);
            ctx = ehsys_FDD168CA(ctx, 0x1C7, 0x1E, 0, 0x30, 2, 0, 0);
        }
        if (g_soundtest.scroll < 0x21) {
            ctx = ehsys_8EA6B1E7(ctx, 4, 4, 4, 4);
            ctx = ehsys_FDD168CA(ctx, 0x1C7, 0xD4, 0, 0x30, 8, 0, 0);
        }
        ctx = ehsys_CABEA834(ctx, -1);
        ctx = func_00001410(ctx, 0x1B4, 0x10, 0xC, 0x38, 0xC, 8, 0xC, 8);
        if (g_soundtest.autoplay) {
            ctx = func_00001410(ctx, 0x19A, 0xC, 0, 0x20, 0x18, 0x10, 0x18, 0x10);
        } else {
            ctx = func_00001410(ctx, 0x19A, 0xC, 0, 0x10, 0x18, 0x10, 0x18, 0x10);
        }
        ctx = ehsys_CABEA834(ctx, -1);
        blink = D_00006930;
        phase = (g_soundtest.frame >> 2) % 4;
        row = g_soundtest.playing - g_soundtest.scroll;
        if (row >= 0 && row < 8) {
            ctx = func_00001410(ctx, 0x34, row * 0x18 + 0x28, blink.v[phase][0], blink.v[phase][1],
                                0xC, 0xC, 0xC, 0xC);
        }
        if (g_soundtest.unlocked[sel] != 0) {
            ctx = ehsys_CABEA834(ctx, -1);
            t.i = 0;
            if (ehsys_get_language() != 0) {
                t.i = -2;
            }
            ctx = func_00001410(ctx, 0xB6, t.i + 0xF4, blink.v[1][0], blink.v[1][1], 0xC, 0xC, 0xC, 0xC);
        }
        ehsys_B8AD96EA(ctx);
        ehsys_942B03D0(0xC, 0xC);
        ehsys_60B55A50(-1);
        func_000013FC(0x2A, 0x10, 0x1E0, ehsys_E58C0FDC(g_soundtest.strtbl, 0x83));
        ctx = ehsys_A4AFF8E6();

        ehsys_B8AD96EA(ctx);
        ehsys_942B03D0(0xC, 0xC);
        if (ehsys_get_language() == 0) {
            ehsys_C792FA38(1);
        }
        for (t.i = 0; t.i < 8; t.i++) {
            if (t.i == g_soundtest.cursor) {
                ehsys_60B55A50(-1);
            } else {
                ehsys_60B55A50(0x9CFFFFFF);
            }
            func_000013FC(0x48, t.yofs + (t.i * 0x18 + 0x28), 0x1E0,
                          ehsys_E58C0FDC(g_soundtest.strtbl,
                                         g_soundtest.unlocked[g_soundtest.scroll + t.i]
                                             ? g_soundtest.scroll + 0x55 + t.i : 0x82));
        }
        if (ehsys_get_language() == 0) {
            ehsys_C792FA38(0);
        }
        ctx = ehsys_A4AFF8E6();

        ehsys_B8AD96EA(ctx);
        ehsys_942B03D0(0xC, 0xC);
        ehsys_60B55A50(-1);
        if (ehsys_get_language() == 0) {
            ehsys_C792FA38(1);
        }
        func_000013FC(0x30, t.yofs + 0xF4, 0x1E0,
                      ehsys_E58C0FDC(g_soundtest.strtbl, g_soundtest.unlocked[sel] ? sel + 0x2B : 0x54));
        if (ehsys_get_language() == 0) {
            ehsys_C792FA38(0);
        }
        ctx = ehsys_A4AFF8E6();

        ehsys_B8AD96EA(ctx);
        ehsys_942B03D0(0xC, 0xC);
        ehsys_60B55A50(-1);
        if (ehsys_get_language() == 0) {
            ehsys_C792FA38(1);
        }
        func_000013FC(0xC2, t.yofs + 0xF4, 0x1E0,
                      ehsys_E58C0FDC(g_soundtest.strtbl, g_soundtest.unlocked[sel] ? sel + 1 : 0x2A));
        if (ehsys_get_language() == 0) {
            ehsys_C792FA38(0);
        }
        ctx = ehsys_A4AFF8E6();
    }
    ehsys_878EE1D3(&ctx);
    ehsys_1EC5342B(ctx);
}

/* func_000013FC — draw a text string at (x, y) with wrap width z, in the
 * engine's 26.6 fixed point.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000013FC(int x, int y, int z, int str) {
    ehsys_BC8E65D7(x << 6, y << 6, z << 6, str, -1);
}

/* func_00001410 — draw one sprite: texture rect (u, v, tw, th) at screen
 * rect (x, y, w, h). The 9th argument comes in on the stack.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00001410(int ctx, int x, int y, int w, int h, int u, int v, int tw, int th) {
    return ehsys_FDD168CA(ehsys_8EA6B1E7(ctx, u, v, tw, th), x, y, 0, w, h, 0, 0);
}

/* func_0000148C — switch BGM: stop/fade out the current track (if any),
 * then start the new one and wait until it is playing.
 * The last wait loop's test is `sltu v0,zero,v0; xori v0,v0,1; bnez`: a
 * materialised boolean negation. `== 0`, `!x`, `!(x != 0)` and
 * `(x == 0) != 0` all fold to a plain `beqz` (2 words short); only
 * `(x != 0) ^ 1` reproduces it.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_0000148C(int from, int to) {
    int *p;

    if (from >= 0) {
        ehsys_088F768A(D_00006820[to]);
        ehsys_6367AE39(1000);
        while (ehsys_06A1D1D8() != 0) {
            ehsys_frame_sync();
        }
        p = &D_00006820[from];
        ehsys_65DA0419(*p);
        while (ehsys_06A1D1D8() != 0) {
            ehsys_frame_sync();
        }
        ehsys_D5D33185(*p);
        while (ehsys_AA9913C7(*p) != 0) {
            ehsys_frame_sync();
        }
    }
    p = &D_00006820[to];
    ehsys_C859D5FE(*p);
    while ((ehsys_AA9913C7(*p) != 0) ^ 1) {
        ehsys_frame_sync();
    }
    ehsys_F6905EFA(0x7F, 100);
    ehsys_BE3756D0(*p);
}

/* func_000015B8 — release the archive.
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
void func_000015B8(void) {
    ehsys_6E42DDCD();
    ehsys_20E340D9(g_soundtest.heap, g_soundtest.ehp);
}

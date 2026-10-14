/*
 * rel_html_view.prx — reconstructed code (matching decompilation)
 *
 * Compiler:     Metrowerks CodeWarrior for PSP — MWCC 1.3 SP7 (mwccpsp_3.0.1_219)
 * Flags:        -O4,s -sdatathreshold 0   (SIZE, not ,p — see docs/09)
 * Verification: scripts/mwcc_build.sh src/rel_html_view.c, then
 *               scripts/mwcc_diff.py asm/rel_html_view/text.s build/mwcc/rel_html_view.o
 *               (relocation-aware). Functions tagged "MATCH 100%" were confirmed
 *               that way. func_0000039C's two jump-table relocation words show
 *               up as "RELOC MISMATCH" in mwcc_diff only because our jump table
 *               is an anonymous local .rodata symbol; the Makefile's SRC=1 link
 *               places that .rodata at jtbl_00005E34, and the table bytes
 *               themselves are linked verbatim from the shipped .data.
 *
 * What the module does: it is a thin driver for the PSP *system* web browser.
 * func_00000000 registers a run callback (func_00000034) and an empty teardown
 * (func_00000084) with the engine via ehsys_B4471B5E, exactly like every other
 * leaf module. The run callback steps a 4-state machine (g_state, func_00000670):
 *   1: load the net/http/ssl utility modules (0x100..0x106), bring up
 *      sceNet/Inet/Resolver/Apctl, then sceSsl/sceHttp/sceHttps + certs and
 *      system cookies; then fill a pspUtilityHtmlViewerParam (g_html_param,
 *      0xA8 bytes — the SDK struct exactly) pointing at Konami's download URL
 *      and a /PSP/SAVEDATA/<game id> download dir, and start the viewer;
 *   2: pump sceUtilityHtmlViewerUpdate until the dialog finishes;
 *   3: tear everything down in reverse order;
 *   4: return 1, which ends func_00000034's frame loop.
 *
 * No Japanese (Shift-JIS) text exists in this module: the only strings in
 * .data are the ASCII s_download_url and s_savedata_dir.
 *
 * Conventions (see src/rel_movie_viewer.c and docs/09-first-match.md):
 *  - names func_XXXXXXXX are address placeholders; names in the per-function
 *    comments are PROPOSALS backed by the imports each function calls.
 *  - switch tests come out in REVERSE source order with bodies laid out in
 *    source order (func_00000670 tests 4,3,2,1; func_000005A0 tests 0,3,2,4,1).
 *
 * MWCC lever found here (also relevant to rel_movie_viewer func_00000294,
 * docs/10): MWCC's copy propagation of a global's constant address appears to
 * be confined to the basic block that holds the `p = &global` assignment. In
 * that block every `p->field` store is rematerialised as `lui + sw %lo(...)`;
 * after the first branch the stores go through the saved register. Splitting
 * the block with a branch on a value the optimiser only folds LATER (a local
 * holding a constant: `int n = sizeof(*p); if (n != 0) ...`) keeps the address
 * in `$s0` for the whole function, and the dead branch is gone from the final
 * code. A test on `p` itself also splits the block but leaves a real
 * `beqz $s0` behind. This is what matched func_00000470.
 *
 * Tooling gotcha hit while matching this module: wibo/mwccpsp lower-cases the
 * output filename, so `-o v_A.o` writes `v_a.o` (silently overwriting any
 * lower-case file of that name) and a script looking for `v_A.o` finds
 * nothing. Use lower-case names for scratch compiles.
 */

/* pspUtilityDialogCommon (pspsdk psputility_dialog.h) */
typedef struct {
    int size;            /* 0x00 */
    int language;        /* 0x04 */
    int buttonSwap;      /* 0x08 */
    int graphicsThread;  /* 0x0C */
    int accessThread;    /* 0x10 */
    int fontThread;      /* 0x14 */
    int soundThread;     /* 0x18 */
    int result;          /* 0x1C */
    int reserved[4];     /* 0x20 */
} pspUtilityDialogCommon;

/* pspUtilityHtmlViewerParam (pspsdk psputility_htmlviewer.h), 0xA8 bytes */
typedef struct {
    pspUtilityDialogCommon base; /* 0x00 */
    void *memaddr;         /* 0x30 */
    unsigned int memsize;  /* 0x34 */
    int unknown1;          /* 0x38 */
    int unknown2;          /* 0x3C */
    char *initialurl;      /* 0x40 */
    unsigned int numtabs;  /* 0x44 */
    unsigned int interfacemode; /* 0x48 */
    unsigned int options;  /* 0x4C */
    char *dldirname;       /* 0x50 */
    char *dlfilename;      /* 0x54 */
    char *uldirname;       /* 0x58 */
    char *ulfilename;      /* 0x5C */
    unsigned int cookiemode; /* 0x60 */
    unsigned int unknown3; /* 0x64 */
    char *homeurl;         /* 0x68 */
    unsigned int textsize; /* 0x6C */
    unsigned int displaymode; /* 0x70 */
    unsigned int connectmode; /* 0x74 */
    unsigned int disconnectmode; /* 0x78 */
    unsigned int memused;  /* 0x7C */
    int unknown4[10];      /* 0x80 */
} pspUtilityHtmlViewerParam;

/* engine (libehsys_rel) imports */
extern void ehsys_B4471B5E(void (*)(void), void (*)(void), void *);
extern void ehsys_sceKernelChangeCurrentThreadAttr(int, int);
extern void ehsys_frame_sync(void);
extern void ehsys_08813E19(int, int);
extern int ehsys_get_language(void);
extern int ehsys_get_button_code(int);
extern void ehsys_sceGuSync(int, int);
extern void ehsys_57018B7C(char *);
extern void ehsys_memset(void *, int, int);
extern void ehsys_strcpy(char *, const char *);
extern void ehsys_strcat(char *, const char *);

/* SDK imports */
extern int sceUtilityLoadModule(int);
extern int sceUtilityUnloadModule(int);
extern int sceSslInit(int);
extern int sceSslEnd(void);
extern int sceHttpInit(unsigned int);
extern int sceHttpEnd(void);
extern int sceHttpsInit(int, int, int, int);
extern int sceHttpsEnd(void);
extern int sceHttpsLoadDefaultCert(int, int);
extern int sceHttpLoadSystemCookie(void);
extern int sceHttpSaveSystemCookie(void);
extern int sceNetInit(int, int, int, int, int);
extern int sceNetTerm(void);
extern int sceNetInetInit(void);
extern int sceNetInetTerm(void);
extern int sceNetResolverInit(void);
extern int sceNetResolverTerm(void);
extern int sceNetApctlInit(int, int);
extern int sceNetApctlTerm(void);
extern int sceUtilityHtmlViewerInitStart(pspUtilityHtmlViewerParam *);
extern int sceUtilityHtmlViewerGetStatus(void);
extern int sceUtilityHtmlViewerUpdate(int);
extern int sceUtilityHtmlViewerShutdownStart(void);

/* module data (names from config/symbols/rel_html_view.extra.txt) */
extern char s_download_url[];  /* "http://www.konami.jp/gs/game/yugioh_tf5/dl/eu.php" */
extern char s_savedata_dir[];  /* "/PSP/SAVEDATA/" */
extern char g_module_reg[];
extern int g_state;
extern pspUtilityHtmlViewerParam g_html_param;
extern char g_url_buf[];
extern char g_dl_dir_buf[];
extern void *g_heap_ptr;
extern char g_heap[];

void func_00000034(void);
void func_00000084(void);
int func_0000008C(void);
void func_00000134(void);
int func_00000178(void);
void func_00000244(void);
int func_00000278(void);
void func_00000358(void);
int func_0000039C(void);
void func_00000414(char *dst);
void func_00000420(char *dst);
int func_00000470(void *mem);
int func_000005A0(void);
void func_00000650(void);
int func_00000670(void);

/* func_00000000 — module entry point: registers the run callback
 * (func_00000034) and the teardown callback (func_00000084).
 * MATCH 100% (mwccpsp_3.0.1_219, -O4,s -sdatathreshold 0). */
int func_00000000(void) {
    ehsys_B4471B5E(func_00000034, func_00000084, g_module_reg);
    return 0;
}

/* func_00000034 — run callback: raises the thread attr (0x4000 = VFPU),
 * initialises the state machine, steps it once per frame until it reports
 * done, then hands control back to the engine (ehsys_08813E19(13, 0) —
 * presumably "switch to mode 13").
 * MATCH 100%. The loop must be `while (1) { if (step()) break; sync(); }`;
 * `while (!step()) sync();` rotates the test and is 3 words longer. */
void func_00000034(void) {
    ehsys_sceKernelChangeCurrentThreadAttr(0, 0x4000);
    func_00000650();
    while (1) {
        if (func_00000670()) break;
        ehsys_frame_sync();
    }
    ehsys_08813E19(13, 0);
}

/* func_00000084 — teardown callback (empty).
 * MATCH 100%. */
void func_00000084(void) {
}

/* func_0000008C — proposed name: load_net_modules. Loads the utility
 * modules 0x100 (NET_COMMON), 0x102..0x106 (NET_ADHOC.. / NET_INET,
 * PARSEURI, PARSEHTTP, HTTP, SSL); on the first failure unloads them all.
 * MATCH 100%. `ret = f(); if (ret < 0)` as two statements — folding the
 * assignment into the condition drops the `move $s0` before the test. */
int func_0000008C(void) {
    int ret;
    ret = sceUtilityLoadModule(0x100);
    if (ret < 0) goto fail;
    ret = sceUtilityLoadModule(0x102);
    if (ret < 0) goto fail;
    ret = sceUtilityLoadModule(0x103);
    if (ret < 0) goto fail;
    ret = sceUtilityLoadModule(0x104);
    if (ret < 0) goto fail;
    ret = sceUtilityLoadModule(0x105);
    if (ret < 0) goto fail;
    ret = sceUtilityLoadModule(0x106);
    if (ret < 0) goto fail;
    return ret;
fail:
    func_00000134();
    return ret;
}

/* func_00000134 — proposed name: unload_net_modules (reverse order).
 * MATCH 100%. */
void func_00000134(void) {
    sceUtilityUnloadModule(0x106);
    sceUtilityUnloadModule(0x105);
    sceUtilityUnloadModule(0x104);
    sceUtilityUnloadModule(0x103);
    sceUtilityUnloadModule(0x102);
    sceUtilityUnloadModule(0x100);
}

/* func_00000178 — proposed name: http_init. SSL + HTTP + HTTPS bring-up,
 * default certs and system cookies; any failure after sceSslInit calls
 * http_term (func_00000244).
 * MATCH 100%. */
int func_00000178(void) {
    int ret;
    ret = sceSslInit(0x28000);
    if (ret < 0) return ret;
    ret = sceHttpInit(0x28000);
    if (ret < 0) { func_00000244(); return ret; }
    ret = sceHttpsInit(0, 0, 0, 0);
    if (ret < 0) { func_00000244(); return ret; }
    ret = sceHttpsLoadDefaultCert(0, 0);
    if (ret < 0) { func_00000244(); return ret; }
    ret = sceHttpLoadSystemCookie();
    if (ret < 0) { func_00000244(); return ret; }
    return ret;
}

/* func_00000244 — proposed name: http_term.
 * MATCH 100%. */
void func_00000244(void) {
    sceHttpSaveSystemCookie();
    sceHttpsEnd();
    sceHttpEnd();
    sceSslEnd();
}

/* func_00000278 — proposed name: net_init. Utility modules, sceNet
 * (0x20000-byte pool, thread prios 0x2A), Inet, Resolver, Apctl
 * (0x5400 stack, prio 0x30), then http_init; on failure net_term.
 * MATCH 100%. */
int func_00000278(void) {
    int ret;
    ret = func_0000008C();
    if (ret < 0) return ret;
    ret = sceNetInit(0x20000, 0x2A, 0, 0x2A, 0);
    if (ret < 0) return ret;
    ret = sceNetInetInit();
    if (ret < 0) { func_00000358(); return ret; }
    ret = sceNetResolverInit();
    if (ret < 0) { func_00000358(); return ret; }
    ret = sceNetApctlInit(0x5400, 0x30);
    if (ret < 0) { func_00000358(); return ret; }
    ret = func_00000178();
    if (ret < 0) { func_00000358(); return ret; }
    return ret;
}

/* func_00000358 — proposed name: net_term (reverse of net_init).
 * MATCH 100%. */
void func_00000358(void) {
    func_00000244();
    sceNetApctlTerm();
    sceNetResolverTerm();
    sceNetInetTerm();
    sceNetTerm();
    func_00000134();
}

/* func_0000039C — proposed name: get_viewer_language. Maps the game's
 * language index (ehsys_get_language) to the value stored in
 * pspUtilityDialogCommon.language. The jump table is jtbl_00005E34 in .data.
 * MATCH 100% (the two jump-table relocation words are only reported as
 * RELOC MISMATCH by mwcc_diff because our table is an anonymous .rodata
 * symbol; see the file header). */
int func_0000039C(void) {
    switch (ehsys_get_language()) {
    case 0: return 0;
    case 1: return 1;
    case 2: return 6;
    case 3: return 2;
    case 4: return 5;
    case 5: return 3;
    default: return 1;
    }
}

/* func_00000414 — appends the download URL to dst (tail call).
 * MATCH 100%. */
void func_00000414(char *dst) {
    ehsys_strcat(dst, s_download_url);
}

/* func_00000420 — builds "/PSP/SAVEDATA/" + <0x20-byte name from
 * ehsys_57018B7C> (presumably the game's savedata id) into dst.
 * MATCH 100%. */
void func_00000420(char *dst) {
    char buf[0x20];
    ehsys_strcpy(dst, s_savedata_dir);
    ehsys_memset(buf, 0, sizeof(buf));
    ehsys_57018B7C(buf);
    ehsys_strcat(dst, buf);
}

/* func_00000470 — proposed name: html_viewer_start. Fills g_html_param
 * (pspUtilityHtmlViewerParam) and starts the system browser with `mem`
 * (the 6 MB g_heap) as its work area. buttonSwap = 0 when the engine's
 * confirm button code is 0x2000 (CIRCLE), else 1 (CROSS).
 * MATCH 100%.
 *
 * Without the `if (size != 0)` guard the compiler copy-propagates
 * &g_html_param into the size/language stores of the first basic block
 * (`lui; sw %lo(g_html_param)`), +2 words. The guard only splits the block
 * for that pass and is folded away afterwards; see the file header. It is
 * very plausibly a remnant of a clear-memory helper/macro in the original. */
int func_00000470(void *mem) {
    pspUtilityHtmlViewerParam *p = &g_html_param;
    int size = sizeof(*p);
    if (size != 0) {
        ehsys_memset(p, 0, sizeof(*p));
    }
    p->base.size = sizeof(*p);
    p->base.language = func_0000039C();
    if ((ehsys_get_button_code(0) & 0xFFFF) == 0x2000) {
        p->base.buttonSwap = 0;
    } else {
        p->base.buttonSwap = 1;
    }
    p->base.soundThread = 0x10;
    p->base.graphicsThread = 0x11;
    p->base.fontThread = 0x12;
    p->base.accessThread = 0x13;
    p->memsize = 0x600000;
    p->memaddr = mem;
    func_00000414(g_url_buf);
    p->initialurl = g_url_buf;
    p->numtabs = 3;
    p->interfacemode = 2;
    p->options = 0x2BA;
    func_00000420(g_dl_dir_buf);
    p->dldirname = g_dl_dir_buf;
    p->cookiemode = 3;
    p->unknown3 = 0x200;
    p->textsize = 1;
    p->displaymode = 1;
    p->connectmode = 1;
    p->unknown4[9] = 1;
    p->homeurl = g_url_buf;
    p->dlfilename = 0;
    p->ulfilename = 0;
    p->uldirname = 0;
    p->disconnectmode = 0;
    return sceUtilityHtmlViewerInitStart(p);
}

/* func_000005A0 — proposed name: html_viewer_update. One frame of the
 * dialog: status 2 (VISIBLE) -> Update, 3 (QUIT) -> ShutdownStart,
 * 0 (NONE, i.e. shut down) -> return 1 on success / 2 if base.result != 0.
 * MATCH 100%.
 *
 * Case order 1,4,2,3,0 is forced by the test order 0,3,2,4,1 (reverse of
 * source). Case 4 must be `break`, NOT `return 0`: with `return 0` the case-1
 * `beq` to the shared `return 0` block gets turned into `beql` + `move v0,0`
 * (2 words differ). With both 1 and 4 as `break`, MWCC fills only the case-4
 * branch, as the target does. The `== 0 ? 1 : 2` polarity gives the target's
 * `bnel` (the `!= 0 ? 2 : 1` spelling gives `beqzl` with swapped constants). */
int func_000005A0(void) {
    pspUtilityHtmlViewerParam *p = &g_html_param;
    ehsys_sceGuSync(0, 0);
    switch (sceUtilityHtmlViewerGetStatus()) {
    case 1:
        break;
    case 4:
        break;
    case 2:
        sceUtilityHtmlViewerUpdate(2);
        break;
    case 3:
        sceUtilityHtmlViewerShutdownStart();
        break;
    case 0:
        return (p->base.result == 0) ? 1 : 2;
    }
    return 0;
}

/* func_00000650 — proposed name: html_view_init: state = 1, heap ptr.
 * MATCH 100%. */
void func_00000650(void) {
    g_state = 1;
    g_heap_ptr = g_heap;
}

/* func_00000670 — proposed name: html_view_step. State machine
 * (see file header); returns 1 once state 4 is reached.
 * MATCH 100%. */
int func_00000670(void) {
    switch (g_state) {
    case 1:
        if (func_00000278() < 0) {
            g_state = 3;
            break;
        }
        if (func_00000470(g_heap_ptr) < 0) {
            g_state = 3;
            break;
        }
        g_state++;
        break;
    case 2:
        if (func_000005A0()) {
            g_state++;
        }
        break;
    case 3:
        func_00000358();
        g_state++;
        break;
    case 4:
        return 1;
    }
    return 0;
}

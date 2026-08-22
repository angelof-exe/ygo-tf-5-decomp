# Import resolution: every call out of every module, named

Before this, every call a module made to the outside world looked like
`func_00001D30(dst, src)` — an address in a stub table, meaningless on its own
and *differently numbered in every module*. This resolves all of them, and does
it once for all 28 modules at the same time.

Run `scripts/resolve_nids.py` to regenerate everything here.

## What an import is

A PSP import is a `(library, NID)` pair. The NID is the first 4 bytes of
`SHA-1(function name)`, little-endian:

```python
>>> struct.unpack("<I", hashlib.sha1(b"sceHttpInit").digest()[:4])[0]
0xAB1ABE07
```

Each module's `.lib.stub` lists its libraries; each entry points at an array of
NIDs and at a matching array of 8-byte trampolines in `.sceStub.text`. Stub *k*
of a library implements NID *k*. That is the whole mechanism.

## The three kinds here, and why all of them resolve

| kind | count | how it resolves |
|---|---:|---|
| `libehsys_rel` | 1729 exported, **986 actually called** | the EBOOT's own export table |
| `lib*_rel` (module to module) | 27 libs, **275 called** | the owning module's export table |
| `sce*` (real SDK) | **47**, all called | name corpus + hash verification |

**The engine table is a project-wide constant.** Every one of the 28 modules
links a stub for *all 1729* exports of `libehsys_rel` whether it calls them or
not — which is why `rel_html_view` calls only 34 of its 1729 stubs. More
usefully, that NID array is **byte-identical in all 28 modules and in the
EBOOT's own export table** (`sha1 820088858e31`). So stub *k* is the same engine
function everywhere, and `build/EBOOT.elf`'s `.lib.ent` pairs each NID with the
address of the function that implements it. Nothing external is needed.

**The SDK names are proven, not trusted.** Names come from a candidate corpus
(PPSSPP's HLE tables, fetched into the gitignored `tools/nid_db/`), but every
one is accepted only if re-hashing the name reproduces the NID we found in the
binary. A wrong or outdated corpus therefore cannot introduce a wrong name — it
can only fail to produce one. All 47 verify; `nids/sdk.csv` carries the
`verified` column as evidence.

## Naming scheme

| kind | name | example |
|---|---|---|
| SDK | the real name | `sceHttpInit` |
| engine | `ehsys_<NID>` | `ehsys_B4471B5E` |
| module export | `<module>_<NID>` | `cardalbum_1A2B3C4D` |

**The engine re-exports the C library.** 107 of the 1729 engine exports hash to
a name we already know, because `libehsys_rel` re-exports much of the C library
and several kernel functions under their real names. Those become
`ehsys_memset`, `ehsys_strcpy`, `ehsys_sceGuSync` and so on — 43 of them are
actually called, and they are the most-called imports in the whole project:

| engine export | call sites, project-wide |
|---|---:|
| `ehsys_memset` | 1064 |
| `ehsys_memcpy` | 257 |
| `ehsys_sprintf` | 97 |
| `ehsys_qsort` | 73 |
| `ehsys_strcpy` | 58 |
| `ehsys_strcat` | 48 |
| `ehsys_strncmp` | 36 |
| `ehsys_strcmp` | 32 |

The `ehsys_` prefix is kept deliberately: these are the *engine's* exports, not
the SDK's, and a bare `memset` would let MWCC expand its own builtin instead of
emitting the call we need to match. Extending `EXTRA_CANDIDATES` in the script
is the cheapest way to name more of the 1729 — every addition is hash-verified,
so a wrong guess there can only fail to match, never mis-name.

The NID is the identity: stable across all 28 modules, independent of any load
address, and the same string in the disassembly and in the C. 26 of the engine's
1729 exports are listed twice (both entries point at the same address — genuine
aliases); repeats get an `_alias2` suffix, assigned by position in an array that
is identical everywhere, so those names are cross-module stable too.

**To give an engine function a real name later**, fill the `name` column in
`nids/ehsys.csv` and re-run the script — every module's symbol file regenerates.
That is the point of the indirection: one edit renames a function across the
whole project.

## Hand-identified engine functions

`nids/ehsys.names.csv` carries names we worked out by reading the EBOOT, for
engine functions whose NIDs hash to nothing known. **These are inferences, not
proofs** — the engine's original symbol names are gone, so unlike a `sce*` name
a chosen name can never be re-derived from its NID. The file therefore records
`confidence` and the `evidence` behind each one, and `resolve_nids.py` merges it
in the same way as `<module>.extra.txt`: edit the file, re-run, and the function
is renamed across all 28 modules at once.

| nid | name | call sites | confidence |
|---|---|---:|---|
| `0x31454993` | `ehsys_get_language` | 334 | high — a 3-bit settings field with a matching setter next to it, mapped onto SDK language ids by `rel_html_view`'s `func_0000039C` |
| `0x8171F765` | `ehsys_get_button_code` | 288 | high — returns one of two adjacent halfwords by arg; the arg-0 result is compared to `PSP_CTRL_CIRCLE` to set `buttonSwap` |
| `0xF1BC43DB` | `ehsys_frame_sync` | 172 | inferred — updates a timer object, then `sceKernelWakeupThread`; called once per main-loop iteration |

## Files

| file | contents |
|---|---|
| `nids/sdk.csv` | `lib, nid, name, verified` — the 47 SDK imports |
| `nids/ehsys.csv` | `index, nid, eboot_vaddr, call_sites, name` — all 1729 engine exports, with how many call sites exist project-wide |
| `nids/modules.csv` | `lib, nid, owner, export_vaddr, call_sites, name` — what each module exports and who calls it |
| `config/symbols/<module>.txt` | splat `symbol_addrs`: that module's stub vram -> name |

Each `config/rel_*.yaml` now points at its symbol file, so re-splitting any
module produces named calls in the disassembly instead of bare addresses.

## What this bought immediately

`rel_html_view` was decompiled before any of this existed, from raw addresses.
Resolution confirmed the entire reading and corrected the vocabulary:

| was | is |
|---|---|
| `func_000007F0(0x100)` "acquire six resources" | `sceUtilityLoadModule(PSP_MODULE_NET_COMMON)` — the ids 0x100..0x106 are the net/http/ssl modules |
| `func_00000808(0x106)` "release" | `sceUtilityUnloadModule` |
| `func_00000798(0x28000)` "160 KB working memory" | `sceSslInit(0x28000)` |
| `func_000007C0(0,0,0,0)` | `sceHttpsInit(0,0,0,0)` |
| `func_00000800(cfg)` "start browser" | `sceUtilityHtmlViewerInitStart(param)` |
| the 0xA8-byte config struct | **`pspUtilityHtmlViewerParam`** — a documented SDK struct, exactly 0xA8 bytes |
| `func_000005A0`'s dispatch | the standard utility-dialog loop: `GetStatus()` -> 2 = `Update()`, 3 = `ShutdownStart()` |

The last row is worth noting: `func_000007E8(2)` was found by pure codegen
reasoning (a dead argument register doubling as a switch constant, see
`src/rel_html_view.c`), and it turns out to be `sceUtilityHtmlViewerUpdate(2)`,
the documented drawing-order argument. The two methods agreed independently.

Re-typing `func_00000470`'s 30 stores against the real `pspUtilityHtmlViewerParam`
field layout is now a straightforward next step, and the same struct appears
wherever the browser is used.

## Corpus provenance

`tools/nid_db/` holds PPSSPP `Core/HLE/*.cpp` files used only as a source of
candidate names; `tools/` is gitignored, and nothing downstream depends on them
once `nids/sdk.csv` exists. To refetch:

```
mkdir -p tools/nid_db && cd tools/nid_db
for f in sceHttp sceNet sceNetInet sceNetApctl sceNetResolver sceSsl \
         sceUtility sceUmd sceJpeg sceIo sceKernelThread sceRtc sceImpose; do
  curl -sfLO "https://raw.githubusercontent.com/hrydgard/ppsspp/master/Core/HLE/$f.cpp"
done
```

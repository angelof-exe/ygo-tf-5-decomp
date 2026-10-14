#!/usr/bin/env python3
"""Measure decompilation progress and draw the decomp-style treemap.

The treemap is the one decomp projects (decomp.dev, objdiff) put in their
README: every rectangle is a piece of code, its AREA is its size in bytes, and
it is GREEN once the C reproduces it byte-for-byte. Modules are the outer
blocks; inside a module that has a src/ file every function is its own block,
so you can see exactly which ones are done.

WHAT COUNTS AS MATCHED. Only what is committed: for every module with a
tracked src/<module>.c, the HEAD version (not the working tree, which may be a
half-edited draft) is compiled with the real mwccpsp and compared function by
function with scripts/mwcc_diff.py. A function is matched when it reports
MATCH. Code sizes come from splat's `nonmatching func_X, 0xSIZE` lines, so
asm/ must have been split (`splat split config/<module>.yaml`). The EBOOT
(modehsys, the shared engine) is shown as one grey block until it is split.

Outputs (all committed, none contain game data):
  docs/progress/treemap.svg     the whole game, light and dark aware
  docs/progress/treemap_started.svg   zoom on the modules with any progress
  docs/progress/progress.json   per-module and per-function numbers
  README.md                     the block between the progress markers

Usage:
  scripts/progress.py            # needs .venv, wibo, mwccpsp, mips binutils
"""
import json
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT_DIR = ROOT / "docs/progress"
WIBO = ROOT / "tools/wibo-bin/wibo"
MWCC = ROOT / "tools/mwccpsp_3.0.1_219/mwccpsp.exe"
FLAGS = ["-O4,s", "-sdatathreshold", "0"]
EBOOT_TEXT = 0x6DE7C  # .text of the decrypted EBOOT (docs/04)

README_START = "<!-- progress:start -->"
README_END = "<!-- progress:end -->"

# Status colours: green = matched ("good"), neutral grey = not yet.
# Validated as a pair with the dataviz palette validator, light and dark.
COLORS = {
    "light": {"surface": "#fcfcfb", "done": "#0ca30c", "todo": "#d6d5d0", "ink": "#0b0b0b", "ink2": "#52514e"},
    "dark": {"surface": "#1a1a19", "done": "#0ca30c", "todo": "#3a3a37", "ink": "#f0efec", "ink2": "#c3c2b7"},
}


def functions(module):
    sizes = {}
    text = ROOT / f"asm/{module}/text.s"
    for line in text.read_text().splitlines():
        m = re.match(r"nonmatching (func_[0-9A-F]+), 0x([0-9A-F]+)", line)
        if m:
            sizes[m.group(1)] = int(m.group(2), 16)
    return sizes


def matched(module, tmp):
    """Functions the committed src/<module>.c matches."""
    rel = f"src/{module}.c"
    if subprocess.run(["git", "ls-files", "--error-unmatch", rel], cwd=ROOT,
                      capture_output=True).returncode != 0:
        return set()
    src = subprocess.run(["git", "show", f"HEAD:{rel}"], cwd=ROOT, capture_output=True, check=True).stdout
    work = tmp / module
    work.mkdir()
    # the source includes headers relative to its own directory in build/mwcc
    (work / f"{module}.c").write_bytes(src)
    for inc in (ROOT / "include").iterdir():
        dest = work / inc.name
        if inc.is_dir():
            shutil.copytree(inc, dest)
        else:
            shutil.copy(inc, dest)
    res = subprocess.run([str(WIBO), str(MWCC), "-c", *FLAGS, "-I-", "-I.", "-o", "out.o", f"{module}.c"],
                         cwd=work, capture_output=True, text=True)
    obj = work / "out.o"
    if res.returncode != 0 or not obj.exists():
        print(f"warning: {module}: committed source does not compile, counting 0\n{res.stdout[-500:]}",
              file=sys.stderr)
        return set()
    diff = subprocess.run([sys.executable, str(ROOT / "scripts/mwcc_diff.py"),
                           str(ROOT / f"asm/{module}/text.s"), str(obj)],
                          capture_output=True, text=True, check=True).stdout
    return set(re.findall(r"^(func_[0-9A-F]+): MATCH", diff, re.M))


# ---------------------------------------------------------------- layout

def squarify(items, x, y, w, h):
    """Bruls et al. squarified treemap. items: [(weight, key)], weight > 0.
    Returns [(key, x, y, w, h)]."""
    items = sorted(items, key=lambda t: -t[0])
    total = sum(v for v, _ in items)
    if total <= 0 or w <= 0 or h <= 0:
        return []
    scale = w * h / total
    areas = [(v * scale, k) for v, k in items]
    out = []

    def worst(row, side):
        s = sum(a for a, _ in row)
        return max(max(side * side * a / (s * s), (s * s) / (side * side * a)) for a, _ in row)

    while areas:
        side = min(w, h)
        row = [areas.pop(0)]
        while areas and worst(row + [areas[0]], side) <= worst(row, side):
            row.append(areas.pop(0))
        s = sum(a for a, _ in row)
        if w >= h:  # lay the row out as a column on the left
            cw = s / h
            cy = y
            for a, k in row:
                ch = a / cw
                out.append((k, x, cy, cw, ch))
                cy += ch
            x, w = x + cw, w - cw
        else:  # as a row on top
            rh = s / w
            cx = x
            for a, k in row:
                rw = a / rh
                out.append((k, cx, y, rw, rh))
                cx += rw
            y, h = y + rh, h - rh
    return out


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def render_svg(data, path, started_only=False):
    W, H, HEAD, PAD = 1200, (520 if started_only else 760), 92, 16
    mods = data["modules"]
    if started_only:
        mods = {n: m for n, m in mods.items() if m["matched_functions"]}
    total = sum(m["bytes"] for m in mods.values())
    done = sum(m["matched_bytes"] for m in mods.values())
    nfun = sum(m["functions"] for m in mods.values())
    nok = sum(m["matched_functions"] for m in mods.values())
    pct = 100 * done / total if total else 0
    boxes = squarify([(m["bytes"], name) for name, m in mods.items()], PAD, HEAD, W - 2 * PAD, H - HEAD - PAD)

    o = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" '
         f'role="img" aria-labelledby="t d" font-family="-apple-system,Segoe UI,Helvetica,Arial,sans-serif">',
         f'<title id="t">Tag Force 5 decompilation progress</title>',
         f'<desc id="d">{pct:.2f}% of code bytes matched ({nok} of '
         f'{nfun} functions). Each block is a module or function, area = code size, '
         f'green = matched byte-for-byte.</desc>',
         "<style>"]
    for mode, sel in (("light", ""), ("dark", "@media (prefers-color-scheme: dark) {")):
        c = COLORS[mode]
        o.append(f"{sel} .bg{{fill:{c['surface']}}} .done{{fill:{c['done']}}} .todo{{fill:{c['todo']}}} "
                 f".gap{{stroke:{c['surface']}}} .ink{{fill:{c['ink']}}} .ink2{{fill:{c['ink2']}}} "
                 f".mod{{fill:none;stroke:{c['surface']};stroke-width:3}}{'}' if sel else ''}")
    o.append("</style>")
    o.append(f'<rect class="bg" width="{W}" height="{H}"/>')
    head = (f"Started modules: {pct:.1f}% matched" if started_only else f"{pct:.2f}% matched")
    o.append(f'<text class="ink" x="{PAD}" y="38" font-size="26" font-weight="600">{head}</text>')
    o.append(f'<text class="ink2" x="{PAD}" y="66" font-size="15">{done:,} of {total:,} code bytes · '
             f'{nok:,} of {nfun:,} functions · '
             f'{data["complete_modules"]} of {len(data["modules"])} modules complete</text>')
    # legend (identity never by colour alone)
    lx = W - PAD - 330
    o.append(f'<rect class="done" x="{lx}" y="26" width="16" height="16" rx="3"/>'
             f'<text class="ink2" x="{lx + 24}" y="39" font-size="14">matched</text>'
             f'<rect class="todo" x="{lx + 110}" y="26" width="16" height="16" rx="3"/>'
             f'<text class="ink2" x="{lx + 134}" y="39" font-size="14">not yet · area = bytes</text>')

    for name, x, y, w, h in boxes:
        m = mods[name]
        o.append(f'<g><title>{esc(name)}: {m["matched_bytes"]:,}/{m["bytes"]:,} bytes, '
                 f'{m["matched_functions"]}/{m["functions"]} functions</title>')
        o.append(f'<rect class="todo" x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}"/>')
        if m.get("function_sizes") and m["matched_functions"]:
            inner = squarify([(sz, fn) for fn, sz in m["function_sizes"].items() if sz > 0], x, y, w, h)
            done_set = set(m["matched"])
            for fn, fx, fy, fw, fh in inner:
                cls = "done" if fn in done_set else "todo"
                o.append(f'<rect class="{cls} gap" stroke-width="0.6" x="{fx:.1f}" y="{fy:.1f}" '
                         f'width="{fw:.1f}" height="{fh:.1f}"><title>{name} {fn}: {m["function_sizes"][fn]} bytes'
                         f'{" — matched" if cls == "done" else ""}</title></rect>')
        o.append(f'<rect class="mod" x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}"/>')
        label = name.replace("rel_", "")
        fs = 14 if w > 110 and h > 36 else 11 if w > 60 and h > 22 else 0
        if fs:
            lab = label if len(label) * fs * 0.6 < w - 8 else label[: max(1, int((w - 8) / (fs * 0.6)) - 1)] + "…"
            two = h > 2.4 * fs + 10 and m["bytes"]
            pw = min(w - 4, len(lab) * fs * 0.62 + 10)
            o.append(f'<rect class="bg" opacity="0.88" rx="3" x="{x + 2:.1f}" y="{y + 2:.1f}" '
                     f'width="{pw:.1f}" height="{(2 * fs + 10) if two else (fs + 8):.1f}"/>')
            o.append(f'<text class="ink" x="{x + 6:.1f}" y="{y + fs + 4:.1f}" font-size="{fs}" '
                     f'font-weight="600">{esc(lab)}</text>')
            if two:
                o.append(f'<text class="ink2" x="{x + 6:.1f}" y="{y + 2 * fs + 8:.1f}" font-size="{fs - 2}">'
                         f'{100 * m["matched_bytes"] / m["bytes"]:.0f}%</text>')
        o.append("</g>")
    o.append("</svg>")
    path.write_text("\n".join(o) + "\n", encoding="utf-8")


def readme_block(data):
    rows = ["| module | functions | code bytes | progress |", "|---|---:|---:|---:|"]
    for name, m in sorted(data["modules"].items(), key=lambda kv: (-kv[1]["matched_bytes"] / max(kv[1]["bytes"], 1), kv[1]["bytes"])):
        if not m["matched_functions"]:
            continue
        pct = 100 * m["matched_bytes"] / m["bytes"]
        mark = " ✅" if m["matched_functions"] == m["functions"] else ""
        rows.append(f"| `{name}` | {m['matched_functions']}/{m['functions']} | "
                    f"{m['matched_bytes']:,}/{m['bytes']:,} | {pct:.1f}%{mark} |")
    pct = 100 * data["matched_bytes"] / data["total_bytes"]
    return "\n".join([
        README_START,
        f"**{pct:.2f}% of all code matched** — {data['matched_functions']:,} of "
        f"{data['total_functions']:,} functions, {data['complete_modules']} of {len(data['modules'])} "
        "modules rebuilt byte-for-byte from C.",
        "",
        '<p align="center"><img src="docs/progress/treemap.svg" alt="Progress treemap: every block is a '
        'module or function, area is code size, green is matched" width="100%"></p>',
        "",
        '<p align="center"><img src="docs/progress/treemap_started.svg" alt="Zoom on the modules that '
        'have been started, one block per function" width="100%"></p>',
        "",
        "Each block is a module (inside started modules, each function); its area is its code size and "
        "it turns green once our C compiles to the identical bytes. `rel_duel_eng`, the rules engine, "
        "is over half the game. Regenerate with `scripts/progress.py`.",
        "",
        *rows,
        README_END,
    ])


def main():
    data = {"modules": {}}
    with tempfile.TemporaryDirectory(prefix="tf5progress") as t:
        tmp = Path(t)
        for text in sorted((ROOT / "asm").glob("rel_*/text.s")):
            mod = text.parent.name
            sizes = functions(mod)
            ok = matched(mod, tmp) & set(sizes)
            data["modules"][mod] = {
                "functions": len(sizes),
                "bytes": sum(sizes.values()),
                "matched_functions": len(ok),
                "matched_bytes": sum(sizes[f] for f in ok),
                "matched": sorted(ok),
                "function_sizes": sizes if ok else {},
            }
            print(f"{mod:20s} {len(ok):5d}/{len(sizes):<5d}", file=sys.stderr)
    data["modules"]["EBOOT (modehsys)"] = {"functions": 0, "bytes": EBOOT_TEXT, "matched_functions": 0,
                                           "matched_bytes": 0, "matched": [], "function_sizes": {}}
    mods = data["modules"].values()
    data["total_bytes"] = sum(m["bytes"] for m in mods)
    data["matched_bytes"] = sum(m["matched_bytes"] for m in mods)
    data["total_functions"] = sum(m["functions"] for m in mods)
    data["matched_functions"] = sum(m["matched_functions"] for m in mods)
    data["complete_modules"] = sum(1 for m in mods if m["functions"] and m["matched_functions"] == m["functions"])

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    render_svg(data, OUT_DIR / "treemap.svg")
    render_svg(data, OUT_DIR / "treemap_started.svg", started_only=True)
    slim = {k: v for k, v in data.items() if k != "modules"}
    slim["modules"] = {n: {k: v for k, v in m.items() if k != "function_sizes"} for n, m in data["modules"].items()}
    (OUT_DIR / "progress.json").write_text(json.dumps(slim, indent=1) + "\n")

    readme = ROOT / "README.md"
    text = readme.read_text(encoding="utf-8")
    block = readme_block(data)
    if README_START in text:
        text = re.sub(re.escape(README_START) + r".*?" + re.escape(README_END), lambda _: block, text, flags=re.S)
    else:
        text = text.replace("## Status\n", "## Status\n\n" + block + "\n", 1)
    readme.write_text(text, encoding="utf-8")
    print(f"total: {data['matched_functions']}/{data['total_functions']} functions, "
          f"{100 * data['matched_bytes'] / data['total_bytes']:.2f}% bytes", file=sys.stderr)


if __name__ == "__main__":
    main()

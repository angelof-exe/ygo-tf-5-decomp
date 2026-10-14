#!/usr/bin/env python3
"""Dump REA/Ghidra pseudocode for every function of a module, in one session.

`rea decompile` re-imports the program into Ghidra on every call (~13 s each),
which is fine for one function and hopeless for a module. This drives REA's
MCP server over stdio instead: open the binary once, list its procedures, and
run `batch_decompile` over them.

Input is the analysis ELF from scripts/prx_to_rea_elf.py (REA refuses raw
PRXs); it is regenerated if missing. Output, per module:

  build/rea/<module>/<addr>.c     Ghidra pseudocode, one file per function
  build/rea/<module>/index.tsv    addr, name, size(if known), status

Addresses are vram == splat's, so build/rea/rel_x/000006bc.c is the
pseudocode of func_000006BC in asm/rel_x/.

The pseudocode is a READING AID, not a draft to compile: Ghidra types, its
`_DAT_` globals and its control flow are not what MWCC was fed. Use it to
understand a function, then write the C against the asm with m2c.

Usage:
  scripts/rea_dump.py rel_movie_viewer [--only 0x6bc,0x34] [--chunk 32]
Requires: `source .rea_env.sh` (Node >= 22.19, GHIDRA_INSTALL_DIR, JDK 21).
"""
import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


class Rea:
    def __init__(self):
        self.p = subprocess.Popen(["rea", "--mcp"], stdin=subprocess.PIPE, stderr=subprocess.DEVNULL,
                                  env={**os.environ, "REA_ANALYSIS_PROVIDER": "ghidra"},
                                  stdout=subprocess.PIPE, text=True, bufsize=1)
        self.next_id = 0
        self.rpc("initialize", {"protocolVersion": "2025-06-18", "capabilities": {},
                                "clientInfo": {"name": "tf5-rea-dump", "version": "1"}})
        self.notify("notifications/initialized")

    def notify(self, method, params=None):
        self.p.stdin.write(json.dumps({"jsonrpc": "2.0", "method": method, "params": params or {}}) + "\n")
        self.p.stdin.flush()

    def rpc(self, method, params):
        self.next_id += 1
        rid = self.next_id
        self.p.stdin.write(json.dumps({"jsonrpc": "2.0", "id": rid, "method": method, "params": params}) + "\n")
        self.p.stdin.flush()
        while True:
            line = self.p.stdout.readline()
            if not line:
                raise RuntimeError("rea --mcp exited")
            msg = json.loads(line)
            if msg.get("id") == rid:
                if "error" in msg:
                    raise RuntimeError(f"{method}: {msg['error']}")
                return msg["result"]
            # progress/log notifications are ignored

    def tool(self, name, args):
        res = self.rpc("tools/call", {"name": name, "arguments": args})
        if res.get("structuredContent") is not None:
            out = res["structuredContent"]
        else:
            text = "".join(c.get("text", "") for c in res.get("content", []))
            try:
                out = json.loads(text)
            except ValueError:
                out = text
        if res.get("isError"):
            raise RuntimeError(f"{name}: {json.dumps(out)[:2000]}")
        return out

    def close(self):
        try:
            self.tool("close_binary", {})
        except Exception:
            pass
        self.p.kill()


def find(obj, pred):
    """Yield every dict inside obj for which pred(dict) holds."""
    if isinstance(obj, dict):
        if pred(obj):
            yield obj
        for v in obj.values():
            yield from find(v, pred)
    elif isinstance(obj, list):
        for v in obj:
            yield from find(v, pred)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("module")
    ap.add_argument("--only", help="comma-separated addresses")
    ap.add_argument("--chunk", type=int, default=32)
    ap.add_argument("--raw", action="store_true", help="print the raw batch result of the first chunk and stop")
    args = ap.parse_args()

    elf = ROOT / f"build/rea/{args.module}.elf"
    if not elf.exists():
        subprocess.run([sys.executable, str(ROOT / "scripts/prx_to_rea_elf.py"), args.module], check=True)
    out_dir = ROOT / f"build/rea/{args.module}"
    out_dir.mkdir(parents=True, exist_ok=True)

    rea = Rea()
    try:
        rea.tool("open_binary", {"path": str(elf)})
        if args.only:
            addrs = [f"0x{int(a, 16):x}" for a in args.only.split(",")]
        else:
            procs = rea.tool("list_procedures", {})
            addrs = []
            for d in find(procs, lambda d: "address" in d and isinstance(d.get("address"), str)):
                kind = (d.get("kind") or d.get("classification") or "")
                if "external" in str(kind) or "thunk" in str(kind):
                    continue
                addrs.append(d["address"])
            if not addrs and isinstance(procs, dict):
                # some versions return {address: name}
                for k in procs.get("procedures", {}) if isinstance(procs.get("procedures"), dict) else []:
                    addrs.append(k)
            # stubs live in .sceStub.text; only real code is worth a dump
            stub_base = min((int(l.split("=")[1].split(";")[0], 16)
                             for l in (ROOT / f"config/symbols/{args.module}.txt").read_text().splitlines()
                             if l.startswith("ehsys_") or l.startswith("sce")), default=1 << 32)
            # Ghidra merges a function into its neighbour when the first one
            # ends in a tail call; splat's boundaries are the ones src/ uses,
            # so add every `glabel func_` from asm/ as well.
            text_s = ROOT / f"asm/{args.module}/text.s"
            if text_s.exists():
                for line in text_s.read_text().splitlines():
                    if line.startswith("glabel func_"):
                        addrs.append("0x" + line.split("_")[1].strip().lower().lstrip("0").rjust(1, "0"))
            addrs = sorted({f"0x{int(a, 16):x}" for a in addrs if int(a, 16) < stub_base}, key=lambda a: int(a, 16))
        print(f"{args.module}: {len(addrs)} functions", file=sys.stderr)

        index = []
        for i in range(0, len(addrs), args.chunk):
            chunk = addrs[i:i + args.chunk]
            res = rea.tool("batch_decompile", {"addresses": chunk})
            if args.raw:
                print(json.dumps(res, ensure_ascii=False, indent=1)[:20000])
                return
            got = {}
            for d in find(res, lambda d: any(isinstance(d.get(k), str) and "{" in d.get(k, "")
                                             for k in ("pseudo_code", "pseudocode", "code", "text"))):
                addr = d.get("address") or d.get("procedure") or d.get("entry")
                code = next(d[k] for k in ("pseudo_code", "pseudocode", "code", "text")
                            if isinstance(d.get(k), str) and "{" in d[k])
                if isinstance(addr, str):
                    got[int(addr, 16)] = (code, d.get("name", ""))
            for a in chunk:
                v = int(a, 16)
                if v in got:
                    code, name = got[v]
                    (out_dir / f"{v:08x}.c").write_text(code, encoding="utf-8")
                    index.append((v, name, "ok"))
                else:
                    index.append((v, "", "missing"))
            print(f"  {min(i + args.chunk, len(addrs))}/{len(addrs)}", file=sys.stderr)
        with open(out_dir / "index.tsv", "w", encoding="utf-8") as f:
            for v, name, st in index:
                f.write(f"0x{v:08X}\t{name}\t{st}\n")
        ok = sum(1 for x in index if x[2] == "ok")
        print(f"{args.module}: wrote {ok}/{len(index)} to {out_dir.relative_to(ROOT)}", file=sys.stderr)
    finally:
        rea.close()


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
Make a splat .s file assemblable by GNU as, by turning the instructions gas does
not know into raw .word directives.

    scripts/asm_prepare.py in.s out.s -- mips-linux-gnu-as -march=mips32r2 ...

WHY. rabbitizer disassembles the full Allegrex instruction set, but binutils
does not implement its custom opcodes — `min`, `max` and the whole VFPU block
are rejected with "opcode not supported on this processor". 20 of the 28 modules
contain at least one, so without this they cannot be reassembled at all.

HOW. Every line splat emits carries the original encoding in its comment:

    /* 3AC 00000358 F0FFBD27 */  min  $a0, $v1, $a0

so an unassemblable line can be replaced by `.word 0x...` with the exact same
bytes. Rather than maintaining a list of which mnemonics binutils supports, this
runs the assembler, rewrites precisely the lines it complained about, and
repeats until it is happy. That way it self-corrects as binutils changes, and it
can never rewrite an instruction gas would have assembled correctly.

The bytes are identical either way, so the resulting object is unaffected — only
the readability of the intermediate file changes, and the intermediate lives in
build/ while asm/ keeps the real disassembly.
"""
import os
import re
import subprocess
import sys
import tempfile

MAX_ROUNDS = 40
# "file.s:1234: Error: opcode not supported on this processor: ..."
ERR = re.compile(r"^[^:]+:(\d+): Error: (opcode not supported|unrecognized opcode|invalid operands)")
# /* rom vram WORD */  mnemonic operands
LINE = re.compile(r"^(\s*)/\* ([0-9A-Fa-f]+) ([0-9A-Fa-f]+) ([0-9A-Fa-f]{8}) \*/(\s*)(.*)$")


def to_word(le_hex):
    """splat prints the 4 bytes in file order; .word wants the value."""
    return int.from_bytes(bytes.fromhex(le_hex), "little")


def main():
    src, dst = sys.argv[1], sys.argv[2]
    assert sys.argv[3] == "--", "usage: asm_prepare.py in.s out.s -- <as> <flags...>"
    as_cmd = sys.argv[4:]

    lines = open(src, errors="replace").read().splitlines(keepends=True)
    rewritten = 0

    for _ in range(MAX_ROUNDS):
        with tempfile.NamedTemporaryFile("w", suffix=".s", delete=False,
                                         dir=os.path.dirname(os.path.abspath(dst))) as fh:
            fh.writelines(lines)
            probe = fh.name
        obj = probe + ".o"
        r = subprocess.run(as_cmd + [probe, "-o", obj], capture_output=True, text=True)
        for f in (probe, obj):
            try:
                os.unlink(f)
            except OSError:
                pass
        if r.returncode == 0:
            break

        bad = sorted({int(m.group(1)) for m in
                      (ERR.match(l) for l in r.stderr.splitlines()) if m})
        if not bad:
            sys.stderr.write(r.stderr)
            sys.exit("asm_prepare: assembler failed for a reason this cannot fix")

        progress = False
        for n in bad:
            m = LINE.match(lines[n - 1])
            if not m:
                continue
            indent, rom, vram, word, _, insn = m.groups()
            lines[n - 1] = (f"{indent}/* {rom} {vram} {word} */  "
                            f".word 0x{to_word(word):08X}  /* {insn.strip()} */\n")
            rewritten += 1
            progress = True
        if not progress:
            sys.stderr.write(r.stderr)
            sys.exit("asm_prepare: could not rewrite the offending lines")
    else:
        sys.exit(f"asm_prepare: still failing after {MAX_ROUNDS} rounds")

    open(dst, "w").writelines(lines)
    if rewritten:
        print(f"asm_prepare: {os.path.basename(src)}: "
              f"{rewritten} Allegrex instruction(s) emitted as .word")


if __name__ == "__main__":
    main()

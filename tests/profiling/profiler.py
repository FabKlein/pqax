import argparse
import re
from pathlib import Path


RESERVED_LOOP_REGISTERS = ("x24", "x25", "x26", "x27", "x28", "x30")


def gen_ubench_name(name,idx):
    return f"{name}_{idx}"

def gen_ubenchmark(name, code, with_padding=True, runtime_loop=False):
    yield f".macro {name}_core"
    for i, c in enumerate(code):
        if "// gap" in c:
            c = "nop"
        yield f"/* {i:02} */ {'':4s}{c}"
    if with_padding:
        yield "padding"
    yield f".endm"
    make_macro = "make_ubench_loop" if runtime_loop else "make_ubench"
    yield f"{make_macro} {name}, nop, {name}_core, nop"

def gen_ubenchmarks(name, code, steps=1, runtime_loop=False):
    lines = len(code)
    for li in range(0, lines + 1, steps):
        yield from gen_ubenchmark(
            gen_ubench_name(name,li), code[:li], runtime_loop=runtime_loop
        )
        yield ""

def gen_c_profiler(name, code, steps=1):
    lines = len(code)
    yield f"ubench_t ubenchs_{name}[] = {{"
    for li in range(0, lines + 1, steps):
        yield f"{'':4s} &ubench_{gen_ubench_name(name,li)},"
    yield "};"
    yield ""
    yield f"char* ubench_{name}_instructions[] = {{"
    for li in range(0, lines, steps):
        yield f"{'':4s} \"{code[li]}\","
    yield "};"
    yield ""
    yield f"const unsigned int num_ubenchs_{name} = {lines};"
    yield ""

def gen_asm_header():
    yield "#include \"ubenchmarks.i\""
    yield "#include \"profiler_macros.i\""
    yield ""

def gen_c_header():
    yield "#include \"profiling.h\""
    yield "#include \"prefix_ubenchs.h\""
    yield ""

def gen_asm(name, code, steps=1, runtime_loop=False):
    yield from gen_asm_header()
    yield f"// Generation mode: {'runtime loop' if runtime_loop else 'flattened'}"
    yield ""
    yield from gen_ubenchmarks(
        name, code, steps=steps, runtime_loop=runtime_loop
    )

def gen_c(name, code, steps=1):
    yield from gen_c_header()
    yield from gen_c_profiler(name, code, steps=1)

def gen_header(name, code, steps=1):
    lines = len(code)
    yield "#ifndef PREFIX_UBENCHS_H"
    yield "#define PREFIX_UBENCHS_H"
    yield ""
    for li in range(0, lines + 1, steps):
        yield f"void ubench_{gen_ubench_name(name,li)}(void*,void*,void*,void*,void*);"
    yield ""
    yield f"extern ubench_t ubenchs_{name}[];"
    yield f"extern char* ubench_{name}_instructions[];"
    yield f"extern const unsigned int num_ubenchs_{name};";


    yield "#endif"

def check_reserved_registers(lines):
    pattern = re.compile(
        r"\b(?:" + "|".join(RESERVED_LOOP_REGISTERS) + r")\b",
        re.IGNORECASE,
    )
    conflicts = [
        (line_number, line)
        for line_number, line in enumerate(lines, start=1)
        if pattern.search(line)
    ]
    if conflicts:
        details = "\n".join(
            f"  asm.txt:{line_number}: {line}" for line_number, line in conflicts
        )
        reserved = ", ".join(RESERVED_LOOP_REGISTERS)
        raise SystemExit(
            f"runtime-loop mode reserves {reserved}; conflicts found:\n{details}"
        )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--loop",
        action="store_true",
        help=(
            "emit one copy of each prefix in an 80-iteration runtime loop; "
            "x24-x27 preserve x0-x3, x30 preserves x4, and x28 is the counter"
        ),
    )
    args = parser.parse_args()

    output_dir = Path(__file__).resolve().parent
    lines = (output_dir / "asm.txt").read_text().splitlines()
    if args.loop:
        check_reserved_registers(lines)

    (output_dir / "prefix_ubenchs.s").write_text(
        '\n'.join(gen_asm("prefix", lines, runtime_loop=args.loop))
    )
    (output_dir / "profiler.c").write_text('\n'.join(gen_c("prefix", lines)))
    (output_dir / "prefix_ubenchs.h").write_text(
        '\n'.join(gen_header("prefix", lines))
    )


if __name__ == "__main__":
    main()

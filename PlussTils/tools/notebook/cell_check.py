"""Compile notebook cells without running them, for an editor that underlines errors as they are typed.

    python3 cell_check.py

It is started by the PlussII extension for VS Code with the environment of a PlussII kernel (from its kernel.json) and
answers requests one JSON line at a time on its standard input:

    {"id": 7, "earlier": ["cell text", ...], "cell": "cell text"}
    -> {"id": 7, "diagnostics": [{"line": 3, "column": 12, "severity": "error", "message": "use of undeclared identifier 'x'"}]}

Lines and columns count from 1. The cell is read the way the kernel runs it: after the cells above it (each without a
final bare expression, which only shows a value), with its own last bare expression shown, against the same prelude and
compiler options. A name a cell defines again is not reported: running a cell again is how a notebook is used, and the
kernel renames such a name when it runs. The prelude is compiled once and kept, so a request takes a fraction of the time
the first one does.
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import pluss_kernel as kernel  # noqa: E402  (the kernel's compiler setup and cell rules, so the two never differ)

DIAGNOSTIC = re.compile(r"^cell:(\d+):(\d+): (error|warning): (.*)$")


def shown_line_shift(cell, text):
    """(line, where the display call's argument begins, how far it moved): the kernel shows a cell's last bare expression
    by wrapping it in its display call, which moves that expression right; a position on it is moved back."""
    before, after = cell.rstrip().split("\n"), text.rstrip().split("\n")
    if before == after:
        return None
    line = len(after)
    call = after[-1].find(kernel.SHOW_CALL)
    if call < 0:
        return None
    argument = call + len(kernel.SHOW_CALL)
    original = before[-1]
    start = kernel.PlussKernel._last_statement_start(original)
    start += len(original[start:]) - len(original[start:].lstrip())
    return line, argument, argument - start


def diagnostics(compiler, earlier, cell):
    ran = [kernel.PlussKernel._declarations_of(text) for text in earlier if text.strip()]
    text = kernel.PlussKernel._show_last_expression(cell)
    shift = shown_line_shift(cell, text)
    report = compiler.check(ran, text)
    found = []
    for line in report.split("\n"):
        match = DIAGNOSTIC.match(line.strip())
        if match is None:
            continue
        message = match.group(4)
        if kernel.REDEFINED.search(line):
            continue  # a cell run again: the kernel renames it
        row, column = int(match.group(1)), int(match.group(2))
        if shift is not None and row == shift[0]:
            column = max(1, column - shift[2]) if column > shift[1] else max(1, shift[1] - shift[2] + 1)
        found.append({"line": row, "column": column, "severity": match.group(3),
                      "message": message.replace("pluss::nb::show(", "").rstrip()})
    return found


def main():
    compiler = kernel.PlussKernel._make_completer()
    if compiler is None:
        print(json.dumps({"id": None, "error": "no clang++ was found to compile cells with"}), flush=True)
        return 1
    compiler.prepare()
    print(json.dumps({"id": None, "ready": True}), flush=True)
    for line in sys.stdin:
        if not line.strip():
            continue
        try:
            request = json.loads(line)
            answer = {"id": request.get("id"), "diagnostics": diagnostics(compiler, request.get("earlier", []), request.get("cell", ""))}
        except Exception as error:  # a request that cannot be answered must not end the service
            answer = {"id": None, "error": str(error)}
        print(json.dumps(answer), flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())

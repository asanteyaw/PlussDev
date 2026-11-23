# PlussII notebook kernel (prototype)

A Jupyter kernel that runs C++ cells against PlussII. Variables, functions and includes stay alive
from one cell to the next. A table or plot is shown as rich output.

    df                      // the last expression of a cell is shown
    pluss::nb::show(chart); // or show anything explicitly

## How it works

`pluss_kernel.py` starts clang-repl once, loads PlussII into it as one shared library and sends it each
cell as an `#include` of a file (clang-repl reads a line at a time, so this keeps a cell together).
What a cell prints returns as its output; a table is returned as HTML and a plot as SVG.

## Try it

You need Python with `ipykernel` and `jupyter_client`, a clang-repl, and a PlussII download.

    python3 install_kernel.py --prefix /path/to/plussii --clang-repl /path/to/clang-repl --sysroot /path/to/sdk

## State of this

A spike, tried on macOS arm64 only, with Homebrew's LLVM 19 clang-repl and an SDK that LLVM 19 can
read (Xcode's newest SDK is too new for it: it fails on `INFINITY` in `<cmath>`). In that setup the
engine starts and runs cells; the first cell, which compiles the PlussII headers, takes several
seconds and later cells are quick. Not done: interrupt, completion, `pluss kernel install`, a
precompiled header for the first cell, `.show()` inline instead of a window, Linux and Windows.

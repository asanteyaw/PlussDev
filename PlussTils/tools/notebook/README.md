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

    pluss kernel install

(or `python3 install_kernel.py`, which takes `--prefix`, `--clang-repl` and `--sysroot`; each is found when left out)

## State of this

A spike, tried on macOS arm64 only, with Homebrew's LLVM 19 clang-repl and an SDK that LLVM 19 can
read (Xcode's newest SDK is too new for it: it fails on `INFINITY` in `<cmath>`). In that setup the
engine starts and runs cells; the first cell, which compiles the PlussII headers, takes several
seconds and later cells are quick. Cells can be re-run (names are rebound), interrupt and crashes restart the engine, `.show()` goes inline, Tab completes and help uses `pluss::tils::doc`. Not done: a precompiled header for the first cell, Linux and Windows.

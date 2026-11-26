# Notebooks (preview)

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussTils Core Manual](core.md)

A notebook runs a program a cell at a time and keeps what earlier cells made. PlussII has a preview of a notebook kernel for Jupyter, so you can write C++ cells, keep your variables from one cell to the next, and see tables and plots in the notebook instead of a separate window. It also runs in the notebook support of editors that use Jupyter kernels.

This is a preview. It comes with the download (under `share/pluss/notebook`) and is registered with one command, `pluss kernel install`. It is tested on macOS (Apple Silicon and Intel) and on Ubuntu 24.04 (x86_64 and ARM64). It does not run on Windows yet.

## How it works

The kernel starts one C++ interpreter (clang-repl) and loads PlussII into it. Each cell you run is compiled in memory and executed; variables, functions and includes from earlier cells stay defined. What the cell prints appears as the cell's output.

| In a cell | The notebook shows |
|---|---|
| a bare expression as the last line, such as `df` or `x * 2.0` | the value, in the shape that suits it: a `DataFrame` as a compact table in the notebook's own colours (a long one shows its first and last five cases); a `Catalog` or a `Registry` as a ragged table, blank where a key has no entry; an `Atlas`, an `Archive` or an `Axel` as a short summary of how many tables are stacked and what they hold; a `Shelf`, a `Stream` or a `Slot` as text, the way the terminal shows it; text for the rest |
| `pluss::nb::show(chart);` | a plot as a picture |
| `pluss::nb::show(df);` | a table |
| anything printed with `std::cout` | the text |
| a compile error | the compiler's message, naming the line of the cell |

A notebook starts as a `main.cpp` does: `<pluss/pluss.h>` is already included, which gives you the core (Series, Note, Timeline, Duration), the table side (DataFrame, Shelf, Catalog) and the plots. Everything else is opt-in as before: include `<pluss/ma.h>`, `<pluss/ai.h>` or `<pluss/tils.h>` (or one part, such as `<pluss/tils/kit/output.h>`) in a cell of its own. The interpreter starts while the notebook opens, so by the time you have written the first cell it is usually ready.

A notebook is meant to be re-run, so the kernel allows it:

| You do | What happens |
|---|---|
| run a cell again that defines a function, a variable or a type again | the new definition replaces the old one for the cells you run from then on |
| stop a running cell (the Interrupt button) | the cell stops and the interpreter restarts; earlier variables are gone and the message says so |
| run code that crashes the interpreter, such as writing through a null pointer | the interpreter restarts and tells you; run the earlier cells again |
| call `.show()` on a table or a plot | it appears under the cell instead of opening a window |
| press Tab, or ask for help on a name | names from your cells and from PlussII are offered; help shows the page that `pluss::tils::doc` finds, from the documentation in the download (the first request includes `<pluss/tils/doc.h>` for you) |

What the preview does not do yet: run on Windows.

## Worked tour

A short session. Each block is one cell, followed by what it shows.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
```

Nothing is shown; `x` now exists for the cells that follow.

```cpp
x.sum().item<double>()
```

```text
6
```

```cpp
#include <pluss/tils/kit/datastore.h>
```

```cpp
auto df = pluss::loader::datastore("sales").frame();
df
```

The table appears as a table in the notebook.

```cpp
auto chart = df.plot().line({"units", "revenue"});
pluss::nb::show(chart);
```

The plot appears as a picture.

```cpp
int answer = ;
```

```text
cell:1:14: error: expected expression
```

The cell stopped with an error, and the kernel is still running: the next cell works.

```cpp
int answer = 6 * 7;
answer
```

```text
42
```

## Getting it

The steps differ a little by system. In each, `<download>` is the folder you extracted from the PlussII download (for the Transient build it is called `plussii-cpp-transient-<system>`), and `pluss` is the program inside its `bin` folder, not a folder of its own. You run it from there:

```text
cd <download>/bin
./pluss kernel install
```

The command finds clang-repl and, on macOS, an SDK that clang-repl can read, builds one shared library of PlussII into `~/.pluss`, and registers the kernel as "PlussII C++" for Jupyter and VS Code. It takes `--clang-repl path` and `--sysroot path` when the defaults are wrong. It needs two things first: the Jupyter kernel packages and a clang-repl.

### macOS (Apple Silicon and Intel)

```bash
python3 -m pip install ipykernel jupyter_client
brew install llvm
cd <download>/bin
./pluss kernel install
```

Homebrew's LLVM provides clang-repl. The installer looks for it in `/opt/homebrew/opt/llvm/bin` and `/usr/local/opt/llvm/bin`, and picks the newest macOS SDK that this clang-repl can read.

### Linux (Ubuntu 24.04, x86_64 and ARM64)

```bash
sudo apt-get install clang-18 libclang-18-dev clang-tools-18 libcurl4-openssl-dev
python3 -m pip install ipykernel jupyter_client
cd <download>/bin
./pluss kernel install
```

On Ubuntu clang-repl is installed under `/usr/lib/llvm-18/bin`, which is often not on the `PATH`. If the installer says it cannot find clang-repl, give it the path:

```bash
./pluss kernel install --clang-repl /usr/lib/llvm-18/bin/clang-repl
```

### Windows

The notebook kernel is not available on Windows yet. PlussII programs and PlussView windows themselves work there; only the notebook preview does not.

### Open it

Start Jupyter (`jupyter lab`) or open a notebook in VS Code and choose the kernel "PlussII C++". The first cell takes a moment while the interpreter starts.


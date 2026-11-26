# Notebooks (preview)

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussTils Core Manual](core.md)

A notebook runs a program a cell at a time and keeps what earlier cells made. PlussII has a preview of a notebook kernel for Jupyter, so you can write C++ cells, keep your variables from one cell to the next, and see tables and plots in the notebook instead of a separate window. It also runs in the notebook support of editors that use Jupyter kernels.

This is a preview. It comes with the download (under `share/pluss/notebook`) and is registered with one command, `pluss kernel install`. It has been tried on Apple Silicon Macs only.

## How it works

The kernel starts one C++ interpreter (clang-repl) and loads PlussII into it. Each cell you run is compiled in memory and executed; variables, functions and includes from earlier cells stay defined. What the cell prints appears as the cell's output.

| In a cell | The notebook shows |
|---|---|
| a bare expression as the last line, such as `df` or `x * 2.0` | the value; a `DataFrame` as a table, text for the rest |
| `pluss::nb::show(chart);` | a plot as a picture |
| `pluss::nb::show(df);` | a table |
| anything printed with `std::cout` | the text |
| a compile error | the compiler's message, naming the line of the cell |

`<pluss/pluss.h>`, `<pluss/tils/doc.h>` and `<pluss/tils/kit/datastore.h>` are already included and `pluss` is available, so a first cell can start with the program. The interpreter starts while the notebook opens, so by the time you have written the first cell it is usually ready.

A notebook is meant to be re-run, so the kernel allows it:

| You do | What happens |
|---|---|
| run a cell again that defines a function, a variable or a type again | the new definition replaces the old one for the cells you run from then on |
| stop a running cell (the Interrupt button) | the cell stops and the interpreter restarts; earlier variables are gone and the message says so |
| run code that crashes the interpreter, such as writing through a null pointer | the interpreter restarts and tells you; run the earlier cells again |
| call `.show()` on a table or a plot | it appears under the cell instead of opening a window |
| press Tab, or ask for help on a name | names from your cells and from PlussII are offered; help shows the page that `pluss::tils::doc` finds, from the documentation in the download |

What the preview does not do yet: run outside macOS.

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

You need the Jupyter kernel packages `ipykernel` and `jupyter_client` (`python3 -m pip install ipykernel jupyter_client`), a clang-repl (macOS: `brew install llvm`), and a PlussII download. Then, from the download:

```text
bin/pluss kernel install
```

The command finds clang-repl and, on macOS, an SDK that clang-repl can read, builds one shared library of PlussII into `~/.pluss`, and registers the kernel as "PlussII C++" for Jupyter and VS Code. It takes `--clang-repl path` and `--sysroot path` when the defaults are wrong.

# Notebooks (preview)

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussTils Core Manual](core.md)

A notebook runs a program a cell at a time and keeps what earlier cells made. PlussII has a preview of a notebook kernel for Jupyter, so you can write C++ cells, keep your variables from one cell to the next, and see tables and plots in the notebook instead of a separate window. It also runs in the notebook support of editors that use Jupyter kernels.

This is a preview. It is in the source tree under `PlussTils/tools/notebook`, it is not installed with the download, and it has been tried on Apple Silicon Macs only.

## How it works

The kernel starts one C++ interpreter (clang-repl) and loads PlussII into it. Each cell you run is compiled in memory and executed; variables, functions and includes from earlier cells stay defined. What the cell prints appears as the cell's output.

| In a cell | The notebook shows |
|---|---|
| a bare expression as the last line, such as `df` or `x * 2.0` | the value; a `DataFrame` as a table, text for the rest |
| `pluss::nb::show(chart);` | a plot as a picture |
| `pluss::nb::show(df);` | a table |
| anything printed with `std::cout` | the text |
| a compile error | the compiler's message, naming the line of the cell |

`<pluss/pluss.h>` is already included and `pluss` is available, so a first cell can start with the program. The interpreter starts while the notebook opens, so by the time you have written the first cell it is usually ready.

What the preview does not do yet: stop a running cell, complete names as you type, show `.show()` windows inline, or run outside macOS.

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
auto df = pluss::table::example("sales");
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

You need Jupyter with its kernel package, a clang-repl, and a PlussII download. On macOS the interpreter must be given a system SDK that it can read: the newest Xcode SDK may be newer than the interpreter understands, in which case use an older one from the command line tools. `PlussTils/tools/notebook/README.md` has the installer and its options.

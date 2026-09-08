<!-- pluss-hand-authored -->
# pluss::note::print

[Back to Note Factory API Reference](../../note_factory_api_reference)

## Signatures

```cpp
std::string  pluss::note::print(const Note& n)
std::string  pluss::note::print(const Note& n, const NotePrintOptions& opt)
std::ostream& pluss::note::print(std::ostream& os, const Note& n,
                                 const NotePrintOptions& opt = {})
```

## Render A `Note` For Inspection

`print()` produces the human-readable representation of a `Note` — the form
this reference uses in every `### Output` block. It mirrors
`pluss::series::print` and `pluss::timeline::print`.

It is a rendering function, not a constructor: it does not build or modify a
`Note`. It lives in the factory namespace because it is the natural
companion to the builders — the thing you reach for immediately after making
one.

The first two overloads **return a `std::string`** and write nothing
themselves; you print or log the result. The third overload writes straight
to `os` and returns the stream for chaining. Streaming a `Note` directly
(`std::cout << n`) also works and is equivalent to
`print(std::cout, n)` with default options.

`print()` has no side effects and is deterministic. An undefined `Note`
renders as `Note(undefined)`.

## The Default Layout

With default options the output is a **header line** followed by the
**values**:

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["one", "two", "three"]
```

- **Header** — `Note(` then `shape`, `dim`, `numel`, and `encoding`
  (`utf8` or `bytes`), then `)`.
- **Values** — laid out by rank:
  - rank `0` (scalar): the single cell alone, e.g. `"Final Exam Review"`.
  - rank `1`: a one-line bracketed list, `["a", "b", "c"]`.
  - rank `2` and `3`: an indented block, one nested list per row.

Each text cell is escaped and wrapped in quotes: `"..."` for UTF-8 mode,
`b"..."` for bytes mode. Control bytes and `DEL` become `\xNN` (with `\n`,
`\t`, … for the named ones) in both modes; in **bytes** mode every byte
`>= 0x80` is escaped too, so the escaped view is always plain ASCII. In
bytes mode with `bytes_mode = Hex`, cells render as `hex[de ad be ef]`
instead.

## `NotePrintOptions` Fields

All fields have defaults; construct the struct and set only what you need.

### Layout

| Field | Type | Default | Effect |
| --- | --- | --- | --- |
| `edge_items` | `std::int64_t` | `2` | When the output is summarised, how many leading and trailing cells to show per axis before the `...` marker. |
| `max_elements` | `std::int64_t` | `256` | Summarise (insert `...`) when `numel` exceeds this **and** `edge_items > 0`. Set to `0` to never summarise. |
| `max_cell_chars` | `std::int64_t` | `96` | Cap on the **inner content** of each cell. Longer content is cut and ends with `...` (a plain cut when the budget is `≤ 3`). Quotes and `hex[]`/`b""` delimiters are added *after* clipping, so a clipped cell is never left unbalanced. Set to `0` to never clip. |

### Header

| Field | Type | Default | Effect |
| --- | --- | --- | --- |
| `metadata_mode` | `NotePrintMetadataMode` | `Full` | `Full` — emit the header, honouring the four `show_*` toggles below. `Compact` — a terse one-liner, `shape` and `encoding` only. `None` — no header line at all. |
| `content_mode` | `NotePrintContentMode` | `Both` | `Both` — header and values. `HeaderOnly` — the header line only. `ValuesOnly` — the values only. |
| `show_shape` | `bool` | `true` | Include `shape=[...]` in a `Full` header. |
| `show_dim` | `bool` | `true` | Include `dim=N` in a `Full` header. |
| `show_numel` | `bool` | `true` | Include `numel=N` in a `Full` header. |
| `show_encoding` | `bool` | `true` | Include `encoding=utf8|bytes` in a `Full` header. |

### Cell text

| Field | Type | Default | Effect |
| --- | --- | --- | --- |
| `bytes_mode` | `NotePrintBytesMode` | `Escaped` | `Escaped` — `b"..."` with C-style escapes. `Hex` — `hex[de ad]`. Affects **bytes-mode** `Note`s only. |
| `quote_strings` | `bool` | `true` | Wrap each cell in quotes. `false` prints the bare cell text. |
| `escape_controls` | `bool` | `true` | Render control and `DEL` bytes as `\xNN` (plus `\n`, `\t`, …); in bytes mode this also escapes every byte `>= 0x80`. `false` emits all of them literally. |

### Related enums

```cpp
enum class NotePrintMetadataMode { Full, Compact, None };
enum class NotePrintContentMode  { Both, HeaderOnly, ValuesOnly };
enum class NotePrintBytesMode    { Escaped, Hex };
```

All three are available unqualified as `pluss::NotePrintMetadataMode`,
`pluss::NotePrintContentMode`, and `pluss::NotePrintBytesMode`.

## Example 1: Default Rendering Of A Column

```cpp
const auto col = pluss::note::make({"one", "two", "three"});  // reused below
std::cout << pluss::note::print(col) << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["one", "two", "three"]
```

## Example 2: A Scalar

```cpp
std::cout << pluss::note::print(pluss::note::make("Final Exam Review")) << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"Final Exam Review"
```

## Example 3: A Two-Dimensional Block

```cpp
const pluss::Note block = pluss::note::make({{"r0c0", "r0c1"}, {"r1c0", "r1c1"}});
std::cout << block << "\n";  // operator<< == print() with default options
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["r0c0", "r0c1"], 
  ["r1c0", "r1c1"]
]
```

## Example 4: Header Only

```cpp
pluss::NotePrintOptions opt;
opt.content_mode = pluss::NotePrintContentMode::HeaderOnly;
std::cout << pluss::note::print(col, opt) << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
```

Just the metadata — useful for logging shape without dumping content.

## Example 5: Values Only

```cpp
pluss::NotePrintOptions opt;
opt.content_mode = pluss::NotePrintContentMode::ValuesOnly;
std::cout << pluss::note::print(col, opt) << "\n";
```

### Output

```text
["one", "two", "three"]
```

## Example 6: No Header At All

```cpp
pluss::NotePrintOptions opt;
opt.metadata_mode = pluss::NotePrintMetadataMode::None;
std::cout << pluss::note::print(col, opt) << "\n";
```

### Output

```text
["one", "two", "three"]
```

`metadata_mode = None` with the default `content_mode = Both` prints the
values with no leading blank line.

## Example 7: Compact Header

```cpp
pluss::NotePrintOptions opt;
opt.metadata_mode = pluss::NotePrintMetadataMode::Compact;
std::cout << pluss::note::print(col, opt) << "\n";
```

### Output

```text
Note(shape=[3], encoding=utf8)
["one", "two", "three"]
```

`dim` and `numel` are dropped — both follow from `shape`.

## Example 8: Summarising A Long Column

```cpp
pluss::NotePrintOptions opt;
opt.edge_items = 1;
opt.max_elements = 3;
std::cout << pluss::note::print(
    pluss::note::make({"a", "b", "c", "d", "e", "f"}), opt) << "\n";
```

### Output

```text
Note(shape=[6], dim=1, numel=6, encoding=utf8)
["a", ..., "f"]
```

`numel` (6) exceeds `max_elements` (3), so only `edge_items` (1) cell at
each end is shown.

## Example 9: Unquoted Cells

```cpp
pluss::NotePrintOptions opt;
opt.quote_strings = false;
std::cout << pluss::note::print(col, opt) << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
[one, two, three]
```

## Example 10: Clipping Wide Cells

```cpp
pluss::NotePrintOptions opt;
opt.max_cell_chars = 12;
std::cout << pluss::note::print(
    pluss::note::make({"short", "a-much-longer-cell-value"}), opt) << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["short", "a-much-..."]
```

`"short"` fits; the long cell is cut to the budget and closed with `...` —
and the closing quote is still there.

## Example 11: Writing Straight To A Stream

```cpp
const auto col = pluss::note::make({"one", "two", "three"});
pluss::note::print(std::cout, col) << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["one", "two", "three"]
```

The three-argument overload writes to the stream and returns it, so `<<`
can continue the line. `std::cout << col` alone does the same with default
options.

## See Also

- [`pluss::note::make`](make.md) / [`pluss::note::bytes`](bytes.md): the
  constructors whose results these examples render.
- [`pluss::note::format`](format.md): build a string; `print` shows a
  `Note`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="perm.html">
    <span class="label">Previous</span>
    <span class="title">perm</span>
  </a>
  <a class="next-link" href="rand.html">
    <span class="label">Next</span>
    <span class="title">rand</span>
  </a>
</div>

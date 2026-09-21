<!-- pluss-hand-authored -->
# Note::split_lines

[Back to Note API Reference](../../note_api_reference)

## Signatures

```cpp
TokenBatch Note::split_lines() const
TokenBatch Note::split_lines(bool keep_ends) const
```

## Split Each Cell Into Lines

`split_lines()` breaks each text cell at line terminators and returns the
lines as a `TokenBatch`. Reach for it when the cells hold multi-line content
— prompts, templates, markdown, copied terminal output, document sections —
and you want line-level structure.

A line terminator is `\n`, a lone `\r`, or the pair `\r\n` (consumed as a
single break). By default the terminators are removed from the returned line
tokens; `split_lines(true)` keeps each line's terminator attached (a `\r\n`
line keeps both bytes), which is what you want if you intend to rejoin the
lines and reproduce the original exactly.

### Trailing terminator and blank lines

A terminator at the very end of a cell ends the last line rather than
starting an empty one, so `"a\nb\n"` yields two lines, not three. A blank
line **in the middle** is a real empty line and is kept: `"a\n\nc"` yields
`"a"`, `""`, `"c"`. An empty cell yields no lines at all.

### The `TokenBatch` result

All lines from all cells are concatenated into the rank-`1` Note
`batch.tokens()`. The rank-`1` Int64 Note `batch.offsets()` has length
`numel + 1` and marks each cell's run: cell `r` owns `tokens.at(k)` for `k`
in `[offsets[r], offsets[r + 1])`. A cell
that produced no lines contributes an empty range.

## Example 1: Basic Line Split

```cpp
const auto batch =
    pluss::make_note({"line1\nline2\nline3", "solo"}).split_lines();

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().integers().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["line1", "line2", "line3", "solo"]
offsets: [0, 3, 4]
```

## Example 2: A Trailing Newline Adds No Empty Line

```cpp
const auto batch = pluss::make_note({"a\nb\n"}).split_lines();
std::cout << pluss::note::print(batch.tokens()) << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["a", "b"]
```

## Example 3: A Blank Line In The Middle Is Kept

```cpp
const auto batch = pluss::make_note({"a\n\nc"}).split_lines();
std::cout << pluss::note::print(batch.tokens()) << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["a", "", "c"]
```

## Example 4: `\r\n` Is One Break

```cpp
const auto batch = pluss::make_note({"a\r\nb"}).split_lines();
std::cout << pluss::note::print(batch.tokens()) << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["a", "b"]
```

The `\r` and `\n` are consumed together; neither remains in a token.

## Example 5: Keeping The Terminators

```cpp
const auto batch = pluss::make_note({"a\nb\n"}).split_lines(true);
std::cout << pluss::note::print(batch.tokens()) << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["a\n", "b\n"]
```

Each line keeps its `\n` (shown by `print` as the escape `\n`).

## Example 6: An Empty Cell Produces No Lines

```cpp
const auto batch = pluss::make_note({""}).split_lines();

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().integers().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[0], dim=1, numel=0, encoding=utf8)
[]
offsets: [0, 0]
```

The one input cell maps to the empty range `[0, 0)`.

## See Also

- [`pluss::note::from_lines`](../note_factory/from_lines.md): the factory
  that splits a **single** string into a 1-D `Note` (not a `TokenBatch`).
- [`Note::split`](split.md): split on an arbitrary literal delimiter.
- [`Note::split_whitespace`](split_whitespace.md): split on whitespace
  runs.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="split_chars.html">
    <span class="label">Previous</span>
    <span class="title">split_chars</span>
  </a>
  <a class="next-link" href="split_whitespace.html">
    <span class="label">Next</span>
    <span class="title">split_whitespace</span>
  </a>
</div>

<!-- pluss-hand-authored -->
# pluss::note::from_lines

[Back to Note Factory API Reference](../../note_factory_api_reference)

## Signatures

```cpp
Note pluss::note::from_lines(string_view text, bool keep_ends = false,
                             bool drop_empty = false)
```

## Split One String Into A Column Of Lines

`from_lines()` takes a single string and returns a rank-`1` `Note` with one
cell per line. It is the standard way to turn a file's contents, a
multi-line message, or a block of log text into an addressable text column
that `Note` methods can then process row by row.

Lines are separated by the newline character `\n`. A `\r` before a `\n` is
**not** stripped — carriage returns stay in the cell text; normalise line
endings first if that matters. The final segment is emitted whether or not
`text` ends with `\n`.

### Parameters

- **`keep_ends`** — when `false` (default) the `\n` separators are removed
  from every cell. When `true`, each cell keeps its trailing `\n` (the last
  segment keeps one only if `text` ended with `\n`). Use `keep_ends = true`
  when you intend to re-join the lines and want the original breaks back
  exactly.
- **`drop_empty`** — when `true`, empty segments (a `\n` immediately
  followed by another `\n`, or a leading/trailing blank line) are omitted
  entirely, so blank lines never become cells. Default `false` keeps them as
  `""` cells.

The result is always rank `1` and **UTF-8 mode**. An empty input yields a
zero-length `Note`.

Splitting is on raw `\n` bytes, so this is safe for UTF-8 text — multi-byte
characters are never split.

## Example 1: Basic Line Split

```cpp
std::cout << pluss::note::print(pluss::note::from_lines("alpha\nbeta\ngamma"))
          << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["alpha", "beta", "gamma"]
```

Three lines, separators removed.

## Example 2: A Trailing Newline Adds No Empty Cell

```cpp
const pluss::Note n = pluss::note::from_lines("alpha\nbeta\n");
std::cout << n << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["alpha", "beta"]
```

The `\n` at the end terminates the last line rather than starting an empty
one.

## Example 3: Blank Lines Become Empty Cells By Default

```cpp
pluss::note::print(std::cout, pluss::note::from_lines("alpha\n\ngamma")) << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["alpha", "", "gamma"]
```

The `\n\n` produces an empty middle cell.

## Example 4: Dropping Blank Lines

```cpp
std::cout << pluss::note::print(
    pluss::note::from_lines("alpha\n\ngamma", false, true)) << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["alpha", "gamma"]
```

`drop_empty = true` removes the empty segment.

## Example 5: Keeping The Line Endings

```cpp
const pluss::Note n = pluss::note::from_lines("alpha\nbeta", true);
std::cout << n << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["alpha\n", "beta"]
```

Each interior line keeps its `\n`. `"beta"` has none because the input did
not end with one. (`print` shows the newline as the escape `\n`; the cell
holds a real newline byte.)

## Example 6: An Empty Input

```cpp
std::cout << pluss::note::print(pluss::note::from_lines("")) << "\n";
```

### Output

```text
Note(shape=[0], dim=1, numel=0, encoding=utf8)
[]
```

A zero-length rank-`1` `Note`, not a scalar.

## See Also

- [`pluss::note::from_words`](from_words.md): split on runs of whitespace
  instead of on line breaks.
- [`pluss::note::make`](make.md): build a column from strings you already
  hold separately.
- [`pluss::note::format`](format.md): assemble a string; pair with
  `from_lines` to round-trip.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="format.html">
    <span class="label">Previous</span>
    <span class="title">format</span>
  </a>
  <a class="next-link" href="from_words.html">
    <span class="label">Next</span>
    <span class="title">from_words</span>
  </a>
</div>

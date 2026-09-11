<!-- pluss-hand-authored -->
# Note::split

[Back to Note API Reference](../../note_api_reference)

## Signatures

```cpp
TokenBatch Note::split(std::string_view delim) const
TokenBatch Note::split(std::string_view delim, std::int64_t limit) const
```

## Split Each Cell On A Literal Delimiter

`split(delim)` cuts each text cell at every occurrence of the literal string
`delim` and returns the pieces as a `TokenBatch`. It is the delimiter-exact
structural splitter — the right tool for CSV-ish fields, `key:value` pairs,
`a -> b` arrows, and other formats with a known separator. When the boundary
is "any run of whitespace," use [`split_whitespace`](split_whitespace.md);
when it is a line break, use [`split_lines`](split_lines.md); when it is a
pattern, use `regex_split`.

`delim` is matched **literally** (no regex) and must be **non-empty** — an
empty delimiter throws `pluss::Error`. It may be more than one character
(`" -> "` is a single four-byte delimiter).

### Empty fields are kept

`split` is a field splitter, not a token filter. Consecutive delimiters, a
leading delimiter, and a trailing delimiter each produce an **empty** piece:
`"a,,b"` splits into `"a"`, `""`, `"b"`, and `",lead"` splits into `""`,
`"lead"`. A cell that does not contain `delim` yields one piece: the whole
cell. (Use `split_whitespace`, which collapses runs, when empty fields are
noise.)

### `limit`

`limit` caps the number of splits, not the number of pieces. After `limit`
splits the rest of the cell is emitted as one final piece: `split(",", 2)`
on `"a,b,c,d"` gives `"a"`, `"b"`, `"c,d"` (2 splits, 3 pieces). `limit = 0`
emits the whole cell unsplit; a negative `limit` (the default) means
unlimited.

### The `TokenBatch` result

All pieces from all cells are concatenated into one flat rank-`1` token
`Note` (`batch.tokens()`); `batch.offsets()` (length `numel + 1`) marks each
cell's run — cell `r` is `tokens()[offsets()[r] .. offsets()[r+1])`. Every
cell contributes at least one piece, so no offset range is empty.

## Example 1: Comma-Separated Fields

```cpp
const auto batch = pluss::make_note({"a,b,c", "x,y", "solo"}).split(",");

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[6], dim=1, numel=6, encoding=utf8)
["a", "b", "c", "x", "y", "solo"]
offsets: [0, 3, 5, 6]
```

Cell `0` → `[0, 3)`, cell `1` → `[3, 5)`, cell `2` (no comma) → `[5, 6)`, one
piece.

## Example 2: Empty Fields Are Preserved

```cpp
const auto batch =
    pluss::make_note({"a,,b", ",lead", "trail,"}).split(",");

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[7], dim=1, numel=7, encoding=utf8)
["a", "", "b", "", "lead", "trail", ""]
offsets: [0, 3, 5, 7]
```

The doubled comma, the leading comma, and the trailing comma each yield an
empty piece.

## Example 3: A Multi-Character Delimiter

```cpp
const auto batch =
    pluss::make_note({"left -> right -> end"}).split(" -> ");
std::cout << pluss::note::print(batch.tokens()) << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["left", "right", "end"]
```

## Example 4: Delimiter Absent

```cpp
const auto batch = pluss::make_note({"no delimiter here"}).split(";");
std::cout << pluss::note::print(batch.tokens()) << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["no delimiter here"]
```

## Example 5: Capping The Number Of Splits

```cpp
const auto batch = pluss::make_note({"a,b,c,d,e"}).split(",", 2);
std::cout << pluss::note::print(batch.tokens()) << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["a", "b", "c,d,e"]
```

Two splits, so three pieces; everything after the second comma stays in the
last piece.

## Example 6: An Empty Delimiter Is Rejected

```cpp
try {
  pluss::make_note({"x"}).split("");
} catch (const pluss::Error& e) {
  const std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
split: delimiter must be non-empty
```

## See Also

- [`Note::split_whitespace`](split_whitespace.md): split on runs of
  whitespace, dropping empty fields.
- [`Note::split_lines`](split_lines.md): split on line breaks.
- `pluss::coden::Note::regex_split`: split on a regular expression.
- [`Note::tokenize_words`](tokenize_words.md): word tokens rather than
  delimiter fields.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort.html">
    <span class="label">Previous</span>
    <span class="title">sort</span>
  </a>
  <a class="next-link" href="split_lines.html">
    <span class="label">Next</span>
    <span class="title">split_lines</span>
  </a>
</div>

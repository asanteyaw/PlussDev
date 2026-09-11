<!-- pluss-hand-authored -->
# Note::lengths_bytes

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<std::int64_t> Note::lengths_bytes() const
```

## Measure Every Cell In Bytes

`lengths_bytes()` returns the storage size of each text cell, counted in
**bytes** (UTF-8 code units), not in characters or grapheme clusters. Use it
to budget buffers, enforce field-width limits, spot outliers before a costly
parse, or turn a `Note` into a numeric feature.

For pure ASCII text the byte count equals the character count. For anything
outside ASCII they differ: `"café"` is four characters but five bytes (`é`
takes two), and `"日本語"` is three characters but nine bytes. When you need
the character count, use `lengths_chars()`; when you need on-screen width,
neither is exact — that requires grapheme and East-Asian-width analysis.

### Return value

Like [`contains`](contains.md), this does **not** return a `Note`. It is a
flat `std::vector<std::int64_t>` with one entry per cell, in row-major order,
length equal to `numel()`. An empty cell measures `0`. Whitespace and control
bytes count like any other byte, so trim with [`strip`](strip.md) first if
boundary padding should not be counted.

The scan is linear in total input size.

## Example 1: ASCII — Bytes Equal Characters

```cpp
const auto lens = pluss::make_note({"", "a", "abc"}).lengths_bytes();
std::cout << "[";
for (std::size_t i = 0; i < lens.size(); ++i)
  std::cout << (i ? ", " : "") << lens[i];
std::cout << "]\n";
```

### Output

```text
[0, 1, 3]
```

## Example 2: Multi-Byte UTF-8 — Bytes Exceed Characters

```cpp
const auto lens =
    pluss::make_note({"café", "日本語", "🎯"}).lengths_bytes();
std::cout << "[";
for (std::size_t i = 0; i < lens.size(); ++i)
  std::cout << (i ? ", " : "") << lens[i];
std::cout << "]\n";
```

### Output

```text
[5, 9, 4]
```

`"café"` is 5 bytes (4 characters), `"日本語"` is 9 bytes (3 characters), and
the emoji `"🎯"` is a single 4-byte character.

## Example 3: Boundary Whitespace Counts — Strip First If It Should Not

```cpp
const auto lens =
    pluss::make_note({"  id-01  ", "\tAB\t"}).strip().lengths_bytes();
std::cout << "[";
for (std::size_t i = 0; i < lens.size(); ++i)
  std::cout << (i ? ", " : "") << lens[i];
std::cout << "]\n";
```

### Output

```text
[5, 2]
```

After [`strip`](strip.md), only `"id-01"` (5) and `"AB"` (2) remain.

## Example 4: Higher-Rank Input Is Flattened Row-Major

```cpp
const auto lens =
    pluss::make_note({{"a", "bb"}, {"ccc", "dddd"}}).lengths_bytes();
std::cout << "[";
for (std::size_t i = 0; i < lens.size(); ++i)
  std::cout << (i ? ", " : "") << lens[i];
std::cout << "]\n";
```

### Output

```text
[1, 2, 3, 4]
```

## See Also

- `pluss::coden::Note::lengths_chars`: the same measurement counted in
  Unicode characters rather than bytes.
- [`Note::strip`](strip.md): remove boundary whitespace before measuring.
- [`Note::contains`](contains.md): the other cell-wise inspector that
  returns a flat vector instead of a `Note`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="keep_printable.html">
    <span class="label">Previous</span>
    <span class="title">keep_printable</span>
  </a>
  <a class="next-link" href="lengths_chars.html">
    <span class="label">Next</span>
    <span class="title">lengths_chars</span>
  </a>
</div>

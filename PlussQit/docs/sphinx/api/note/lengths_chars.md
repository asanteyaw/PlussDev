<!-- pluss-hand-authored -->
# Note::lengths_chars

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::lengths_chars() const
```

## Measure Every Cell In Unicode Characters

`lengths_chars()` returns the length of each text cell counted in **Unicode
code points** (characters), not bytes. Use it when a length limit is
expressed in characters — a headline budget, a form field, a fixed-width
label — rather than in storage.

The difference from [`lengths_bytes`](lengths_bytes.md) is UTF-8 width:
`"café"` is 4 characters but 5 bytes (`é` is 2 bytes), and `"日本語"` is 3
characters but 9 bytes. Pure ASCII text has equal character and byte counts.
For a base letter plus its combining marks counted as one unit, use
[`lengths_graphemes`](lengths_graphemes.md); for storage size, use
`lengths_bytes`.

### Return value

An Int64 `Note` with the receiver's shape, one native count per cell. An empty
cell measures `0`; `integers()` exposes a borrowed contiguous host view.

## Example 1: ASCII — Characters Equal Bytes

```cpp
const auto lens = pluss::make_note({"", "a", "abc"}).lengths_chars();
std::cout << "[";
for (std::size_t i = 0; i < lens.integers().size(); ++i)
  std::cout << (i ? ", " : "") << lens.integers()[i];
std::cout << "]\n";
```

### Output

```text
[0, 1, 3]
```

## Example 2: Multi-Byte UTF-8 — Characters Below Bytes

```cpp
const auto lens =
    pluss::make_note({"café", "日本語", "🎯"}).lengths_chars();
std::cout << "[";
for (std::size_t i = 0; i < lens.integers().size(); ++i)
  std::cout << (i ? ", " : "") << lens.integers()[i];
std::cout << "]\n";
```

### Output

```text
[4, 3, 1]
```

`"café"` is 4 characters, `"日本語"` is 3, and the emoji is a single
character.

## Example 3: Contrast With `lengths_bytes`

```cpp
const auto bytes = pluss::make_note({"café"}).lengths_bytes();
std::cout << "[" << bytes[0] << "]\n";
```

### Output

```text
[5]
```

Same cell, 5 bytes versus 4 characters.

## Example 4: Higher-Rank Input Is Flattened Row-Major

```cpp
const auto lens =
    pluss::make_note({{"a", "bb"}, {"日本", "x"}}).lengths_chars();
std::cout << "[";
for (std::size_t i = 0; i < lens.integers().size(); ++i)
  std::cout << (i ? ", " : "") << lens.integers()[i];
std::cout << "]\n";
```

### Output

```text
[1, 2, 2, 1]
```

## See Also

- [`Note::lengths_bytes`](lengths_bytes.md): storage size in UTF-8 bytes.
- [`Note::lengths_graphemes`](lengths_graphemes.md): grapheme-cluster count
  (combining marks folded into their base).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lengths_bytes.html">
    <span class="label">Previous</span>
    <span class="title">lengths_bytes</span>
  </a>
  <a class="next-link" href="lengths_graphemes.html">
    <span class="label">Next</span>
    <span class="title">lengths_graphemes</span>
  </a>
</div>

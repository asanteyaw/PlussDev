<!-- pluss-hand-authored -->
# Note::lengths_graphemes

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<std::int64_t> Note::lengths_graphemes() const
```

## Measure Every Cell In Grapheme Clusters

`lengths_graphemes()` returns the length of each text cell counted in
**extended grapheme clusters** — "what a reader would call a character."
A base letter and the combining marks that attach to it count as **one**, so
this is the measurement to use for cursor movement, column alignment, and
"visible length" limits.

The difference from [`lengths_chars`](lengths_chars.md) appears whenever text
uses combining marks: `"e"` followed by U+0301 (combining acute accent) is
**one** grapheme but **two** code points. Precomposed text (`"café"` written
with a single `é` code point) counts the same either way.

Segmentation follows the Unicode default grapheme-break algorithm (UAX #29)
via the library's Unicode backend. One known limitation: ZWJ emoji sequences
(a family emoji, a flag, a profession emoji) are **not** joined into a single
cluster by the current backend, so they count as several graphemes. When the
Unicode backend is unavailable, this method falls back to the code-point
count.

### Return value

Not a `Note`: a flat `std::vector<std::int64_t>` with one entry per cell, in
row-major order, length `numel()`. An empty cell measures `0`.

## Example 1: A Combining Mark Counts As One

```cpp
const std::string e_acute = "e\xCC\x81";  // "e" + U+0301
const auto lens =
    pluss::make_note({e_acute, "abc"}).lengths_graphemes();
std::cout << "[";
for (std::size_t i = 0; i < lens.size(); ++i)
  std::cout << (i ? ", " : "") << lens[i];
std::cout << "]\n";
```

### Output

```text
[1, 3]
```

The decomposed `é` is one grapheme; `"abc"` is three.

## Example 2: Contrast With `lengths_chars`

```cpp
const std::string e_acute = "e\xCC\x81";
const auto chars = pluss::make_note({e_acute}).lengths_chars();
std::cout << "[" << chars[0] << "]\n";
```

### Output

```text
[2]
```

Same cell: 2 code points, 1 grapheme.

## Example 3: Precomposed Text Counts The Same As Characters

```cpp
const auto lens =
    pluss::make_note({"café", "日本語"}).lengths_graphemes();
std::cout << "[";
for (std::size_t i = 0; i < lens.size(); ++i)
  std::cout << (i ? ", " : "") << lens[i];
std::cout << "]\n";
```

### Output

```text
[4, 3]
```

## See Also

- [`Note::lengths_chars`](lengths_chars.md): Unicode code-point count.
- [`Note::lengths_bytes`](lengths_bytes.md): storage size in UTF-8 bytes.
- [`Note::to_nfc`](to_nfc.md): fold combining sequences to precomposed form
  so `lengths_chars` and `lengths_graphemes` agree.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lengths_chars.html">
    <span class="label">Previous</span>
    <span class="title">lengths_chars</span>
  </a>
  <a class="next-link" href="lower.html">
    <span class="label">Next</span>
    <span class="title">lower</span>
  </a>
</div>

<!-- pluss-hand-authored -->
# Note::strip

[Back to Note API Reference](../../note_api_reference)

## Signatures

```cpp
Note Note::strip() const
Note Note::strip(std::string_view chars) const
```

## Trim Characters From Both Ends Of Every Cell

`strip()` returns a new `Note`, same shape as the input, with leading and
trailing whitespace removed from each cell. `strip(chars)` instead removes
any character that appears in the `chars` set. Use it to clean boundary noise
before comparison, tokenization, measurement, or display — padded fields,
wrapping markers (`##title##`), stray delimiters at the ends of a
concatenation.

### `strip()` — whitespace

The no-argument form removes the ASCII whitespace bytes space, `\t`, `\n`,
`\r`, `\f`, `\v` from both ends. Whitespace **inside** the cell is left
exactly as-is — `strip()` is a boundary trim, not a normalizer. Combine with
[`replace`](replace.md) or a tokenizer if interior spacing also needs work.

### `strip(chars)` — a character set

`chars` is treated as a **set of bytes**: every position at either end whose
byte is in the set is removed, and trimming stops at the first byte that is
not. Order and repetition in `chars` do not matter. So `strip("#")` removes
runs of `#` from the ends only (`"##a#b##"` → `"a#b"`), and `strip("xy-")`
removes any mix of `x`, `y`, `-` from the ends (`"xy-payload-yx"` →
`"payload"`). Because the set is byte-based, pass only ASCII characters in
`chars`; a multi-byte character there would match its individual bytes.

A cell made entirely of strippable characters becomes empty. The operation
is shape-preserving, cell-independent, and returns a fresh `Note`.

## Example 1: Trim Whitespace

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"  Ada Lovelace  ", "\tGRACE HOPPER\t", "Linus"}).strip())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Ada Lovelace", "GRACE HOPPER", "Linus"]
```

## Example 2: Interior Whitespace Is Kept

```cpp
const pluss::Note n = pluss::make_note({"a\tb  c"}).strip();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["a\tb  c"]
```

There is no leading or trailing whitespace, so the cell is unchanged; the
tab and double space between the letters stay.

## Example 3: Strip A Character Set

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"##alpha##", "#beta", "gamma##", "#a#b#"}).strip("#"))
    << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["alpha", "beta", "gamma", "a#b"]
```

Only the `#` runs at the ends go; the `#` between `a` and `b` stays.

## Example 4: A Set Of Several Characters

```cpp
pluss::note::print(std::cout,
                   pluss::make_note({"xy-payload-yx"}).strip("xy-"))
    << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["payload"]
```

Any mix of `x`, `y`, `-` is trimmed from both ends until a different byte is
reached.

## Example 5: Fully Strippable Cells Become Empty

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"   ", "###"}).strip().strip("#"))
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["", ""]
```

## Example 6: Shape Is Preserved

```cpp
std::cout << pluss::note::print(
    pluss::make_note({{"  a  ", "  b  "}}).strip())
    << "\n";
```

### Output

```text
Note(shape=[1, 2], dim=2, numel=2, encoding=utf8)
[
  ["a", "b"]
]
```

## See Also

- [`Note::lower`](lower.md): case-fold; commonly chained with `strip` for
  full normalization.
- [`Note::remove_all`](remove_all.md): delete a substring **anywhere**, not
  just at the boundaries.
- `pluss::coden::Note::lstrip` / `rstrip`: trim one end only.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="starts_with.html">
    <span class="label">Previous</span>
    <span class="title">starts_with</span>
  </a>
  <a class="next-link" href="swapcase.html">
    <span class="label">Next</span>
    <span class="title">swapcase</span>
  </a>
</div>

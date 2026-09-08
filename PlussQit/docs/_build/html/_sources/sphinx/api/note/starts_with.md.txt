<!-- pluss-hand-authored -->
# Note::starts_with

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<std::uint8_t> Note::starts_with(std::string_view prefix) const
```

## Test Every Cell For A Literal Prefix

`starts_with(prefix)` reports, for each text cell, whether it begins with the
literal string `prefix`. It is the anchored-at-the-start companion of
[`contains`](contains.md): use it to route rows by a tag, a scheme, or a
key prefix, when only a match at position 0 counts.

The test is a **literal byte comparison** — no regex, no case folding. Fold
case first with [`lower`](lower.md) for a case-insensitive check.

### Return value

Like [`contains`](contains.md), the result is **not** a `Note`. It is a flat
`std::vector<std::uint8_t>` with one entry per cell — `1` where the cell
starts with `prefix`, `0` otherwise — in row-major order, length equal to
`numel()`.

### Edge cases

- An **empty `prefix`** is a prefix of every cell, so every entry is `1`.
- A `prefix` longer than a cell cannot match: its entry is `0`.

## Example 1: Route By Prefix

```cpp
const auto mask =
    pluss::make_note({"the end", "The end", "in the end"}).starts_with("the");
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[1, 0, 0]
```

Only the first cell starts with the exact bytes `the`; `"The end"` differs in
case, and in `"in the end"` the match is not at position 0.

## Example 2: An Empty Prefix Matches Everything

```cpp
const auto mask = pluss::make_note({"x", ""}).starts_with("");
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[1, 1]
```

## Example 3: A Prefix Longer Than The Cell

```cpp
const auto mask = pluss::make_note({"ab"}).starts_with("abcd");
std::cout << "[" << static_cast<int>(mask[0]) << "]\n";
```

### Output

```text
[0]
```

## Example 4: Higher-Rank Input Is Flattened Row-Major

```cpp
const auto mask =
    pluss::make_note({{"img_1", "img_2"}, {"vid_1", "img_3"}}).starts_with("img_");
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[1, 1, 0, 1]
```

## See Also

- [`Note::ends_with`](ends_with.md): the anchored-at-the-end counterpart.
- [`Note::contains`](contains.md): match anywhere in the cell.
- `pluss::coden::Note::remove_prefix` / `ensure_prefix`: act on the prefix
  rather than test for it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="split_whitespace.html">
    <span class="label">Previous</span>
    <span class="title">split_whitespace</span>
  </a>
  <a class="next-link" href="strip.html">
    <span class="label">Next</span>
    <span class="title">strip</span>
  </a>
</div>

<!-- pluss-hand-authored -->
# Note::ends_with

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<std::uint8_t> Note::ends_with(std::string_view suffix) const
```

## Test Every Cell For A Literal Suffix

`ends_with(suffix)` reports, for each text cell, whether it ends with the
literal string `suffix`. It is the anchored-at-the-end companion of
[`starts_with`](starts_with.md) and [`contains`](contains.md): use it for
extension checks, terminal-punctuation checks, or unit suffixes.

The test is a **literal byte comparison** — no regex, no case folding. For a
case-insensitive extension check (`.md` vs `.MD`), fold case first with
[`lower`](lower.md).

### Return value

Not a `Note`: a flat `std::vector<std::uint8_t>`, one entry per cell (`1` /
`0`), row-major, length `numel()`.

### Edge cases

- An **empty `suffix`** is a suffix of every cell → all `1`.
- A `suffix` longer than a cell → `0`.

## Example 1: An Extension Check

```cpp
const auto mask =
    pluss::make_note({"report.md", "notes.MD", "data.csv"}).ends_with(".md");
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[1, 0, 0]
```

`"notes.MD"` differs in case; use `.lower().ends_with(".md")` to accept it.

## Example 2: An Empty Suffix Matches Everything

```cpp
const auto mask = pluss::make_note({"x", ""}).ends_with("");
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[1, 1]
```

## Example 3: A Suffix Longer Than The Cell

```cpp
const auto mask = pluss::make_note({"ab"}).ends_with("abcd");
std::cout << "[" << static_cast<int>(mask[0]) << "]\n";
```

### Output

```text
[0]
```

## Example 4: Higher-Rank Input Is Flattened Row-Major

```cpp
const auto mask =
    pluss::make_note({{"a.py", "b.py"}, {"c.js", "d.py"}}).ends_with(".py");
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

- [`Note::starts_with`](starts_with.md): the anchored-at-the-start
  counterpart.
- [`Note::contains`](contains.md): match anywhere in the cell.
- `pluss::coden::Note::remove_suffix` / `ensure_suffix`: act on the suffix
  rather than test for it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count_substring.html">
    <span class="label">Previous</span>
    <span class="title">count_substring</span>
  </a>
  <a class="next-link" href="hex_decode.html">
    <span class="label">Next</span>
    <span class="title">hex_decode</span>
  </a>
</div>

<!-- pluss-hand-authored -->
# Note::contains

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<std::uint8_t> Note::contains(std::string_view needle) const
```

## Test Every Cell For A Literal Substring

`contains(needle)` reports, for each text cell, whether `needle` occurs
anywhere inside it. It is the predicate half of the substring family: use it
to build a mask for filtering, counting, or branching, when the question is
"which cells mention this?" rather than "rewrite the matches" (that is
[`replace`](replace.md) / [`remove_all`](remove_all.md)) or "pull the matches
out" (that is [`regex_findall`](regex_findall.md)).

The match is a **literal byte-substring** search — no regular-expression
metacharacters, no word boundaries, no normalization. `needle` and the cell
are compared exactly as stored, so the test is **case-sensitive** and
byte-oriented; fold case first with [`lower`](lower.md) when you want a
case-insensitive check.

### Return value

The result is **not** a `Note`. It is a flat `std::vector<std::uint8_t>` with
one entry per cell — `1` where `needle` was found, `0` where it was not — in
row-major order. For a rank-`0` `Note` the vector has one entry; for a
rank-`2` or rank-`3` `Note` the cells are flattened in the same order
[`print`](../../note_api_reference.md) walks them. The length always equals
`numel()`.

Returning a plain `std::uint8_t` vector (rather than a boolean `Series`) keeps
the predicate cheap and lets you feed it straight into indexing loops or into
`pluss::make_series` if you want Series-side masking.

### Edge cases

- An **empty `needle`** is contained in every cell, including empty cells, so
  every entry is `1`.
- A `needle` longer than a cell can never match that cell: its entry is `0`.
- The scan is linear in total input size and does no allocation beyond the
  result vector.

## Example 1: A Basic Mask

```cpp
const auto n = pluss::make_note({"midterm_1", "final_exam", "quiz"});
const auto mask = n.contains("exam");
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[0, 1, 0]
```

Only `"final_exam"` contains the substring `exam`.

## Example 2: The Test Is Case-Sensitive

```cpp
const auto n = pluss::make_note({"midterm_1", "final_exam", "quiz"});
const auto mask = n.contains("EXAM");
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[0, 0, 0]
```

`"EXAM"` is a different byte sequence from `"exam"`.

## Example 3: Fold Case First For A Case-Insensitive Check

```cpp
const auto mask =
    pluss::make_note({"Exam 1", "FINAL EXAM", "Quiz"}).lower().contains("exam");
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[1, 1, 0]
```

## Example 4: An Empty Needle Matches Everything

```cpp
const auto mask = pluss::make_note({"abc", "", "x"}).contains("");
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[1, 1, 1]
```

## Example 5: A Needle Longer Than The Cell Never Matches

```cpp
const auto mask = pluss::make_note({"ab"}).contains("abcd");
std::cout << "[" << static_cast<int>(mask[0]) << "]\n";
```

### Output

```text
[0]
```

## Example 6: Higher-Rank Input Is Flattened Row-Major

```cpp
const auto mask =
    pluss::make_note({{"apple", "grape"}, {"plum", "pear"}}).contains("p");
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[1, 1, 1, 1]
```

Four cells, flattened in row-major order; every one contains a `p`.

## See Also

- [`Note::regex_findall`](regex_findall.md): when you need the matching
  substrings, not just a yes/no.
- [`Note::replace`](replace.md) / [`Note::remove_all`](remove_all.md):
  rewrite or delete matches instead of testing for them.
- [`Note::lower`](lower.md): normalize case before a case-insensitive test.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="collapse_whitespace.html">
    <span class="label">Previous</span>
    <span class="title">collapse_whitespace</span>
  </a>
  <a class="next-link" href="contains_any_alpha.html">
    <span class="label">Next</span>
    <span class="title">contains_any_alpha</span>
  </a>
</div>

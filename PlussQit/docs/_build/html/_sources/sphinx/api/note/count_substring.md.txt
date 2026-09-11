<!-- pluss-hand-authored -->
# Note::count_substring

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<std::int64_t> Note::count_substring(std::string_view needle) const
```

## Count Occurrences Of A Literal Substring In Each Cell

`count_substring(needle)` returns, for each cell, how many times the literal
string `needle` occurs in it. It is the counting member of the substring
family — [`contains`](contains.md) answers "any?", this answers "how many?"
Use it to weight rows, spot the cell with the most separators, or turn a
`Note` into a numeric feature.

Occurrences are counted **left to right and non-overlapping**: after a match
the scan resumes at its end, so `"aaaa"` contains `"aa"` twice, not three
times. The match is literal — no regex, no case folding.

### Return value

Not a `Note`: a flat `std::vector<std::int64_t>` with one entry per cell, in
row-major order, length `numel()`.

### Edge cases

- An **empty `needle`** yields `0` for every cell (it is not counted between
  every character).
- A `needle` that never appears yields `0`.

## Example 1: A Basic Count

```cpp
const auto counts =
    pluss::make_note({"banana", "aaaa", "xyz", ""}).count_substring("a");
std::cout << "[";
for (std::size_t i = 0; i < counts.size(); ++i)
  std::cout << (i ? ", " : "") << counts[i];
std::cout << "]\n";
```

### Output

```text
[3, 4, 0, 0]
```

## Example 2: Matches Do Not Overlap

```cpp
const auto counts = pluss::make_note({"aaaa"}).count_substring("aa");
std::cout << "[" << counts[0] << "]\n";
```

### Output

```text
[2]
```

## Example 3: An Empty Needle Counts Zero

```cpp
const auto counts = pluss::make_note({"abc"}).count_substring("");
std::cout << "[" << counts[0] << "]\n";
```

### Output

```text
[0]
```

## Example 4: A Multi-Character Needle

```cpp
const auto counts =
    pluss::make_note({"a-b-c-d", "no dashes"}).count_substring("-");
std::cout << "[";
for (std::size_t i = 0; i < counts.size(); ++i)
  std::cout << (i ? ", " : "") << counts[i];
std::cout << "]\n";
```

### Output

```text
[3, 0]
```

## See Also

- [`Note::contains`](contains.md): the yes/no version.
- `pluss::coden::Note::count_regex`: count matches of a pattern rather than a
  literal.
- [`Note::regex_findall`](regex_findall.md): return the matches, not just
  their count.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="contiguous.html">
    <span class="label">Previous</span>
    <span class="title">contiguous</span>
  </a>
  <a class="next-link" href="dedent.html">
    <span class="label">Next</span>
    <span class="title">dedent</span>
  </a>
</div>

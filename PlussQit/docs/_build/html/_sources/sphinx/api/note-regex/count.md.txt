<!-- pluss-hand-authored -->
# NoteRegexAccessor::count

[Back to Note Regex Accessor API Reference](../../note_regex_accessor_api_reference)

## Signature

```cpp
Note NoteRegexAccessor::count(std::string_view pattern) const
```

## Measure Pattern Frequency Without Extracting Matches

`count(pattern)` compiles `pattern` as an ECMAScript regular expression and
counts its non-overlapping matches independently in every receiver cell. It is
the measurement operation in the regex family: use it when the number of
occurrences matters but the matching text does not.

The result is an Int64 `Note` with exactly the receiver's shape. A cell with no
match receives `0`; no cell is removed, flattened, or represented by an absent
value. This differs from [`findall`](findall.md), whose variable number of
matches requires a ragged `TokenBatch`, and from
[`Note::count_substring`](../note/count_substring.md), which treats its argument
as literal text rather than regex syntax.

### What Counts As A Match

Scanning proceeds from left to right. After a match, the next search begins at
the end of that match, so results never overlap. For example, `aa` occurs twice,
not three times, in `aaaa`. Parenthesized capture groups only select subexpressions
inside a match; they do not change the count and do not create additional
result lanes.

`count()` has no flags argument. Matching is case-sensitive and uses the default
ECMAScript grammar. Use a pattern that expresses the required case behavior, or
normalize text before counting. A malformed expression throws `pluss::Error`
before any result is returned.

### Type, Shape, And Cost

Text and Bytes Notes are accepted. Bool, Int64, Float64, and undefined Notes are
rejected because their cells are not character sequences. The regex is compiled
once per call and reused for every cell. The method allocates one contiguous
Int64 output lane per input cell; it does not allocate or retain the matching
substrings. That makes it the direct choice when extraction would only be used
to compute a length.

## Example 1: Count Every Digit Run Per Cell

```cpp
const auto text = pluss::make_note({"A1,b22,,C333", "none"});
std::cout << pluss::note::print(text.regex().count(R"(\d+)")) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, dtype=Int64)
[3, 0]
```

The first cell has three digit runs. Their widths do not matter: `1`, `22`, and
`333` each contribute one match.

## Example 2: Higher-Rank Shape Is Preserved

```cpp
const auto grid = pluss::make_note(
    {{"id=7 id=42", "none"}, {"x9", "100 and 200"}});
const auto counts = grid.regex().count(R"(\d+)");
std::cout << "shape=[" << counts.size(0) << ", " << counts.size(1) << "]\n";
std::cout << "values=[";
for (std::size_t i = 0; i < counts.integers().size(); ++i)
  std::cout << (i == 0 ? "" : ", ") << counts.integers()[i];
std::cout << "]\n";
```

### Output

```text
shape=[2, 2]
values=[2, 0, 1, 2]
```

Counting is cellwise, not a reduction over the complete Note. The two-dimensional
layout survives unchanged and only each cell's payload becomes an integer.

## Example 3: Matches Do Not Overlap

```cpp
const auto counts =
    pluss::make_note({"aaaa", "id=7 id=42"})
        .regex().count(R"(id=(\d+)|aa)");
std::cout << pluss::note::print(counts) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, dtype=Int64)
[2, 2]
```

In `aaaa`, the first `aa` consumes positions 0 and 1, and the second consumes
positions 2 and 3. In the second cell, the capture group describes the digits
inside each `id=...` match; it does not add separate counts.

## Example 4: Invalid Regex Syntax Fails Before Counting

```cpp
try {
  (void)pluss::make_note({"x"}).regex().count("(");
} catch (const pluss::Error& error) {
  const std::string message(error.what());
  std::cout << message.substr(0, message.find('\n')) << '\n';
}
```

### Output

```text
regex().count: The expression contained mismatched ( and ).
```

## Example 5: Non-Text Storage Is Rejected

```cpp
try {
  (void)pluss::note::make(std::vector<bool>{true, false})
      .regex().count("true");
} catch (const pluss::Error& error) {
  const std::string message(error.what());
  std::cout << message.substr(0, message.find('\n')) << '\n';
}
```

### Output

```text
regex().count: expected Text or Bytes storage
```

Printed boolean spellings are presentation, not stored text, so the regex
engine does not reinterpret them as character data.

## See Also

- [`NoteRegexAccessor::findall`](findall.md): retain each matching substring and
  its source-cell boundaries.
- [`Note::count_substring`](../note/count_substring.md): count one literal byte
  sequence without regex metacharacters or compilation.
- [`NoteRegexAccessor::search`](search.md): return one Bool per cell when only
  presence or absence matters.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../note_regex_accessor_api_reference.html">
    <span class="label">Previous</span>
    <span class="title">Regex accessor</span>
  </a>
  <a class="next-link" href="findall.html">
    <span class="label">Next</span>
    <span class="title">findall</span>
  </a>
</div>

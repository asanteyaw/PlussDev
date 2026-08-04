# Note::contains

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::contains(std::string_view needle) const -> std::vector<std::uint8_t>
```

## What It Does

Checks each text cell for the presence of a substring and returns one boolean
flag per cell.

This is the direct `Note`-level membership question:

- "Does this case mention `exam`?"
- "Does this row contain `http`?"
- "Does this cell still contain a marker I expected to remove?"

The output is returned as a dense vector of `0/1` flags rather than another
`Note`, because the method is answering a predicate question, not building new
text.

That makes `contains()` one of the easiest inspection surfaces to compose with
later transforms: you can check first, then decide whether a cleanup or
replacement pass is even needed.

## Parameters

**needle**

The substring to search for inside each cell.

The match is literal substring matching, not regex matching. If you need
pattern syntax, use [`regex_findall`](regex_findall.md) or one of the regex
predicate methods instead.

Matching is case-sensitive unless you normalize the text first, for example
with [`lower`](lower.md).

## Returns

`std::vector<std::uint8_t>`
    One flag per element in the `Note`.

- `1` means the substring was found.
- `0` means it was not found.

## Examples

Example 1. Check which feature names mention `exam`.

```cpp
auto note = pluss::make_note({"midterm_1", "final_exam", "quiz"});
auto mask = note.contains("exam");
std::cout << "contains_exam:";
for (std::uint8_t v : mask) {
  std::cout << " " << static_cast<int>(v);
}
std::cout << "\n";
```

### Output

```text
contains_exam: 0 1 0
```

Example 2. Lowercase first when you want a case-insensitive substring workflow.

```cpp
auto cleaned = pluss::make_note({"Exam 1", "final exam", "Quiz"}).lower();
auto mask = cleaned.contains("exam");
std::cout << "contains_exam_lower:";
for (std::uint8_t v : mask) {
  std::cout << " " << static_cast<int>(v);
}
std::cout << "\n";
```

### Output

```text
contains_exam_lower: 1 1 0
```

## Notes

- `contains()` is shape-preserving in the sense that it evaluates every text
  cell independently, but it returns a flag vector rather than a new `Note`.
- Matching is substring-based, so `"exam"` matches `"final_exam"` without
  needing an exact whole-cell match.
- This is often the cheapest way to build a quick text mask before handing the
  result to another layer such as `Thread`, `DataFrame`, or custom control
  logic.

## See Also

- [`Note::regex_findall`](regex_findall.md): extract regex matches rather than
  returning a yes/no mask.
- [`Note::replace`](replace.md): rewrite matching substrings instead of testing
  for them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../note_api_reference.html">
    <span class="label">Previous</span>
    <span class="title">Note API Reference</span>
  </a>
  <a class="next-link" href="lengths_bytes.html">
    <span class="label">Next</span>
    <span class="title">lengths_bytes</span>
  </a>
</div>

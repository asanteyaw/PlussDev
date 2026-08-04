# Note::lengths_bytes

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::lengths_bytes() const -> std::vector<std::int64_t>
```

## What It Does

Returns the byte length of each text cell.

This is the raw storage-oriented size question, not the user-visible character
count question. It is useful when you care about UTF-8 payload size, buffer
budgets, export diagnostics, or byte-level heuristics.

If you want character-oriented or grapheme-oriented size instead, this is not
the method to reach for.

So `lengths_bytes()` is best thought of as an encoding/storage diagnostic,
not a linguistic measurement.

## Parameters

This method takes no parameters.

## Returns

`std::vector<std::int64_t>`
    One byte length per element in the `Note`.

## Examples

Example 1. Measure the UTF-8 byte width of several labels.

```cpp
auto note = pluss::make_note({"midterm_1", "final_exam", "quiz"});
auto lengths = note.lengths_bytes();
std::cout << "lengths_bytes:";
for (std::int64_t v : lengths) {
  std::cout << " " << v;
}
std::cout << "\n";
```

### Output

```text
lengths_bytes: 9 10 4
```

Example 2. Trim boundary noise first when the size question is about cleaned text.

```cpp
auto cleaned = pluss::make_note({"  id-001  ", "\tAB\t"}).strip();
auto lengths = cleaned.lengths_bytes();
std::cout << "lengths_bytes_strip:";
for (std::int64_t v : lengths) {
  std::cout << " " << v;
}
std::cout << "\n";
```

### Output

```text
lengths_bytes_strip: 6 2
```

## Notes

- The result is a plain numeric vector because the answer is metadata about the
  text payload, not transformed text.
- For ASCII-only content, byte length and character count are the same.
- For non-ASCII UTF-8 text, byte length can be larger than the visible
  character count.

## See Also

- [`Note::contains`](contains.md): boolean inspection instead of size
  inspection.
- [`Note::strip`](strip.md): trim boundary characters before measuring if
  leading/trailing noise should not count.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="contains.html">
    <span class="label">Previous</span>
    <span class="title">contains</span>
  </a>
  <a class="next-link" href="lower.html">
    <span class="label">Next</span>
    <span class="title">lower</span>
  </a>
</div>

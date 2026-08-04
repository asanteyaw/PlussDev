# Note::regex_findall

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::regex_findall(std::string_view pattern) const -> TokenBatch
Note::regex_findall(std::string_view pattern, std::int64_t flags) const -> TokenBatch
```

## What It Does

Finds every regex match in each text cell and returns the matches as a
`TokenBatch`.

This is the extraction-oriented regex method. Use it when the goal is not just
"does this match?" but "pull out every token that matches this pattern."

That makes it useful for:

- numbers
- ticket IDs
- dates
- hashtags
- symbol-like fragments

It is a token-extraction surface, not a yes/no predicate surface. If your next
step depends on the matched substrings themselves, `regex_findall()` is the
right shape.

## Parameters

**pattern**

The regex pattern to apply to each text cell.

The returned tokens are the matched substrings themselves, flattened across the
whole `Note`.

**flags**

Optional regex-engine flags encoded as an integer bitmask.

Use this overload only when you already know the regex-engine flag contract you
need from the implementation. This page does not assign guessed semantics to
unnamed flag bits.

## Returns

`TokenBatch`
    A flattened token stream plus `offsets()` that mark where each original
    `Note` element begins and ends inside that stream.

## Examples

Example 1. Extract every digit run from two text cells.

```cpp
auto note = pluss::make_note({
    "Order 17 ships on 2026-08-02",
    "Ticket 204 closes at 18:30",
});

auto matches = note.regex_findall(R"(\d+)");
std::cout << "regex_findall_digits tokens:";
for (std::int64_t i = 0; i < matches.tokens().numel(); ++i) {
  std::cout << " [" << matches.tokens().at(i) << "]";
}
std::cout << "\n";
std::cout << "regex_findall_digits offsets:";
for (std::int64_t off : matches.offsets()) {
  std::cout << " " << off;
}
std::cout << "\n";
```

### Output

```text
regex_findall_digits tokens: [17] [2026] [08] [02] [204] [18] [30]
regex_findall_digits offsets: 0 4 7
```

Example 2. Extract only uppercase letter runs when the pattern is more
structural than numeric.

```cpp
auto note = pluss::make_note({"AX-17", "BY_204"});
auto matches = note.regex_findall(R"([A-Z]+)");
std::cout << "regex_findall_caps tokens:";
for (std::int64_t i = 0; i < matches.tokens().numel(); ++i) {
  std::cout << " [" << matches.tokens().at(i) << "]";
}
std::cout << "\n";
std::cout << "regex_findall_caps offsets:";
for (std::int64_t off : matches.offsets()) {
  std::cout << " " << off;
}
std::cout << "\n";
```

### Output

```text
regex_findall_caps tokens: [AX] [BY]
regex_findall_caps offsets: 0 1 2
```

Example 3. Extract hashtag-like fragments from short social-style text.

```cpp
auto note = pluss::make_note({"#alpha update", "ship #beta now"});
auto matches = note.regex_findall(R"(#\w+)");
std::cout << "regex_findall_tags tokens:";
for (std::int64_t i = 0; i < matches.tokens().numel(); ++i) {
  std::cout << " [" << matches.tokens().at(i) << "]";
}
std::cout << "\n";
std::cout << "regex_findall_tags offsets:";
for (std::int64_t off : matches.offsets()) {
  std::cout << " " << off;
}
std::cout << "\n";
```

### Output

```text
regex_findall_tags tokens: [#alpha] [#beta]
regex_findall_tags offsets: 0 1 2
```

## Notes

- `offsets()` is the boundary map back to the original cells.
- In the example above, the first input cell contributes tokens in the flat
  range `[0, 4)`, and the second contributes tokens in `[4, 7)`.
- If you only want a boolean answer, `regex_findall()` is heavier than a regex
  predicate method because it materializes every match.

## See Also

- [`Note::contains`](contains.md): literal substring inspection without regex.
- [`Note::split_whitespace`](split_whitespace.md): tokenize by whitespace rather
  than regex matches.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nlp_remove_urls.html">
    <span class="label">Previous</span>
    <span class="title">nlp_remove_urls</span>
  </a>
  <a class="next-link" href="replace.html">
    <span class="label">Next</span>
    <span class="title">replace</span>
  </a>
</div>

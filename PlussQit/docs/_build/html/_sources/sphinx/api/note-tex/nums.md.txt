<!-- pluss-hand-authored -->
# NoteTexAccessor::nums

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteTexAccessor::nums() const
```

## Extract Unsigned Decimal Lexemes

`nums()` finds unsigned integer and decimal lexemes inside complete math
fragments. `10` and `2.5` are tokens; a leading sign is a separate operator,
so `-3` contributes `"-"` through `ops()` and `"3"` through `nums()`. A
scientific literal is not combined: `1e3` is read according to the lexical
grammar rather than converted to a numeric value.

The method is an extractor, not a parser or cast. Returned values remain Text,
preserve their authored spelling, and are never range-checked or rounded.
Prose numbers, escaped dollar amounts, and unmatched math delimiters are
ignored. Every complete `$`, `$$`, `\(`, and `\[` region is scanned in source
order.

Variable counts are represented by flat tokens plus one cumulative offset per
receiver cell. Empty lanes therefore remain visible without fabricated values.
Undefined input propagates and non-text input throws. The scan is linear.

## Example 1: Exclude Prose And Preserve Spelling

```cpp
const auto source = pluss::make_note(
    std::vector<std::string>{
        R"(prose 99; $x_1 + \sin(\alpha) = 2.5$)",
        R"(\[\beta \leq y^2\] and \(\sqrt{z}\))",
        R"(escaped \$7 and plain theta)",
        R"($$\argmax_x f(x) \approx 10$$)"},
    std::array<std::int64_t, 2>{2, 2});
const auto result = source.tex().nums();
std::cout << result.tokens() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["2.5", "2", "10"]
```

`99` and `7` are outside valid math regions.

## Example 2: Offsets For A Rank-Two Receiver

```cpp
const auto offsets = result.offsets().integers();
for (auto value : offsets) std::cout << value << ' ';
std::cout << '\n';
```

### Output

```text
0 1 2 2 3 
```

## Example 3: Undefined And Invalid Dtypes

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().nums().tokens().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().nums();
} catch (const pluss::Error&) { std::cout << "numbers Int64: rejected\n"; }
```

### Output

```text
false
numbers Int64: rejected
```

## See Also

- [`ops`](ops.md): extract the sign and other operator lexemes separately.
- [`ids`](ids.md): extract variable-shaped lexemes.
- [`symbols`](symbols.md): combine control words and operators in source order.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="matrix_norm.html"><span class="label">Previous</span><span class="title">matrix_norm</span></a>
  <a class="next-link" href="ops.html"><span class="label">Next</span><span class="title">ops</span></a>
</div>

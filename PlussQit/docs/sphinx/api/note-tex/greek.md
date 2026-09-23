<!-- pluss-hand-authored -->
# NoteTexAccessor::greek

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteTexAccessor::greek() const
```

## Classify Lowercase Greek Control Words

`greek()` finds recognized lowercase Greek-letter commands inside complete
math fragments and returns their names without the leading backslash. It
recognizes the standard lowercase sequence from `alpha` through `omega`, plus
the variant forms `varepsilon`, `vartheta`, and `varphi`.

The operation deliberately distinguishes TeX commands from prose and from
literal Unicode letters. Plain `alpha`, the character `α`, and `\alpha`
outside math are not tokens. Uppercase commands are not silently folded to
lowercase. Use `symbols()` when all control words are wanted, not only this
semantic vocabulary.

All four anonymous math delimiter forms are scanned. Escaped or unmatched
delimiters do not open math. Tokens keep document order and duplicates; the
ragged `TokenBatch` offsets retain each cell's range. Undefined input
propagates, and non-text input throws.

## Example 1: Commands, Prose, And Empty Lanes

```cpp
const auto source = pluss::make_note(
    std::vector<std::string>{
        R"(prose 99; $x_1 + \sin(\alpha) = 2.5$)",
        R"(\[\beta \leq y^2\] and \(\sqrt{z}\))",
        R"(escaped \$7 and plain theta)",
        R"($$\argmax_x f(x) \approx 10$$)"},
    std::array<std::int64_t, 2>{2, 2});
const auto result = source.tex().greek();
std::cout << result.tokens() << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["alpha", "beta"]
```

## Example 2: Offsets Preserve The Empty Third Cell

```cpp
const auto offsets = result.offsets().integers();
for (auto value : offsets) std::cout << value << ' ';
std::cout << '\n';
```

### Output

```text
0 1 2 2 2 
```

## Example 3: Dtype Boundary

```cpp
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().greek();
} catch (const pluss::Error&) { std::cout << "Greek commands Int64: rejected\n"; }
```

### Output

```text
Greek commands Int64: rejected
```

## See Also

- [`symbols`](symbols.md): collect every TeX control word and operator symbol.
- [`ids`](ids.md): collect broad identifier lexemes without semantic classification.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="graphics_tags.html"><span class="label">Previous</span><span class="title">graphics_tags</span></a>
  <a class="next-link" href="headings.html"><span class="label">Next</span><span class="title">headings</span></a>
</div>

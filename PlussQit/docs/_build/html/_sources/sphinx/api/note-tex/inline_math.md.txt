<!-- pluss-hand-authored -->
# NoteTexAccessor::inline_math

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteTexAccessor::inline_math() const
```

## Extract Inline Formulae Without Mixing In Displays

`inline_math()` extracts complete inline-math regions from every Text cell. It
recognizes both authored forms, `$...$` and `\(...\)`, and retains the opening
and closing delimiters in each token. Keeping the delimiters makes the result
suitable for source inspection or relocation without guessing which spelling
the author chose.

The method deliberately excludes `$$...$$` and `\[...\]` display regions. Use
[`display_math`](display_math.md) for only those regions, or
[`math_blocks`](math_blocks.md) when inline and display forms belong in one
source-ordered stream. A single-dollar region may not cross a physical
newline; an escaped dollar does not close it. Parenthesized inline math may
span lines and is closed by the next unescaped `\)`.

Scanning is lexical rather than a TeX execution step. Delimiters inside a
comment or macro argument still have their ordinary byte-level meaning, and
the method neither expands commands nor evaluates formulae. An escaped opener,
an unmatched opener, and plain text contribute no token. Empty but balanced
regions are valid and are returned with their delimiters.

The result is a `TokenBatch`: `tokens()` is a flat Text Note in cell and source
order, while `offsets()` is an Int64 Note with one boundary per receiver cell
plus the final boundary. Cell `i` owns
`[offsets[i], offsets[i + 1])`. This representation preserves ownership for a
matrix or higher-rank receiver without padding cells to a common token count.
Undefined input produces undefined tokens and offsets `[0]`; every defined
non-Text dtype is rejected.

The scanner visits each source byte once and copies only matched regions. It
does not build a syntax tree. The token Note owns its strings, so it remains
valid after the receiver is destroyed.

## Example 1: Separate Inline And Display Forms

```cpp
const auto source = pluss::make_note(
    std::vector<std::string>{
        R"(price \$5; $x+\$y$ then $$display$$ and \(z\))",
        "\\[block\\] and $line\nbreak$", R"(\(unfinished)", "plain"},
    std::array<std::int64_t, 2>{2, 2});
const auto result = source.tex().inline_math();

std::cout << result.tokens() << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["$x+\\$y$", "\\(z\\)"]
```

The escaped dollar inside the first formula is content, not its closer. Both
display regions are excluded, the newline prevents `$line...break$` from
forming one inline region, and the unmatched `\(` is ignored.

## Example 2: Recover Matrix-Cell Ownership

```cpp
std::cout << "offsets=[";
const auto offsets = result.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 2, 2, 2, 2]
```

Both tokens came from the first logical cell. The remaining three matrix
lanes own empty half-open ranges.

## Example 3: Undefined And Invalid Input

```cpp
const auto absent = pluss::Note::undefined().tex().inline_math();
std::cout << std::boolalpha << absent.tokens().defined() << " offsets=["
          << absent.offsets().integers()[0] << "]\n";

try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().inline_math();
} catch (const pluss::Error&) {
  std::cout << "inline math Int64: rejected\n";
}
```

### Output

```text
false offsets=[0]
inline math Int64: rejected
```

## See Also

- [`display_math`](display_math.md): extract only display-math regions.
- [`math_blocks`](math_blocks.md): merge all four math-region forms in source order.
- [`add_math_delimiters`](add_math_delimiters.md): make each complete cell one math region.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="input_tags.html"><span class="label">Previous</span><span class="title">input_tags</span></a>
  <a class="next-link" href="int_norm.html"><span class="label">Next</span><span class="title">int_norm</span></a>
</div>

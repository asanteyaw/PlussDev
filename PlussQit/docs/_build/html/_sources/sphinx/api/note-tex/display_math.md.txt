<!-- pluss-hand-authored -->
# Note::TexAccessor::display_math

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch Note::TexAccessor::display_math() const
```

## Extract Block-Math Fragments

`display_math()` finds display-style math fragments in every Text cell. It
recognizes the two source forms that delimit an anonymous display directly:
`$$...$$` and `\[...\]`. The returned token includes both delimiters, so it can
be inspected, transformed, or emitted again without guessing which spelling
the source used.

This is extraction, not conversion. Inline `$...$` and `\(...\)` fragments are
ignored, prose remains outside the result, and the receiver is unchanged. Use
`math_blocks()` when inline and display math belong in the same result, or
`inline_math()` when only inline fragments are wanted.

Scanning is lane-local and source ordered. Each input cell contributes zero or
more tokens to a flat Text `Note`; the Int64 `offsets()` Note records where each
cell's range begins and ends. Consequently, a rank-two receiver still produces
a flat token Note, but its four logical cells produce five offsets. Empty cells
and cells without display math contribute repeated boundaries rather than
placeholder strings.

Only complete delimiter pairs are returned. An unmatched opener or closer does
not create a partial token. The operation does not parse commands inside a
fragment or validate mathematical syntax. Undefined input produces an
undefined token Note with offsets `[0]`; non-text input is rejected. Work is
linear in the bytes scanned plus the bytes copied into matching tokens.

## Example 1: Distinguish Display And Inline Math

```cpp
const auto source = pluss::make_note({
    R"(inline $a$ then $$b+c$$ and \[d=e\])",
    R"($$first$$ between $$second$$)",
    R"(plain text and \(inline\))"});
const auto blocks = source.tex().display_math();

std::cout << blocks.tokens() << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["$$b+c$$", "\\[d=e\\]", "$$first$$", "$$second$$"]
```

The inline fragments do not appear. Both display spellings retain their
original delimiters.

## Example 2: Recover Cell Ownership

```cpp
std::cout << "offsets=[";
const auto offsets = blocks.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 2, 4, 4]
```

Cell zero owns tokens `[0, 2)`, cell one owns `[2, 4)`, and cell two owns the
empty range `[4, 4)`.

## Example 3: Undefined And Invalid Dtypes

```cpp
const auto absent = pluss::Note::undefined().tex().display_math();
std::cout << std::boolalpha << absent.tokens().defined() << " offsets=["
          << absent.offsets().integers()[0] << "]\n";

try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().display_math();
} catch (const pluss::Error&) {
  std::cout << "display math Int64: rejected\n";
}
```

### Output

```text
false offsets=[0]
display math Int64: rejected
```

## See Also

- `inline_math`: extract only inline math fragments.
- `math_blocks`: extract both inline and display math.
- `add_math_delimiters`: wrap whole cells instead of extracting fragments.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dedup_blank_lines.html"><span class="label">Previous</span><span class="title">dedup_blank_lines</span></a>
  <a class="next-link" href="dots_norm.html"><span class="label">Next</span><span class="title">dots_norm</span></a>
</div>

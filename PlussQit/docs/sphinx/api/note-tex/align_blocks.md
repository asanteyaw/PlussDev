<!-- pluss-hand-authored -->
# Note::TexAccessor::align_blocks

[Back to Note Tex Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch Note::TexAccessor::align_blocks() const
```

## Extract Complete Alignment Environments

`align_blocks()` finds complete `align` and `align*` environments in every
Text cell. Each token includes its `\begin{...}` and matching `\end{...}`
markers, so it can be inspected, stored, or passed to another TeX operation
without reconstructing the environment name.

Matches retain source order, including when starred and unstarred forms are
interleaved. The extractor spans newlines, accepts any body content up to the
matching closing marker, and ignores unrelated environments. An incomplete
environment is not emitted; this method extracts valid lexical blocks rather
than repairing them.

The ragged result follows the standard `TokenBatch` contract. `tokens()` is a
rank-1 Text Note of all blocks. `offsets()` is a rank-1 Int64 Note of length
`source.numel() + 1`; flattened source cell `i` owns tokens in
`[offsets[i], offsets[i + 1])`. Cells without a block therefore produce equal
adjacent offsets. Source rank is represented by those lane boundaries rather
than copied onto the variable-length token result.

The receiver is not mutated. Extraction is linear in source bytes plus regex
matching cost and allocates the matched blocks and offsets. Undefined produces
an undefined token Note with offsets `[0]`. Only Text is accepted; Bytes and
native dtypes reject.

## Example 1: Starred And Unstarred Blocks Keep Document Order

```cpp
const auto documents = pluss::make_note({
    R"(before \begin{align*}a&=1\end{align*} middle \begin{align}b&=2\end{align})",
    "no alignment here",
    R"(\begin{align}c&=3\end{align})"});
const auto blocks = documents.tex().align_blocks();

std::cout << blocks.tokens() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["\\begin{align*}a&=1\\end{align*}", "\\begin{align}b&=2\\end{align}", "\\begin{align}c&=3\\end{align}"]
```

## Example 2: Offsets Recover Per-Cell Ownership

```cpp
std::cout << "offsets=[";
for (std::size_t i = 0; i < blocks.offsets().integers().size(); ++i)
  std::cout << (i ? ", " : "") << blocks.offsets().integers()[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 2, 2, 3]
```

Cell 0 owns two blocks, cell 1 owns the empty interval `[2, 2)`, and cell 2
owns the final block.

## Example 3: Native Dtypes Reject

```cpp
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().align_blocks();
} catch (const pluss::Error&) {
  std::cout << "align Int64: rejected\n";
}
```

### Output

```text
align Int64: rejected
```

## See Also

- [`env("align")`](env.md): extract one explicitly named environment.
- `display_math`: extract display-math delimiters rather than environments.
- [`TokenBatch`](../../tokenbatch_api_reference.md): offsets, ownership, and empty rows.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add_math_delimiters.html"><span class="label">Previous</span><span class="title">add_math_delimiters</span></a>
  <a class="next-link" href="args.html"><span class="label">Next</span><span class="title">args</span></a>
</div>

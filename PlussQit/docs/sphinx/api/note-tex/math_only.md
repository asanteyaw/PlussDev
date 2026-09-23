<!-- pluss-hand-authored -->
# NoteTexAccessor::math_only

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::math_only() const
```

## Project Each Cell Onto Its Authored Formulae

`math_only()` removes prose and joins the contents of every complete math
region in a Text cell. It recognizes `$...$`, `$$...$$`, `\(...\)`, and
`\[...\]`, strips those outer delimiters, trims each recovered body, and joins
non-empty bodies with one ASCII space. The result is useful when formula text,
rather than its inline/display layout, is the material to index or transform.

This method is a shape-preserving projection, not a ragged extractor. One input
cell always produces one output cell. A cell with no complete non-empty math
region becomes the empty string, which preserves its location in a matrix or
higher-rank Note. Use [`math_blocks`](math_blocks.md) when every region must
remain a separate token with its original delimiters and per-cell offsets.

Delimiter recognition is shared with the math extractors. An escaped dollar is
literal content, `$$` is recognized before `$`, a single-dollar region cannot
cross a physical newline, and unmatched openers are ignored. Commands and
spacing inside a recovered body are otherwise retained; `math_only()` does not
evaluate TeX, expand macros, or validate a formula. Joining normalizes
whitespace between regions and trims the final cell, but does not remove
commands from inside a region.

The receiver is not mutated. A new Text Note with the same rank and shape is
allocated, including for scalar and empty-shaped inputs. Undefined input stays
undefined, and every defined non-Text dtype is rejected before transformation.
The kernel scans each source cell linearly and copies only retained formula
bodies into the result arena.

## Example 1: Project All Four Delimiter Forms

```cpp
const auto source = pluss::make_note(
    std::vector<std::string>{
        R"(price \$5, $x+\$y$, $$z$$, and \(q\))",
        "\\[a+b\\] then $line\nbreak$", R"(\(unfinished)", "plain"},
    std::array<std::int64_t, 2>{2, 2});

std::cout << source.tex().math_only() << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["x+\\$y z q", "a+b"],
  ["", ""]
]
```

The escaped currency dollar is not an opener, while the escaped dollar inside
the first formula remains content. The newline prevents the second cell's
single-dollar candidate from becoming a region. Unmatched and math-free cells
remain present as empty strings.

## Example 2: Contrast Projection With Extraction

```cpp
const auto projected = source.tex().math_only();
const auto extracted = source.tex().math_blocks(-1);

std::cout << "projected shape=[" << projected.sizes()[0] << ", "
          << projected.sizes()[1] << "]\n";
std::cout << extracted.tokens() << '\n';
```

### Output

```text
projected shape=[2, 2]
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["$x+\\$y$", "$$z$$", "\\(q\\)", "\\[a+b\\]"]
```

Projection keeps one lane per source cell; extraction keeps one lane per match.

## Example 3: Undefined And Invalid Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().math_only().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().math_only();
} catch (const pluss::Error&) {
  std::cout << "math only Int64: rejected\n";
}
```

### Output

```text
false
math only Int64: rejected
```

## See Also

- [`math_blocks`](math_blocks.md): retain separate regions and original delimiters.
- [`inline_math`](inline_math.md): extract only inline regions.
- [`display_math`](display_math.md): extract only display regions.
- `text_only`: keep prose while removing math regions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="math_funcs.html"><span class="label">Previous</span><span class="title">math_funcs</span></a>
  <a class="next-link" href="matrix_norm.html"><span class="label">Next</span><span class="title">matrix_norm</span></a>
</div>

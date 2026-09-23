<!-- pluss-hand-authored -->
# NoteTexAccessor::text_only

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::text_only() const
```

## Project Each Cell Onto Its Prose, Dropping Math

`text_only()` removes every complete math region from a Text cell —
recognizing `$...$`, `$$...$$`, `\(...\)`, and `\[...\]` the same way
[`math_only`](math_only.md) does — and keeps everything else, collapsing
the resulting whitespace gaps to one ASCII space and trimming the cell. It
is the exact prose-preserving mirror of `math_only`, which keeps the
formulae and drops the prose; the two together partition a cell's visible
content into its two halves.

This method is a shape-preserving projection, not a ragged extractor. One
input cell always produces one output cell. A cell that is entirely math
becomes the empty string, which preserves its location in a matrix or
higher-rank Note. An unmatched opener — a `$` with no closing `$` before
the cell ends — is not treated as a region boundary, so the unmatched
dollar and the text after it are kept as ordinary prose.

Delimiter recognition is shared with the math extractors: an escaped dollar
(`\$`) is literal prose, not an opener; `$$` is recognized before a plain
`$`; and a single-dollar region cannot cross a physical newline. Content
outside any math region — including any TeX commands there — is retained
exactly, byte for byte aside from whitespace collapsing; `text_only()` does
not itself strip commands or comments.

The receiver is not mutated. A new Text Note with the same rank and shape is
allocated, including for scalar and empty-shaped inputs. Undefined input
stays undefined, and every defined non-Text dtype is rejected before
transformation.

## Example 1: All Four Delimiter Forms And An Unmatched Opener

```cpp
const auto source = pluss::make_note({
    R"(Before $x^2$ after)", R"(A \(y+1\) B \[z\] C)",
    R"(price \$5 and $unfinished)", ""});
std::cout << source.tex().text_only() << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["Before after", "A B C", "price \\ unfinished", ""]
```

The first two cells lose their math and keep the surrounding prose, with
the gap it left collapsed to one space. In the third cell, `\$5` is literal
prose (`\` survives — see the note below), and the unmatched `$unfinished`
is never closed, so it is never treated as math and its text is kept as-is.
The empty cell stays empty.

`\$` unescapes to a bare `\` here because `text_only()` copies retained
prose byte for byte without re-escaping; only the `$` that made the pair a
literal dollar sign is consumed by the delimiter scanner, leaving the
backslash behind exactly as read.

## Example 2: Idempotent On Already-Prose-Only Input

```cpp
const auto prose_only = source.tex().text_only();
std::cout << std::boolalpha
          << (prose_only.tex().text_only() == prose_only).all().bool_item()
          << '\n';
```

### Output

```text
true
```

Once every math region is gone, a second call has nothing left to remove.

## Example 3: A 2-D Grid, Shape Preserved

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{"left $x$", "$y$ right", "plain", "$$z$$"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << grid.tex().text_only() << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["left", "right"], 
  ["plain", ""]
]
```

The fully-math last cell reduces to the empty string in place, keeping the
grid's shape intact.

## Example 4: Undefined And Non-Text Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().text_only().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().text_only();
} catch (const pluss::Error&) {
  std::cout << "text only Int64: rejected\n";
}
```

### Output

```text
false
text only Int64: rejected
```

## See Also

- [`math_only`](math_only.md): the exact prose-dropping mirror of this
  method.
- [`math_blocks`](math_blocks.md): keep every math region as a separate
  ragged token instead of discarding it.
- [`to_text`](to_text.md): a broader prose extraction that also strips
  commands and comments, not just math regions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tabular_blocks.html"><span class="label">Previous</span><span class="title">tabular_blocks</span></a>
  <a class="next-link" href="to_html.html"><span class="label">Next</span><span class="title">to_html</span></a>
</div>

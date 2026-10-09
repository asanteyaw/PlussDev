<!-- pluss-hand-authored -->
# Note::NlpAccessor::remove_headers

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note Note::NlpAccessor::remove_headers() const
```

## Remove Hash-Marked Heading Lines

`remove_headers()` removes complete hash-marked heading lines from each
text cell. It is intended for extracted or authored text in which headings are
structural labels and the body is the material to retain. The complete line is
deleted; this is not a marker-stripping operation that turns `## Results` into
`Results`.

After optional leading spaces or tabs, a heading must satisfy both conditions:

| Component | Rule |
|---|---|
| Marker count | Between one and six consecutive `#` bytes |
| Marker boundary | End of line, a space, or a tab immediately after the final `#` |

Consequently, `# Title`, `   ### Results`, and a line containing only `##`
are headings. `####### Seven` is retained because seven markers are outside
the supported level range. `#include <vector>` and `#topic` are retained
because the marker is immediately followed by a non-whitespace byte. Those
distinctions let source fragments and hashtag-like prose coexist with heading
cleanup without being silently discarded.

When at least one heading is removed, surviving lines are joined through
collapsed ASCII whitespace and stripped. This produces compact prose from a
multi-section cell. When no heading exists, the original bytes are returned
exactly, including line breaks and indentation. If every line is a heading,
the corresponding result cell is empty rather than absent.

The method allocates a new shape-identical Text Note and does not mutate the
receiver. Every cell is classified independently. Undefined and empty Notes
propagate according to the common Note transform contract; Bytes, Bool, Int64,
and Float64 Notes reject because line classification is defined only for Text.
Cells without `#` bypass line parsing entirely.

## Example 1: Heading Levels And Indentation

```cpp
std::cout << pluss::note::print(
    pluss::note::make({
        "# Heading\nBody\n   ### Nested\nText\n"
        "####### Seven\n#include <x>\n#topic"})
        .nlp().remove_headers()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["Body Text ####### Seven #include <x> #topic"]
```

Only the one- and three-marker heading lines are removed. The seven-marker,
preprocessor-like, and tag-like lines become part of the compacted body.

## Example 2: Hash-Prefixed Non-Headings Are Exact No-Ops

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"  #topic\n#include <vector>  "})
        .nlp().remove_headers()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["  #topic\n#include <vector>  "]
```

Because neither line has whitespace after a valid marker run, no transformation
occurs and the cell's formatting remains untouched.

## Example 3: Classification Is Per Cell

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{"a@b.io", "Page 1", "#tag", "<b>bold</b>"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << pluss::note::print(matrix.nlp().remove_headers()) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["a@b.io", "Page 1"], 
  ["#tag", "<b>bold</b>"]
]
```

`#tag` is not a heading, and the matrix's two-dimensional shape is retained.

## See Also

- [`remove_footers`](remove_footers.md): remove recognized page and rights lines.
- [`remove_bullets`](remove_bullets.md): retain list-item text while deleting its marker.
- `Note::NlpAccessor::remove_markdown`: remove several Markdown constructs in one broader pass.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_hashtags.html"><span class="label">Previous</span><span class="title">remove_hashtags</span></a>
  <a class="next-link" href="remove_html.html"><span class="label">Next</span><span class="title">remove_html</span></a>
</div>

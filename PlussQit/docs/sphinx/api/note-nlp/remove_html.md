<!-- pluss-hand-authored -->
# NoteNlpAccessor::remove_html

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::remove_html() const
```

## Strip Markup While Retaining Visible Text

`remove_html()` removes element-like markup and comments while preserving
text between tags. It is useful when markup supplies presentation or document
structure but the next operation needs plain textual content. It does not
decode entities, execute a document model, or validate nesting; use
`html_unescape()` separately when encoded entities such as `&amp;` should become
their represented characters.

The lexical recognizer handles these forms:

| Form | Example | Result |
|---|---|---|
| Opening and closing elements | `<strong>text</strong>` | Tags removed; `text` retained |
| Elements with attributes | `<a href="/x">link</a>` | Tags removed; `link` retained |
| Self-closing elements | `<br/>`, `<item key="x" />` | Element removed |
| Closed comments | `<!-- internal -->` | Comment and its contents removed |
| Declarations | `<!DOCTYPE html>` | Declaration removed |
| Processing instructions | `<?xml version="1.0"?>` | Instruction removed |
| Named or namespaced elements | `<x-card>`, `<svg:path>` | Tag removed |

An ordinary element name must begin immediately after `<` or `</` with an
ASCII letter. This protects comparison-like prose such as `3 < 5` and
`x < y > z`; `< b >` is likewise not treated as an element. Because this is a
bounded lexical cleanup rather than a full parser, malformed or unclosed
constructs are not repaired, script/style semantics are not interpreted, and
matching tags are not required to balance before recognized tag spans are
removed.

After an actual removal, ASCII whitespace is collapsed and stripped. A space
introduced immediately before `.`, `,`, `;`, `:`, `!`, `?`, `)`, or `]` is
removed, so closing markup does not turn `world.` into `world .`. Text from
adjacent block elements remains separated rather than being concatenated into
one word. If nothing matches, the input cell is returned byte-for-byte.

The result is a fresh Text Note with unchanged rank and shape. Empty visible
content becomes an empty cell, not absence. Applying the operation twice is
idempotent for the recognized grammar. Undefined Notes remain undefined,
empty Notes remain empty, and Bytes, Bool, Int64, and Float64 Notes reject.
Cells missing either `<` or `>` take a fast path that avoids regex processing.

## Example 1: Elements, Comments, Punctuation, And Entities

```cpp
std::cout << pluss::note::print(
    pluss::note::make({
        "<p>Hello <strong>world</strong>.</p>"
        "<!-- hidden --><div>Tail &amp; more</div>"})
        .nlp().remove_html()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["Hello world. Tail &amp; more"]
```

The comment disappears with its contents, visible element text survives, the
period remains attached to `world`, and `&amp;` remains encoded by design.

## Example 2: Declarations And Custom Elements

```cpp
std::cout << pluss::note::print(
    pluss::note::make({
        "<!DOCTYPE html><?xml version=\"1.0\"?>"
        "<x-card data-id=\"7\">Value</x-card>"})
        .nlp().remove_html()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["Value"]
```

This demonstrates lexical support beyond the small set of built-in element
names; the operation recognizes element syntax rather than maintaining a tag
allow-list.

## Example 3: Adjacent Blocks Keep A Word Boundary

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"<div>One</div><div>Two</div>"})
        .nlp().remove_html()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["One Two"]
```

## Example 4: Comparison Text Is An Exact No-Op

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"  3 < 5 and x < y > z\nstill text  "})
        .nlp().remove_html()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["  3 < 5 and x < y > z\nstill text  "]
```

No tag was recognized, so whitespace normalization is not applied as an
unrelated side effect.

## Example 5: Higher-Rank Notes Retain Their Structure

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{"a@b.io", "Page 1", "#tag", "<b>bold</b>"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << pluss::note::print(matrix.nlp().remove_html()) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["a@b.io", "Page 1"], 
  ["#tag", "bold"]
]
```

Only the fourth cell contains recognized markup; every coordinate and extent
remain unchanged.

## Example 6: Empty, Undefined, And Non-Text Contracts

```cpp
const auto empty = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 1>{0});
std::cout << pluss::note::print(empty.nlp().remove_html()) << '\n';

const pluss::Note undefined;
std::cout << "undefined defined=" << std::boolalpha
          << undefined.nlp().remove_html().defined() << '\n';

try {
  (void)pluss::note::bytes({std::byte{0x3c}}).nlp().remove_html();
} catch (const std::exception&) {
  std::cout << "Bytes: rejected\n";
}
```

### Output

```text
Note(shape=[0], dim=1, numel=0, encoding=utf8)
[]
undefined defined=false
Bytes: rejected
```

Bool, Int64, and Float64 Notes reject under the same Text-only rule.

## See Also

- [`html_unescape`](../note/html_unescape.md): decode entities after markup removal when desired.
- [`remove_code`](remove_code.md): delete inline, fenced, and indented code forms.
- `NoteNlpAccessor::remove_markdown`: remove a broader set of Markdown constructs.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_headers.html"><span class="label">Previous</span><span class="title">remove_headers</span></a>
  <a class="next-link" href="remove_latex.html"><span class="label">Next</span><span class="title">remove_latex</span></a>
</div>

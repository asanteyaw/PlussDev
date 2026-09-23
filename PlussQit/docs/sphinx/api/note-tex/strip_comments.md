<!-- pluss-hand-authored -->
# NoteTexAccessor::strip_comments

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::strip_comments() const
```

## Remove TeX Comments Without Flattening Lines

`strip_comments()` removes an unescaped `%` and every following byte up to the
next physical line ending. The line ending itself is retained. This makes it a
source-cleaning step: line boundaries remain available to later diagnostics,
paragraph handling, or [`fix_linebreaks()`](fix_linebreaks.md), while commented
commands no longer participate in extraction and conversion.

A percent sign is escaped when immediately preceded by an odd run of
backslashes; an even run leaves it active as a comment marker. LF, CRLF, and CR
line endings are recognized independently and preserved byte-for-byte. A final
comment without a line ending removes the remainder of that cell. The method is
lexical: it does not attempt macro expansion or verbatim-environment parsing.

The transform is out-of-place, idempotent, shape preserving, and Text-only.
Empty results remain empty strings in their original positions. Undefined state
propagates, while defined non-Text Notes reject. Processing is one linear pass
over each cell with one output allocation.

## Example 1: Escapes And All Physical Line Endings

```cpp
const auto source = pluss::make_note({
    "keep \\% percent % LF\nnext",
    "alpha % CRLF\r\nbeta",
    "one % CR\rtwo",
    "100\\% visible"});
std::cout << source.tex().strip_comments() << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["keep \\% percent \nnext", "alpha \r\nbeta", "one \rtwo", "100\\% visible"]
```

## Example 2: Comments Do Not Remove Matrix Lanes

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{"a % x\nb", "plain", "% all\nnext", ""},
    std::array<std::int64_t, 2>{2, 2});
std::cout << grid.tex().strip_comments() << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["a \nb", "plain"],
  ["\nnext", ""]
]
```

## Example 3: Idempotence, Undefined State, And Dtype

```cpp
const auto once = pluss::make_note("a % hidden\nb").tex().strip_comments();
std::cout << std::boolalpha
          << "idempotent="
          << ((once == once.tex().strip_comments()).all().bool_item())
          << " undefined="
          << pluss::Note::undefined().tex().strip_comments().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().strip_comments();
} catch (const pluss::Error&) {
  std::cout << "Int64: rejected\n";
}
```

### Output

```text
idempotent=true undefined=false
Int64: rejected
```

## See Also

- [`fix_linebreaks`](fix_linebreaks.md): normalize and compact physical lines.
- [`line_norm`](line_norm.md): convert CRLF and CR endings to LF.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_color.html"><span class="label">Previous</span><span class="title">strip_color</span></a>
  <a class="next-link" href="strip_delims.html"><span class="label">Next</span><span class="title">strip_delims</span></a>
</div>

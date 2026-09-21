<!-- pluss-hand-authored -->
# NoteTexAccessor::input_tags

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::input_tags() const
```

## Turn External-Input Directives Into Visible Tags

`input_tags()` replaces complete `\input{path}` and `\include{path}` commands
with the plain-text marker `[include:path]`. It exists for workflows that need
to retain the fact and target of an external source reference while making the
cell self-contained and safe to display without resolving files.

The method does **not** read the filesystem, search TeX paths, append an
extension, or splice another document into the Note. Its old expansion-shaped
name was therefore inaccurate. Use `includes()` when paths should be extracted
as a ragged result instead of embedded back into the source text.

Whitespace between the command and its required braced argument is accepted.
Balanced braces inside the path are retained. A malformed directive without a
complete braced argument remains unchanged. A doubled backslash is a TeX
linebreak boundary, so `\\input{literal}` is not mistaken for an input command.
Other commands, surrounding prose, and cell order are preserved byte for byte.

The operation is lane-wise and shape preserving for scalar, vector, matrix, and
rank-three Notes. It allocates a new Text Note and does not mutate the receiver.
Undefined input remains undefined; non-text dtypes are rejected. The scanner is
linear in source bytes and reserves output capacity from the input size.

## Example 1: Both Directive Spellings

```cpp
const auto source = pluss::make_note({
    R"(Before \input{chapters/one} after)",
    R"(\include {appendix})"});
std::cout << source.tex().input_tags() << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["Before [include:chapters/one] after", "[include:appendix]"]
```

## Example 2: Escaped And Malformed Boundaries

```cpp
const auto boundaries = pluss::make_note(
    {R"(\\input{literal})", R"(\input broken)", R"(\input{nested/{part}})"});
std::cout << boundaries.tex().input_tags() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["\\\\input{literal}", "\\input broken", "[include:nested/{part}]"]
```

## Example 3: Higher-Rank Shape Is Preserved

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{R"(Before \input{chapters/one} after)",
                             R"(\include {appendix} and \\input{literal})",
                             R"(\input{nested/{part}})", R"(\input broken)"},
    std::array<std::int64_t, 2>{2, 2});
const auto tagged = grid.tex().input_tags();
std::cout << tagged << '\n';
std::cout << "shape=[" << tagged.sizes()[0] << ", " << tagged.sizes()[1]
          << "]\n";
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["Before [include:chapters/one] after", "[include:appendix] and \\\\input{literal}"], 
  ["[include:nested/{part}]", "\\input broken"]
]
shape=[2, 2]
```

## Example 4: Undefined And Invalid Dtypes

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().input_tags().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().input_tags();
} catch (const pluss::Error&) {
  std::cout << "input tags Int64: rejected\n";
}
```

### Output

```text
false
input tags Int64: rejected
```

## See Also

- `includes`: extract include targets with per-cell offsets.
- `resolve_graphics`: turn graphics directives into visible graphic tags.
- `to_text`: perform a broader lossy TeX-to-text reduction.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="frac_slash.html"><span class="label">Previous</span><span class="title">frac_slash</span></a>
  <a class="next-link" href="macro_subs.html"><span class="label">Next</span><span class="title">macro_subs</span></a>
</div>

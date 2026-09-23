<!-- pluss-hand-authored -->
# NoteTexAccessor::vector_norm

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::vector_norm() const
```

## Rewrite Vector Notation To `\vec{...}`

`vector_norm()` rewrites every `\mathbf{...}`, `\boldsymbol{...}`, and
`\overrightarrow{...}` call, and every bare `\vec x` (a single ASCII letter,
space-separated, not yet braced), to the single canonical form
`\vec{...}`. Use it before comparing or rendering TeX math source authored
with mixed vector conventions, so `\mathbf{v}`, `\vec v`, and `\vec{v}` all
collapse to one spelling.

`\mathbf`/`\boldsymbol`/`\overrightarrow` accept any brace-balanced content
with no inner `{`/`}` (so `\mathbf{AB}`, not just single letters); the bare
`\vec x` form only matches a single letter with no braces, since a
multi-character bare form (`\vec AB`) is not valid TeX to begin with. An
already-braced `\vec{x}` is left untouched — it is already canonical, and
there is no third pass to normalize it further.

The rewrite is lexical and single-pass: it does not recurse into nested
braces, does not distinguish math mode from text mode, and treats
`\mathbf{}` (empty content) as a non-match, left unchanged. Other bold or
arrow commands (`\mathbb`, `\Vec`, `\overleftarrow`) are not recognized and
pass through untouched — only the three named commands above are rewritten.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. Undefined input remains undefined; Bool, Int64, Float64, and Bytes
receivers are rejected because TeX source is Text.

## Example 1: All Three Long Forms And The Bare Form

```cpp
std::cout << pluss::make_note(
    {"\\mathbf{v} + \\boldsymbol{w} - \\overrightarrow{AB}"})
    .tex().vector_norm()
    << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["\\vec{v} + \\vec{w} - \\vec{AB}"]
```

## Example 2: A Bare Form Next To An Already-Braced Form

```cpp
std::cout << pluss::make_note({"\\vec v + \\vec{u}"}).tex().vector_norm()
          << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["\\vec{v} + \\vec{u}"]
```

Both cells read `\vec{v} + \vec{u}` after the call; the second term was
already canonical and is copied through unchanged.

## Example 3: Non-Matches Are Left Alone

```cpp
std::cout << pluss::make_note({"plain text \\mathbf{}"}).tex().vector_norm()
          << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["plain text \\mathbf{}"]
```

An empty `\mathbf{}` group has no content to capture, so it is not a match.

## Example 4: A 2-D Grid, Shape Preserved

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{"\\mathbf{a}", "plain", "\\vec b", ""},
    std::array<std::int64_t, 2>{2, 2});
std::cout << grid.tex().vector_norm() << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["\\vec{a}", "plain"], 
  ["\\vec{b}", ""]
]
```

## Example 5: Undefined And Non-Text Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().vector_norm().defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().vector_norm();
} catch (const pluss::Error&) {
  std::cout << "Int64: rejected\n";
}
```

### Output

```text
false
Int64: rejected
```

## See Also

- `matrix_norm`: the equivalent canonicalization for matrix
  environments.
- `escape`: protect literal text rather than canonicalize existing TeX
  commands.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unescape.html"><span class="label">Previous</span><span class="title">unescape</span></a>
  <a class="next-link" href="warnings.html"><span class="label">Next</span><span class="title">warnings</span></a>
</div>

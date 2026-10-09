<!-- pluss-hand-authored -->
# Note::TexAccessor::pm_signs

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::pm_signs() const
```

## Render Paired-Sign Control Words As Plain Text

`pm_signs()` converts the complete TeX control word `\pm` to
`+/-` and `\mp` to `-/+`. The ordering is intentional: `\pm` means plus over
minus, while `\mp` carries the complementary minus-over-plus order commonly
used in a paired expression.

Matching follows TeX control-word boundaries. `\pmatrix` is one different
control word, not `\pm` followed by `atrix`, and therefore remains untouched.
The scanner also distinguishes the linebreak control symbol `\\`; letters
following that symbol are not consumed as a command. This is materially safer
than raw substring replacement in documents containing matrix-like macro names
or escaped source fragments.

The operation replaces syntax only. It does not choose one branch of an
equation, duplicate an expression into two alternatives, alter surrounding
spacing, or remove math delimiters. Comments are ordinary source text at this
stage, so call `strip_comments()` first if commented commands should be
ignored.

The result is a new shape-preserving Text Note. Unmatched cells are copied,
empty cells remain empty, and undefined input remains undefined. Native
non-Text dtypes reject. Each lane is scanned once, and output allocation is
proportional to the resulting byte count; the receiver is unchanged.

## Example 1: Both Sign Orders

```cpp
const auto signs = pluss::make_note({R"(x \pm e)", R"(y \mp d)"});
std::cout << signs.tex().pm_signs() << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["x +/- e", "y -/+ d"]
```

## Example 2: Command And Control-Symbol Boundaries

```cpp
const auto boundaries = pluss::make_note(
    {R"(x \pm e, y \mp d)", R"(\pmatrix and \\pm stay TeX)"});
std::cout << boundaries.tex().pm_signs() << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["x +/- e, y -/+ d", "\\pmatrix and \\\\pm stay TeX"]
```

## Example 3: Shape Preservation And Unmatched Cells

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{R"(a\pm b)", "plain", R"(c\mp d)", ""},
    std::array<std::int64_t, 2>{2, 2});
std::cout << grid.tex().pm_signs() << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["a+/- b", "plain"], 
  ["c-/+ d", ""]
]
```

## Example 4: Undefined Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().pm_signs().defined()
          << '\n';
```

### Output

```text
false
```

## Example 5: Non-Text Input Rejects

```cpp
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().pm_signs();
} catch (const pluss::Error&) {
  std::cout << "plus-minus Int64: rejected\n";
}
```

### Output

```text
plus-minus Int64: rejected
```

## See Also

- [`ops`](ops.md): extract mathematical operator tokens without rewriting.
- `to_text`: reduce a broader TeX fragment to readable text.
- `frac_slash`: convert structured fractions to plain division.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="packages.html"><span class="label">Previous</span><span class="title">packages</span></a>
  <a class="next-link" href="prime_norm.html"><span class="label">Next</span><span class="title">prime_norm</span></a>
</div>

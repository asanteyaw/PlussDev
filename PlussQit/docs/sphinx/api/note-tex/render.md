<!-- pluss-hand-authored -->
# NoteTexAccessor::render

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::render(std::int64_t max_passes = 1) const
```

## Build A Readable Unicode Projection

`render()` converts common authored TeX notation into compact readable Unicode
text. It expands simple local macros, unwraps common text commands, renders
fractions and roots, maps recognized symbols, converts supported scripts to
Unicode superscripts or subscripts, removes math delimiters, and collapses
layout whitespace. It is useful for previews, search labels, logs, and compact
display where retaining the TeX source is not required.

This is deliberately **not a TeX compiler**. It does not execute packages,
load files, lay out boxes, evaluate conditionals, apply fonts, or preserve a
round trip to the source. Unknown control words are discarded while balanced
brace contents remain readable. A TeX linebreak becomes whitespace. Escaped
symbols are unescaped near the end of the projection. Use [`to_text()`](to_text.md)
for a prose-oriented extraction that removes mathematics, or keep the original
Note when exact source fidelity matters.

`max_passes` controls recursive expansion of simple zero-argument macros found
in each cell. The default `1` performs one substitution pass. Larger values
permit chains such as `\A -> \B -> text` to resolve; processing stops early
when a pass makes no change. The value must be positive and is validated before
the receiver is read. It is a work bound, not an output style.

The operation is out-of-place and shape preserving. Its output is Text even
when all visible content disappears. Undefined state propagates after argument
validation; defined non-Text input rejects. Work is approximately linear in
the input for each macro pass, followed by linear lexical transformations;
large `max_passes` values should therefore be reserved for genuinely recursive
macro chains.

## Example 1: Mathematics And Readable Symbols

```cpp
const auto source = pluss::make_note({
    R"($\frac{1}{2}\alpha + x^{2} - y_1$)",
    R"(\textbf{Rate} \leq \infty; \unknown{kept})",
    R"(escaped \\alpha and \$5)"});
std::cout << source.tex().render() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["(1)/(2)α + x² - y₁", "Rate ≤ ∞; kept", "escaped alpha and $5"]
```

The unknown wrapper disappears but its braced text remains. The doubled
backslash is treated as a TeX linebreak, so the following `alpha` is ordinary
text rather than the Greek control word.

## Example 2: Matrix Shape Is Preserved

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{R"($x_1$)", R"(\alpha)", "plain", ""},
    std::array<std::int64_t, 2>{2, 2});
std::cout << grid.tex().render() << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["x₁", "α"],
  ["plain", ""]
]
```

## Example 3: Recursive Macro Expansion

```cpp
const auto source = pluss::make_note(
    R"(\newcommand{\A}{\B}\newcommand{\B}{done}\A)");
std::cout << "one=" << source.tex().render(1).scalar()
          << " two=" << source.tex().render(2).scalar() << '\n';
```

### Output

```text
one= two=done
```

After one pass, `\A` has become unresolved `\B`; the readable projection drops
that unknown control word. Two passes reach the visible body `done`.

## Example 4: Invalid Work Bounds Reject First

```cpp
try {
  (void)pluss::Note::undefined().tex().render(0);
} catch (const pluss::Error&) {
  std::cout << "render(0): rejected\n";
}
```

### Output

```text
render(0): rejected
```

## Example 5: Undefined State And Dtype

```cpp
std::cout << std::boolalpha << "undefined="
          << pluss::Note::undefined().tex().render().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().render();
} catch (const pluss::Error&) {
  std::cout << "render Int64: rejected\n";
}
```

### Output

```text
undefined=false
render Int64: rejected
```

## See Also

- [`to_text`](to_text.md): recover prose while removing mathematical regions.
- [`macro_subs`](macro_subs.md): expand macros while retaining TeX syntax.
- [`frac_slash`](frac_slash.md): convert only fraction notation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="refs.html"><span class="label">Previous</span><span class="title">refs</span></a>
  <a class="next-link" href="sections.html"><span class="label">Next</span><span class="title">sections</span></a>
</div>

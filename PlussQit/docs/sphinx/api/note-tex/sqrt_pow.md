<!-- pluss-hand-authored -->
# Note::TexAccessor::sqrt_pow

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::sqrt_pow() const
```

## Rewrite Roots As Reciprocal Powers

`sqrt_pow()` rewrites a complete square root `\sqrt{x}` as
`(x)^(1/2)` and an indexed root `\sqrt[n]{x}` as `(x)^(1/n)`. Parenthesizing
both the radicand and exponent denominator keeps the textual precedence
explicit when the rewritten fragment is embedded in a larger expression.

The parser accepts ASCII spacing between the command, optional degree, and
required radicand. Square brackets and braces must balance. Nested braces are
part of one radicand, and nested `\sqrt` calls are converted recursively. The
degree and radicand are source expressions, not numeric parameters: the method
retains their text and does not test whether a degree is zero, simplify powers,
or evaluate a root.

Only the complete `\sqrt` control word is eligible. A longer name such as
`\sqrted`, an incomplete optional group, a missing braced radicand, or text
following the linebreak symbol `\\` remains unchanged. This preservation rule
avoids producing half-rewritten source from malformed editor buffers.

The operation returns a fresh Text Note with identical rank and shape. Cells
without complete roots pass through unchanged; empty and undefined states are
preserved. Non-Text dtypes reject. Processing is a lexical pass with recursive
work proportional to nested root content, and no mutation occurs.

## Example 1: Square And Indexed Roots

```cpp
const auto roots = pluss::make_note({R"(\sqrt{x})", R"(\sqrt[3]{a+b})"});
std::cout << roots.tex().sqrt_pow() << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["(x)^(1/2)", "(a+b)^(1/3)"]
```

## Example 2: Nested Roots

```cpp
const auto nested = pluss::make_note(R"(\sqrt{x} + \sqrt[3]{a+\sqrt{b}})");
std::cout << nested.tex().sqrt_pow() << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"(x)^(1/2) + (a+(b)^(1/2))^(1/3)"
```

## Example 3: Prefixes, Linebreaks, And Incomplete Calls Survive

```cpp
const auto incomplete =
    pluss::make_note(R"(\sqrted{x} + \\sqrt{x} + \sqrt[3]oops)");
std::cout << incomplete.tex().sqrt_pow() << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"\\sqrted{x} + \\\\sqrt{x} + \\sqrt[3]oops"
```

## Example 4: Higher-Rank Shape Is Preserved

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{R"(\sqrt{a})", "plain",
                             R"(\sqrt[4]{b})", ""},
    std::array<std::int64_t, 2>{2, 2});
std::cout << grid.tex().sqrt_pow() << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["(a)^(1/2)", "plain"], 
  ["(b)^(1/4)", ""]
]
```

## Example 5: Undefined And Invalid Dtype

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().sqrt_pow().defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().sqrt_pow();
} catch (const pluss::Error&) {
  std::cout << "roots Int64: rejected\n";
}
```

### Output

```text
false
roots Int64: rejected
```

## See Also

- `simplify_roots`: normalize root spelling while keeping TeX root syntax.
- `frac_slash`: rewrite fraction constructs as explicit division.
- [`math_funcs`](math_funcs.md): extract recognized mathematical function commands.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="simplify_roots.html"><span class="label">Previous</span><span class="title">simplify_roots</span></a>
  <a class="next-link" href="strip_bib.html"><span class="label">Next</span><span class="title">strip_bib</span></a>
</div>

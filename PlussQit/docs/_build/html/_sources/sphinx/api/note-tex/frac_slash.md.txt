<!-- pluss-hand-authored -->
# NoteTexAccessor::frac_slash

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::frac_slash() const
```

## Rewrite Fraction Commands As Explicit Division

`frac_slash()` rewrites complete `\frac`, `\dfrac`, `\tfrac`, and
`\cfrac` calls from `\frac{numerator}{denominator}` to
`(numerator)/(denominator)`. Parentheses are always inserted around both
operands. They preserve the fraction's grouping when the result is embedded in
a larger expression; the method does not guess whether either pair is
stylistically dispensable.

Both required groups must be balanced before a call is converted. Nested
braces are understood, and nested supported fraction commands are converted
recursively from the inside of each operand. Whitespace between the control
word and its groups is accepted; leading and trailing ASCII whitespace inside
each operand is removed. Other content inside an operand is retained exactly.

This is a source rewrite, not symbolic algebra. It does not evaluate numbers,
cancel factors, distribute division, convert roots, or remove math delimiters.
Likewise, a similarly prefixed command such as `\fraction` is unrelated and
remains unchanged. A command following the linebreak control symbol `\\` is
also left alone, and incomplete calls are preserved rather than partially
destroyed. Those boundaries make the operation safe for preprocessing mixed
TeX source where only complete fraction constructs should change.

The output is a newly allocated Text Note with the receiver's exact rank and
shape. Empty cells remain empty and cells without a convertible call are
copied unchanged. Undefined input remains undefined. Bool, Int64, Float64, and
Bytes Notes reject because their lanes are not TeX source. Runtime is linear
in ordinary source length, with additional traversal proportional to nested
fraction content; the receiver is never mutated.

## Example 1: The Four Fraction Spellings

```cpp
const auto source = pluss::make_note(
    R"(\frac{1}{2} + \dfrac{a+b}{c} + \tfrac{m}{n} + \cfrac{p}{q})");
std::cout << source.tex().frac_slash() << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"(1)/(2) + (a+b)/(c) + (m)/(n) + (p)/(q)"
```

## Example 2: Nested Fractions

```cpp
const auto fractions = pluss::make_note({
    R"(\frac{1}{2} + \dfrac{a+b}{c} + \cfrac{\frac{x}{y}}{z})",
    R"(\fraction{1}{2} + \\frac{3}{4} + \frac{broken})"});

std::cout << fractions.tex().frac_slash() << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["(1)/(2) + (a+b)/(c) + ((x)/(y))/(z)", "\\fraction{1}{2} + \\\\frac{3}{4} + \\frac{broken}"]
```

The nested numerator is recursively rewritten. The second lane demonstrates
all three preservation boundaries: a different command name, a linebreak, and
an incomplete second argument.

## Example 3: Higher-Rank Shape Is Preserved

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{R"(\frac{a}{b})", "plain",
                             R"(\tfrac{x}{y})", ""},
    std::array<std::int64_t, 2>{2, 2});
std::cout << grid.tex().frac_slash() << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["(a)/(b)", "plain"], 
  ["(x)/(y)", ""]
]
```

## Example 4: Undefined Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().frac_slash().defined()
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
      .tex().frac_slash();
} catch (const pluss::Error&) {
  std::cout << "fractions Int64: rejected\n";
}
```

### Output

```text
fractions Int64: rejected
```

## See Also

- `simplify_fractions`: canonicalize fraction syntax while retaining TeX form.
- `sqrt_pow`: rewrite root commands as explicit powers.
- `to_text`: remove broader TeX structure for readable prose.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="escape.html"><span class="label">Previous</span><span class="title">escape</span></a>
  <a class="next-link" href="input_tags.html"><span class="label">Next</span><span class="title">input_tags</span></a>
</div>

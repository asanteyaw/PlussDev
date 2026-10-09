<!-- pluss-hand-authored -->
# Note::TexAccessor::simplify_fractions

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::simplify_fractions() const
```

## Canonicalize Fraction Source Without Rendering It

`simplify_fractions()` rewrites complete `\frac`, `\dfrac`, `\tfrac`, and
`\cfrac` calls to one canonical `\frac{numerator}{denominator}` spelling. Use it
when TeX must remain TeX but display-style variants and incidental outer
whitespace should no longer affect comparison or downstream processing. Use
[`frac_slash()`](frac_slash.md) when the desired result is arithmetic text such
as `(a)/(b)`, and [`canonicalize_macros()`](canonicalize_macros.md) when several
unrelated legacy control words should be normalized together.

Matching is lexical, exact, and case-sensitive. Both required arguments must be
balanced braced groups. Nested braces and nested fraction calls are supported;
recognized fractions inside either argument are normalized recursively. Only
ASCII whitespace at each argument's outer edge is removed. Content inside the
argument, unrelated commands, escaped linebreak-like spellings, longer command
names, and incomplete calls remain unchanged.

The transform is idempotent, out-of-place, and shape preserving. It returns
Text, propagates undefined state, and rejects defined non-Text Notes. Its scanner
is linear in source bytes; recursion follows syntactic nesting depth and does
not invoke a TeX engine.

## Example 1: Variants, Nesting, And Outer Whitespace

```cpp
const auto source = pluss::make_note({
    R"(\dfrac { a + {b} } { \tfrac{ c }{ d } })",
    R"(\cfrac{x}{y} + \frac { 1 } { 2 })"});
std::cout << source.tex().simplify_fractions() << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["\\frac{a + {b}}{\\frac{c}{d}}", "\\frac{x}{y} + \\frac{1}{2}"]
```

## Example 2: Near Misses And Malformed Calls Survive

```cpp
const auto source = pluss::make_note(
    R"(\operatorname{x} \fraction{a}{b} \\dfrac{a}{b} \frac{broken})");
std::cout << source.tex().simplify_fractions() << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"\\operatorname{x} \\fraction{a}{b} \\\\dfrac{a}{b} \\frac{broken}"
```

## Example 3: Matrix Shape Is Preserved

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{R"(\tfrac{a}{b})", "plain",
                             R"(\dfrac {x}{y})", ""},
    std::array<std::int64_t, 2>{2, 2});
std::cout << grid.tex().simplify_fractions() << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["\\frac{a}{b}", "plain"],
  ["\\frac{x}{y}", ""]
]
```

## Example 4: Idempotence, Undefined State, And Dtype

```cpp
const auto once = pluss::make_note(R"(\dfrac {a}{b})")
                      .tex().simplify_fractions();
std::cout << std::boolalpha
          << "idempotent="
          << ((once == once.tex().simplify_fractions()).all().bool_item())
          << " undefined="
          << pluss::Note::undefined().tex().simplify_fractions().defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex()
      .simplify_fractions();
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

- [`frac_slash`](frac_slash.md): project complete fractions into slash form.
- [`canonicalize_macros`](canonicalize_macros.md): normalize a broader control-word set.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_macro.html"><span class="label">Previous</span><span class="title">set_macro</span></a>
  <a class="next-link" href="simplify_roots.html"><span class="label">Next</span><span class="title">simplify_roots</span></a>
</div>

<!-- pluss-hand-authored -->
# Note::TexAccessor::strip_delims

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::strip_delims() const
```

## Remove Math Markers, Retain Their Payload

`strip_delims()` removes the six lexical math markers `$`, `$$`, `\(`,
`\)`, `\[`, and `\]` from every Text cell. Formula content and all other TeX
commands remain in source form. Use it when delimiters are unwanted but the
mixture of prose and formulas must stay in its original order; use
[`math_only()`](math_only.md) when only formula bodies should survive, or
`text_only()` when formulas themselves should disappear.

This is deliberately a lexical transform, not a balance check. Every
recognized marker is removed independently, including an unmatched opening or
closing marker. Escaped literal dollars (`\$`) and doubled-backslash text are
preserved. The method does not remove braces, evaluate commands, insert spaces,
or collapse existing whitespace, so adjacent source separated only by a marker
can become adjacent in the result.

The operation is out-of-place, idempotent, and preserves rank, shape, cell
order, and Text dtype. Undefined input propagates; defined non-Text Notes
reject. It performs one linear scan and allocates at most the retained byte
count for each cell.

## Example 1: Every Delimiter Form And Escaped Literals

```cpp
const auto source = pluss::make_note({
    R"(A $x+1$ B $$y$$ C \(z\) D \[q\])",
    R"(price \$5; escaped \\(literal\\); unmatched $tail)",
    "plain text"});
std::cout << source.tex().strip_delims() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["A x+1 B y C z D q", "price \\$5; escaped \\\\(literal\\\\); unmatched tail", "plain text"]
```

The unmatched dollar is still a delimiter token and disappears. The escaped
dollar and doubled backslashes are literal source and remain.

## Example 2: Shape And Idempotence

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{"$a$", R"(\(b\))", R"(\$3)", ""},
    std::array<std::int64_t, 2>{2, 2});
const auto clean = grid.tex().strip_delims();
std::cout << clean << '\n';
std::cout << "shape=[" << clean.size(0) << ", " << clean.size(1) << "]\n";
std::cout << std::boolalpha
          << "idempotent=" << ((clean == clean.tex().strip_delims()).all().bool_item())
          << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["a", "b"], 
  ["\\$3", ""]
]
shape=[2, 2]
idempotent=true
```

## Example 3: Undefined And Invalid Dtype

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().strip_delims().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().strip_delims();
} catch (const pluss::Error&) {
  std::cout << "strip delims Int64: rejected\n";
}
```

### Output

```text
false
strip delims Int64: rejected
```

## See Also

- [`add_math_delimiters`](add_math_delimiters.md): wrap an undelimited cell as one formula.
- [`balance_math_delims`](balance_math_delims.md): repair unmatched math markers.
- [`math_only`](math_only.md): project each cell onto its formula bodies.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_comments.html"><span class="label">Previous</span><span class="title">strip_comments</span></a>
  <a class="next-link" href="strip_env.html"><span class="label">Next</span><span class="title">strip_env</span></a>
</div>

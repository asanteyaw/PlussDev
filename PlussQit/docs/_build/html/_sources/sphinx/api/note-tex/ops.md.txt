<!-- pluss-hand-authored -->
# NoteTexAccessor::ops

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteTexAccessor::ops() const
```

## Extract Mathematical Operator Lexemes

`ops()` selects the ASCII operators `+ - * / = < > ^` and the TeX operator
commands `\times`, `\cdot`, `\pm`, `\mp`, `\leq`, `\geq`, `\neq`, and
`\approx` from complete math regions. Command tokens retain their leading
backslash, making a TeX command distinguishable from an ASCII character.

This is lexical inventory rather than precedence analysis. Unary and binary
minus are the same token, repeated operators remain repeated, and the method
does not build an expression tree. Commands outside the explicit vocabulary
are omitted even if a document defines them as operators. Use `symbols()` when
all control words should join the operator stream.

Prose and malformed math regions are ignored. Results follow source order
across each cell and carry lane ownership in `offsets()`. Undefined input
propagates; Bool, Int64, and Float64 inputs are rejected.

## Example 1: ASCII And TeX Operators

```cpp
const auto source = pluss::make_note(
    std::vector<std::string>{
        R"(prose 99; $x_1 + \sin(\alpha) = 2.5$)",
        R"(\[\beta \leq y^2\] and \(\sqrt{z}\))",
        R"(escaped \$7 and plain theta)",
        R"($$\argmax_x f(x) \approx 10$$)"},
    std::array<std::int64_t, 2>{2, 2});
const auto result = source.tex().ops();
std::cout << result.tokens() << '\n';
```

### Output

```text
Note(shape=[5], dim=1, numel=5, encoding=utf8)
["+", "=", "\\leq", "^", "\\approx"]
```

## Example 2: Prose Operators Do Not Count

```cpp
const auto offsets = result.offsets().integers();
for (auto value : offsets) std::cout << value << ' ';
std::cout << '\n';
```

### Output

```text
0 2 4 4 5 
```

The repeated `4` confirms that the third cell's escaped dollar and prose
contributed no math operator.

## Example 3: Undefined And Invalid Dtypes

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().ops().tokens().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().ops();
} catch (const pluss::Error&) { std::cout << "operators Int64: rejected\n"; }
```

### Output

```text
false
operators Int64: rejected
```

## See Also

- [`symbols`](symbols.md): include every TeX control word as well as operators.
- [`nums`](nums.md): extract number lexemes without their signs.
- [`pm_signs`](pm_signs.md): rewrite paired-sign commands instead of extracting them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nums.html"><span class="label">Previous</span><span class="title">nums</span></a>
  <a class="next-link" href="packages.html"><span class="label">Next</span><span class="title">packages</span></a>
</div>

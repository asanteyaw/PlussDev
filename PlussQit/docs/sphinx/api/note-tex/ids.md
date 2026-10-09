<!-- pluss-hand-authored -->
# Note::TexAccessor::ids

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch Note::TexAccessor::ids() const
```

## Extract ASCII Identifier Lexemes

`ids()` returns identifier-shaped lexemes found inside complete math regions.
An identifier begins with an ASCII letter and may continue with ASCII letters,
digits, or underscores. Thus `x`, `x_1`, and `argmax_x` are each one token.
Numbers cannot begin an identifier.

This is intentionally lexical rather than semantic. In `\sin(\alpha)`, both
`sin` and `alpha` are identifier lexemes because the backslash is not part of
the identifier pattern. Choose `math_funcs()` or `greek()` when category
membership matters, and `symbols()` when the leading backslash must survive.
No symbol table, declaration lookup, or expression parser is involved.

Prose outside math and incomplete or escaped math regions are ignored. Tokens
retain repetitions and source order, and offsets map the flattened result back
to every input cell. Undefined input propagates; non-text dtypes are rejected.

## Example 1: Variables And Command Bodies

```cpp
const auto source = pluss::make_note(
    std::vector<std::string>{
        R"(prose 99; $x_1 + \sin(\alpha) = 2.5$)",
        R"(\[\beta \leq y^2\] and \(\sqrt{z}\))",
        R"(escaped \$7 and plain theta)",
        R"($$\argmax_x f(x) \approx 10$$)"},
    std::array<std::int64_t, 2>{2, 2});
const auto result = source.tex().ids();
std::cout << result.tokens() << '\n';
```

### Output

```text
Note(shape=[12], dim=1, numel=12, encoding=utf8)
["x_1", "sin", "alpha", "beta", "leq", "y", "sqrt", "z", "argmax_x", "f", "x", "approx"]
```

There is no token for prose `theta` or the exponent `2`. Command bodies such
as `leq` and `approx` do appear because this method is deliberately lexical.

## Example 2: Read A Single Cell's Range

```cpp
const auto offsets = result.offsets().integers();
for (std::int64_t i = offsets[1]; i < offsets[2]; ++i)
  std::cout << result.tokens().at(i) << '\n';
```

### Output

```text
beta
leq
y
sqrt
z
```

## Example 3: Undefined And Invalid Dtypes

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().ids().tokens().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().ids();
} catch (const pluss::Error&) { std::cout << "identifiers Int64: rejected\n"; }
```

### Output

```text
false
identifiers Int64: rejected
```

## See Also

- [`nums`](nums.md): extract unsigned decimal-number lexemes.
- [`math_funcs`](math_funcs.md): classify recognized function commands.
- [`greek`](greek.md): classify recognized Greek-letter commands.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="headings.html"><span class="label">Previous</span><span class="title">headings</span></a>
  <a class="next-link" href="includes.html"><span class="label">Next</span><span class="title">includes</span></a>
</div>

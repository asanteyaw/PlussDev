<!-- pluss-hand-authored -->
# NoteTexAccessor::symbols

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteTexAccessor::symbols() const
```

## Inventory Control Words And Operators

`symbols()` extracts every alphabetic TeX control word together with the ASCII
operators `+ - * / = < > ^` from complete math regions. It is the broadest
lexical inventory in this tranche: `\sin`, `\alpha`, and a project-local
`\foo` are all symbols, with their leading backslash retained.

The method does not classify or deduplicate. Use `math_funcs()` or `greek()`
for known semantic subsets, and `ops()` for the narrower operator vocabulary.
Arguments are scanned independently, so `\frac{x}{y}` contributes `\frac`
but `x` and `y` are available through `ids()`, not this operation. Control
symbols such as `\%` are not alphabetic control words and are not returned.

Only complete anonymous math delimiters establish the search region. Escaped
dollars and prose are ignored. Tokens preserve source order and repetitions;
offsets preserve input-cell ownership. Undefined input propagates and non-text
input throws.

## Example 1: A Broad Symbol Stream

```cpp
const auto source = pluss::make_note(
    std::vector<std::string>{
        R"(prose \alpha; $x + \sin(\alpha) = 2.5$)",
        R"(\[\beta \leq y^2\] and \(\sqrt{z}\))",
        R"(escaped \$7)", R"($$\argmax_x f(x) \approx 10$$)"},
    std::array<std::int64_t, 2>{2, 2});
const auto result = source.tex().symbols();
std::cout << result.tokens() << '\n';
```

### Output

```text
Note(shape=[10], dim=1, numel=10, encoding=utf8)
["+", "\\sin", "\\alpha", "=", "\\beta", "\\leq", "^", "\\sqrt", "\\argmax", "\\approx"]
```

## Example 2: Rank-Two Ownership

```cpp
const auto offsets = result.offsets().integers();
for (auto value : offsets) std::cout << value << ' ';
std::cout << '\n';
```

### Output

```text
0 4 8 8 10 
```

## Example 3: Undefined And Invalid Dtypes

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().symbols().tokens().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().symbols();
} catch (const pluss::Error&) { std::cout << "symbols Int64: rejected\n"; }
```

### Output

```text
false
symbols Int64: rejected
```

## See Also

- [`ops`](ops.md): restrict extraction to the operator vocabulary.
- [`math_funcs`](math_funcs.md): classify recognized function commands.
- [`greek`](greek.md): classify recognized lowercase Greek commands.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sum_norm.html"><span class="label">Previous</span><span class="title">sum_norm</span></a>
  <a class="next-link" href="sup_norm.html"><span class="label">Next</span><span class="title">sup_norm</span></a>
</div>

<!-- pluss-hand-authored -->
# Note::TexAccessor::math_funcs

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch Note::TexAccessor::math_funcs() const
```

## Recognize Function Commands Inside Math

`math_funcs()` extracts the names of recognized mathematical function control
words from complete math fragments. The current vocabulary is `sin`, `cos`,
`tan`, `log`, `ln`, `exp`, `max`, `min`, `argmax`, `argmin`, `det`, `Pr`, and
`sqrt`. Tokens omit the leading backslash: `\sin` contributes `"sin"`.

Only content inside `$...$`, `$$...$$`, `\(...\)`, or `\[...\]` is searched.
The same words in prose are ignored, as are escaped dollar signs and incomplete
math regions. Matching is case-sensitive and requires a complete control-word
boundary, so a longer custom command is not classified by prefix. This is a
lexical classifier, not expression evaluation: it does not infer that `f(x)`
is a function or validate argument arity.

Results preserve source order and repetition. They use `TokenBatch` offsets to
retain cell ownership while flattening variable token counts. Undefined input
propagates as undefined tokens; non-text input is rejected. Runtime is linear
in scanned bytes plus recognized tokens.

## Example 1: Functions Across Four Math Delimiters

```cpp
const auto source = pluss::make_note(
    std::vector<std::string>{
        R"(prose 99; $x_1 + \sin(\alpha) = 2.5$)",
        R"(\[\beta \leq y^2\] and \(\sqrt{z}\))",
        R"(escaped \$7 and plain theta)",
        R"($$\argmax_x f(x) \approx 10$$)"},
    std::array<std::int64_t, 2>{2, 2});
const auto result = source.tex().math_funcs();
std::cout << result.tokens() << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["sin", "sqrt"]
```

## Example 2: Cell Ownership

```cpp
const auto offsets = result.offsets().integers();
for (auto value : offsets) std::cout << value << ' ';
std::cout << '\n';
```

### Output

```text
0 1 2 2 2 
```

`\argmax_x` is one longer control word, not the exact built-in `\argmax`, so
the boundary rule excludes it.

## Example 3: Undefined Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().math_funcs().tokens().defined()
          << '\n';
```

### Output

```text
false
```

## See Also

- [`ids`](ids.md): return all ASCII identifier lexemes, including command names.
- [`symbols`](symbols.md): retain complete control words with their backslashes.
- [`sqrt_pow`](sqrt_pow.md): transform roots instead of classifying them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="math_blocks.html"><span class="label">Previous</span><span class="title">math_blocks</span></a>
  <a class="next-link" href="math_only.html"><span class="label">Next</span><span class="title">math_only</span></a>
</div>

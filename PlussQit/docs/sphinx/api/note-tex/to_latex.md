<!-- pluss-hand-authored -->
# Note::TexAccessor::to_latex

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::to_latex(std::int64_t mode = 0) const
```

## Convert Plain Text Toward TeX Source

`to_latex()` is the opposite direction from [`to_text`](to_text.md) and
[`render`](render.md): where those reduce authored TeX down to plain text
or Unicode, `to_latex()` builds TeX source up from plain text. It escapes
every cell the same way [`escape`](escape.md) does, then — depending on
`mode` — maps common math words and comparison operators to their TeX
commands and wraps the result in `$...$` when it looks like a formula.

Unlike `to_text`'s and `to_markdown`'s `mode`, **this one is validated**:
only `0`, `1`, and `2` are accepted, and any other value — including
negative numbers, which `to_text`'s `mode` treats as `0` — is rejected.

### `mode`

- **`0`** (the default) — escape only, exactly [`escape()`](escape.md)'s
  ten-character table. No word or operator mapping, no wrapping.
- **`1`** — escape, then map recognized math words (`alpha` → `\alpha`,
  `infty`/`inf` → `\infty`, `sqrt` → `\sqrt`, and similar) and comparison
  operators (`<=` → `\leq `, `>=` → `\geq `, `!=` → `\neq `, `->` → `\to `,
  and related arrows/signs), then collapse and trim whitespace. A
  `sqrt(...)` call is additionally rewritten to `\sqrt{...}` braces.
- **`2`** — everything `1` does, and additionally wraps the result in
  `$...$` **when it looks like a formula**: the escaped/mapped text
  contains a backslash (any `\command`, from mode 1's own mapping or
  already present in the source) or one of `= + - * / ^ _ < >`. A cell with
  none of those — ordinary prose — is left unwrapped even at mode 2.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. Undefined input remains undefined; Bool, Int64, Float64, and Bytes
receivers are rejected because the input is treated as plain text.

## Example 1: All Three Modes On The Same Source

```cpp
const auto source = pluss::make_note(
    {"x_1 & 50% complete", "alpha + beta <= gamma",
     "plain words", "sqrt(x) -> infty"});
std::cout << source.tex().to_latex(0) << '\n';
std::cout << source.tex().to_latex(1) << '\n';
std::cout << source.tex().to_latex(2) << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["x\\_1 \\& 50\\% complete", "alpha + beta <= gamma", "plain words", "sqrt(x) -> infty"]
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["x\\_1 \\& 50\\% complete", "\\alpha + \\beta \\leq \\gamma", "plain words", "\\sqrt{x} \\to \\infty"]
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["$x\\_1 \\& 50\\% complete$", "$\\alpha + \\beta \\leq \\gamma$", "plain words", "$\\sqrt{x} \\to \\infty$"]
```

At mode 0, only the special characters are escaped — `alpha`, `<=`, and
`sqrt(x) -> infty` are untouched. At mode 1, the words and operators map to
commands. At mode 2, every cell whose mapped text contains `\` or one of
`=+-*/^_<>` gets wrapped in `$...$` — three of the four do; `"plain words"`
has neither and stays bare even though it went through the same mode-2
call.

## Example 2: A 2-D Grid At The Default Mode

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{"a_b", "alpha", "x+y", ""},
    std::array<std::int64_t, 2>{2, 2});
std::cout << grid.tex().to_latex() << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["a\\_b", "alpha"], 
  ["x+y", ""]
]
```

`to_latex()` with no argument is mode 0: `a_b` is escaped to `a\_b`, but
`alpha` is left as the literal word (mode 0 does no word mapping).

## Example 3: An Out-Of-Range Mode Is Rejected

```cpp
for (const auto mode : {-1, 3}) {
  try {
    (void)source.tex().to_latex(mode);
  } catch (const pluss::Error&) {
    std::cout << "to_latex(" << mode << "): rejected\n";
  }
}
```

### Output

```text
to_latex(-1): rejected
to_latex(3): rejected
```

Only `0`, `1`, and `2` are valid; every other integer, including negative
values, is rejected before the receiver is read.

## Example 4: Undefined And Non-Text Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().to_latex().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().to_latex();
} catch (const pluss::Error&) {
  std::cout << "to latex Int64: rejected\n";
}
```

### Output

```text
false
to latex Int64: rejected
```

## See Also

- [`escape`](escape.md): exactly `to_latex(0)`'s special-character table,
  callable on its own.
- [`to_text`](to_text.md): the inverse direction — reduce authored TeX to
  plain text.
- [`render`](render.md): render TeX toward readable Unicode instead of
  building TeX source.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_html.html"><span class="label">Previous</span><span class="title">to_html</span></a>
  <a class="next-link" href="to_markdown.html"><span class="label">Next</span><span class="title">to_markdown</span></a>
</div>

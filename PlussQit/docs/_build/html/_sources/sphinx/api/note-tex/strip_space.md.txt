<!-- pluss-hand-authored -->
# Note::TexAccessor::strip_space

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::strip_space() const
```

## Remove Explicit TeX Spacing Syntax

`strip_space()` removes authored spacing controls while retaining neighboring
visible content. It recognizes the control symbols `\,`, `\;`, `\:`, `\!`,
`\ `, and `\/`; the exact control words `\quad`, `\qquad`, `\enspace`,
`\thinspace`, `\medspace`, and `\thickspace`; balanced `\hspace{...}` and
`\vspace{...}` calls, including starred forms; and the first dimension token
after `\vskip`.

Removed syntax contributes a separator before ASCII whitespace is collapsed
and the cell is trimmed. This avoids accidentally joining tokens such as
`left\quad right`. The method removes spacing instructions, not all source
whitespace and not visible delimiter-size commands. Use
[`strip_lr()`](strip_lr.md) or [`strip_size()`](strip_size.md) for those
separate concerns.

Matching is lexical and exact. A doubled backslash protects the following
text, longer names such as `\quadratic` remain, and an incomplete balanced
argument is retained rather than partially consumed. The operation is
out-of-place, shape preserving, and idempotent. Undefined state propagates;
defined non-Text input rejects. Each cell is scanned once, with temporary
storage bounded by the balanced argument depth.

## Example 1: Control Symbols, Words, And Dimension Calls

```cpp
const auto source = pluss::make_note({
    R"(a\,b\;c\:d\!e\ f\/g\quad h\qquad i)",
    R"(x\hspace*{1{e}m}y\vspace{2pt}z\vskip 3pt q)",
    R"(\\quad \quadratic \hspace{unfinished)"});
std::cout << source.tex().strip_space() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["a b c d e f g h i", "x y z q", "\\\\quad \\quadratic \\hspace{unfinished"]
```

## Example 2: Matrix Shape And Empty Cells

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{R"(a\,b)", "plain", R"(c\quad d)", ""},
    std::array<std::int64_t, 2>{2, 2});
std::cout << grid.tex().strip_space() << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["a b", "plain"],
  ["c d", ""]
]
```

## Example 3: Idempotence, Undefined State, And Dtype

```cpp
const auto once = pluss::make_note(R"(a\quad b)").tex().strip_space();
std::cout << std::boolalpha
          << "idempotent="
          << ((once == once.tex().strip_space()).all().bool_item())
          << " undefined="
          << pluss::Note::undefined().tex().strip_space().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().strip_space();
} catch (const pluss::Error&) {
  std::cout << "strip space Int64: rejected\n";
}
```

### Output

```text
idempotent=true undefined=false
strip space Int64: rejected
```

## See Also

- [`strip_lr`](strip_lr.md): remove automatic delimiter-size controls.
- [`strip_size`](strip_size.md): remove explicit `big`-family controls.
- [`dedup_blank_lines`](dedup_blank_lines.md): cap vertical blank-line runs.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_size.html"><span class="label">Previous</span><span class="title">strip_size</span></a>
  <a class="next-link" href="strip_tables.html"><span class="label">Next</span><span class="title">strip_tables</span></a>
</div>

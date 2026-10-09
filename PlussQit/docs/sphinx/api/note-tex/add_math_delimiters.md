<!-- pluss-hand-authored -->
# Note::TexAccessor::add_math_delimiters

[Back to Note Tex Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::add_math_delimiters() const
```

## Make Each Cell A Complete Math Fragment

`add_math_delimiters()` returns a shape-preserving Text Note in which every
non-empty cell has one complete outer math delimiter pair. Use it when each
cell is an expression that must be embedded independently in TeX source. It
does not decide whether prose is mathematical; the caller makes that semantic
choice by selecting this operation.

Plain text is trimmed of surrounding ASCII whitespace and wrapped in `$...$`.
Cells already enclosed by `$...$`, `$$...$$`, `\(...\)`, or `\[...\]` retain
their delimiter style. A one-sided outer delimiter is completed rather than
wrapped again. Empty or whitespace-only cells become `""`, not `"$$"`.

The method is idempotent: applying it to its own output produces the same
values. It operates cell by cell, preserves rank and every extent, and
allocates fresh Text storage. Undefined propagates. Bytes and native Bool,
Int64, or Float64 Notes reject because no implicit decoding or formatting is
performed. Work is linear in the total input bytes.

This is lexical wrapping, not TeX validation. It does not parse an expression,
check command names, or repair delimiters nested inside a cell. Use
[`balance_math_delims()`](balance_math_delims.md) when an authored fragment's
internal outer delimiters may be incomplete.

## Example 1: Plain, Complete, Partial, And Empty Cells

```cpp
const auto expressions = pluss::make_note(
    {" x+y ", "$z$", R"(\(a+b\))", R"(\[c=d\])", "", "$u", R"(v\))"});
std::cout << expressions.tex().add_math_delimiters() << '\n';
```

### Output

```text
Note(shape=[7], dim=1, numel=7, encoding=utf8)
["$x+y$", "$z$", "\\(a+b\\)", "\\[c=d\\]", "", "$u$", "\\(v\\)"]
```

The existing delimiter convention is preserved; only the plain expression
chooses the default dollar pair.

## Example 2: Matrix Geometry Is Preserved

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{"a", "b", "$c$", ""},
    std::array<std::int64_t, 2>{2, 2});
std::cout << grid.tex().add_math_delimiters() << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["$a$", "$b$"], 
  ["$c$", ""]
]
```

## Example 3: Undefined Propagates

```cpp
std::cout << std::boolalpha << "undefined="
          << pluss::Note::undefined().tex().add_math_delimiters().defined()
          << '\n';
```

### Output

```text
undefined=false
```

## Example 4: Native Numeric Storage Is Not Stringified

```cpp
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().add_math_delimiters();
} catch (const pluss::Error&) {
  std::cout << "Int64 add delimiters: rejected\n";
}
```

### Output

```text
Int64 add delimiters: rejected
```

## See Also

- [`balance_math_delims`](balance_math_delims.md): repair unmatched outer math delimiters.
- [`escape`](escape.md): protect literal text rather than marking it as mathematics.
- `wrap_inline_math` and `wrap_display_math`: request a particular delimiter spelling.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../note_tex_accessor_api_reference.html"><span class="label">Previous</span><span class="title">Note TeX Accessor</span></a>
  <a class="next-link" href="align_blocks.html"><span class="label">Next</span><span class="title">align_blocks</span></a>
</div>

<!-- pluss-hand-authored -->
# NoteTexAccessor::balance_math_delims

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::balance_math_delims() const
```

## Complete Unmatched Math Delimiters

`balance_math_delims()` repairs the three math-delimiter families recognized
by the lexical TeX surface: unescaped dollar signs, `\(...\)`, and `\[...\]`.
Use it when fragments may have been clipped at a cell boundary but their
delimiter spelling should otherwise remain unchanged.

For dollar math, an odd number of unescaped `$` bytes receives one trailing
`$`. A dollar preceded by an odd run of backslashes is escaped and does not
participate. For parenthesized and bracketed math, the method counts opening
and closing markers independently. Missing closes are appended; missing opens
are prepended. Consequently `\(a+b` becomes `\(a+b\)`, while `a+b\)` becomes
`\(a+b\)`.

The method does not convert inline math to display math, pair individual
markers by nesting position, understand comments, or validate expression
syntax. It only equalizes each delimiter family's counts. In particular,
crossed or interleaved delimiters can be count-balanced while still being
invalid TeX. Choose [`add_math_delimiters()`](add_math_delimiters.md) when a
cell is a plain expression that needs one complete outer pair; choose this
method when delimiter characters are already part of the authored fragment.

The operation is idempotent, cell-independent, and shape-preserving. It
returns newly owned Text storage, leaves empty and already balanced cells
unchanged, and propagates undefined. Non-Text Notes reject. The scan is linear
in input bytes; prepending missing `\(` or `\[` markers may move a cell's
existing bytes once during output construction.

## Example 1: Every Supported Delimiter Family

```cpp
const auto fragments = pluss::make_note(
    {"$x+y", R"(\(a+b)", R"(c+d\))", R"(\[e=f)", R"(g=h\])", R"(\$5)"});
std::cout << fragments.tex().balance_math_delims() << '\n';
```

### Output

```text
Note(shape=[6], dim=1, numel=6, encoding=utf8)
["$x+y$", "\\(a+b\\)", "\\(c+d\\)", "\\[e=f\\]", "\\[g=h\\]", "\\$5"]
```

The escaped dollar in the final cell is literal currency text, so no closing
dollar is introduced.

## Example 2: Balancing Is Idempotent And Undefined Propagates

```cpp
const auto balanced = fragments.tex().balance_math_delims();
const bool idempotent =
    (balanced.tex().balance_math_delims() == balanced).all().bool_item();
std::cout << std::boolalpha << "idempotent=" << idempotent
          << " undefined="
          << pluss::Note::undefined().tex().balance_math_delims().defined()
          << '\n';
```

### Output

```text
idempotent=true undefined=false
```

## Example 3: Native Integer Cells Reject

```cpp
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().balance_math_delims();
} catch (const pluss::Error&) {
  std::cout << "math Int64: rejected\n";
}
```

### Output

```text
math Int64: rejected
```

## See Also

- [`add_math_delimiters`](add_math_delimiters.md): wrap plain expressions in one outer pair.
- [`balance_braces`](balance_braces.md): repair unescaped curly-brace structure.
- [`strip_delims`](strip_delims.md): remove delimiter syntax rather than completing it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="balance_braces.html"><span class="label">Previous</span><span class="title">balance_braces</span></a>
  <a class="next-link" href="bibliography_keys.html"><span class="label">Next</span><span class="title">bibliography_keys</span></a>
</div>

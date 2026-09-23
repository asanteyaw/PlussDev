<!-- pluss-hand-authored -->
# NoteTexAccessor::line_norm

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::line_norm() const
```

## Canonicalize Line Endings Without Reformatting Source

`line_norm()` converts every CRLF pair (`\r\n`) and every remaining carriage
return (`\r`) to one line feed (`\n`). Existing line feeds remain unchanged.
It gives downstream scanners one byte-level line convention without changing
where lines begin or end.

The operation is deliberately narrower than [`fix_linebreaks`](fix_linebreaks.md).
It does not remove trailing spaces, cap blank-line runs, trim the document, or
interpret TeX commands. It is also distinct from
[`dedup_blank_lines`](dedup_blank_lines.md), which controls the number of empty
lines while preserving their existing line-ending spelling. Compose the two
when both canonical LF bytes and bounded vertical whitespace are required.

Normalization is lane-wise and shape preserving. Empty cells stay empty; a
terminal CR becomes a terminal LF; a CRLF pair becomes exactly one LF rather
than two. Applying the method again cannot change the result. The receiver is
unchanged and the returned Text Note owns a new packed arena with the same rank
and shape.

Undefined input remains undefined. Defined Bool, Int64, Float64, and Bytes
Notes are rejected: line-ending semantics belong to validated Text, not opaque
bytes. Runtime and output allocation are linear in the source byte count; no
per-line container is constructed.

## Example 1: Normalize Every Input Convention In A Matrix

```cpp
const auto source = pluss::make_note(
    std::vector<std::string>{"a\r\nb\rc\n", "stable", "", "x\r"},
    std::array<std::int64_t, 2>{2, 2});
const auto normalized = source.tex().line_norm();

std::cout << normalized << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["a\nb\nc\n", "stable"],
  ["", "x\n"]
]
```

CRLF is consumed as one unit, lone CR is replaced, existing LF is retained,
and the terminal line boundary remains terminal.

## Example 2: Verify Shape And Idempotence

```cpp
const auto twice = normalized.tex().line_norm();
std::cout << std::boolalpha
          << "same_shape=" << (twice.sizes() == source.sizes())
          << " idempotent=" << (twice == normalized).all().bool_item()
          << '\n';
```

### Output

```text
same_shape=true idempotent=true
```

## Example 3: Compose With Blank-Line Deduplication

```cpp
const auto compact =
    pluss::make_note("one\r\n \r\n\t\r\ntwo")
        .tex().line_norm().tex().dedup_blank_lines();
std::cout << compact << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"one\n\ntwo"
```

Order matters: normalization establishes LF first, then blank-line reduction
removes whitespace from the collapsed empty run.

## Example 4: Undefined And Invalid Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().line_norm().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().line_norm();
} catch (const pluss::Error&) {
  std::cout << "line norm Int64: rejected\n";
}
```

### Output

```text
false
line norm Int64: rejected
```

## See Also

- [`dedup_blank_lines`](dedup_blank_lines.md): cap empty-line runs.
- [`fix_linebreaks`](fix_linebreaks.md): normalize lines, trim horizontal ends, and cap blank runs.
- [`Note::split_lines`](../note/split_lines.md): split cells after choosing a line convention.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="limit_norm.html"><span class="label">Previous</span><span class="title">limit_norm</span></a>
  <a class="next-link" href="macro_defs.html"><span class="label">Next</span><span class="title">macro_defs</span></a>
</div>

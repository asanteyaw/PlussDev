<!-- pluss-hand-authored -->
# Note::TexAccessor::strip_tables

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::strip_tables() const
```

## Remove Complete Table Structures

`strip_tables()` removes complete environments named `table`, `table*`,
`tabular`, `tabular*`, `longtable`, or `array`. It is intended for source
cleanup where tabular structures, captions inside them, and all cell content
must disappear as one region. Use `tables()` when those structures
should be extracted instead, or [`strip_env(name)`](strip_env.md) when only one
caller-selected environment kind should be removed.

The scanner balances nested environments as a stack, including heterogeneous
nesting such as a `tabular` inside a `table`. Removing an accepted outer region
therefore consumes that whole region once; a standalone accepted inner region
is removed independently. Environment names are exact and case-sensitive.
Escaped openers and incomplete or mismatched regions remain unchanged because
silently deleting an unterminated remainder would lose authored source.

Each removed region contributes a separator, after which ASCII whitespace is
collapsed and the cell is trimmed. A cell containing only a table becomes an
empty string; it is not removed from the Note. The operation allocates a new
Text Note, preserves rank and shape, leaves the receiver unchanged, and is
idempotent. Undefined state propagates; defined non-Text input rejects.
Runtime is linear in source bytes, with stack memory proportional to maximum
environment nesting depth.

## Example 1: Nested And Standalone Table Families

```cpp
const auto source = pluss::make_note({
    R"(before \begin{table}outer \begin{tabular}x\end{tabular} tail\end{table} after)",
    R"(x \begin{array}a\end{array} y \begin{longtable}b\end{longtable} z)",
    R"(\\begin{table}escaped\\end{table} \begin{table}unfinished)"});
std::cout << source.tex().strip_tables() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["before after", "x y z", "\\\\begin{table}escaped\\\\end{table} \\begin{table}unfinished"]
```

## Example 2: Empty Results Preserve Matrix Position

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{R"(\begin{tabular}a\end{tabular})", "plain",
                             R"(x \begin{table*}b\end{table*} y)", ""},
    std::array<std::int64_t, 2>{2, 2});
std::cout << grid.tex().strip_tables() << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["", "plain"],
  ["x y", ""]
]
```

## Example 3: Idempotence, Undefined State, And Dtype

```cpp
const auto once = pluss::make_note(
    R"(a \begin{table}b\end{table} c)").tex().strip_tables();
std::cout << std::boolalpha
          << "idempotent="
          << ((once == once.tex().strip_tables()).all().bool_item())
          << " undefined="
          << pluss::Note::undefined().tex().strip_tables().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().strip_tables();
} catch (const pluss::Error&) {
  std::cout << "strip tables Int64: rejected\n";
}
```

### Output

```text
idempotent=true undefined=false
strip tables Int64: rejected
```

## See Also

- `tables`: extract complete table-family regions with lane offsets.
- `tabular_blocks`: extract complete `tabular` structures specifically.
- [`strip_env`](strip_env.md): remove one selected environment kind.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_space.html"><span class="label">Previous</span><span class="title">strip_space</span></a>
  <a class="next-link" href="sub_norm.html"><span class="label">Next</span><span class="title">sub_norm</span></a>
</div>

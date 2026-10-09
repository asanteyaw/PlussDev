<!-- pluss-hand-authored -->
# Note::TexAccessor::tables

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch Note::TexAccessor::tables() const
```

## Extract Complete `table`/`table*` Environments

`tables()` extracts every complete `\begin{table}...\end{table}` and
`\begin{table*}...\end{table*}` region, including its delimiters, as one
token per match, in source order. It is useful when a document's floating
tables must be collected, counted, or analyzed independently of the
surrounding prose.

Both the starred and unstarred forms are recognized in the same pass and
interleaved in document order — this is a genuine superset of what a single
generic `env("table")` call returns, which only matches one exact
environment name at a time. A `table` environment nested inside another
`table` environment is captured as **one** token spanning from the outer
`\begin{table}` to its matching outer `\end{table}`; the inner
`\begin{table}...\end{table}` is not separately extracted, since matching is
non-overlapping and outer-first. An escaped `\\begin{table}` is not a real
environment opener and produces no match, and an unterminated
`\begin{table}broken` with no closing tag is dropped rather than partially
returned.

The closed `TokenBatch` stores every extracted region — delimiters included
— in its rank-1 Text `tokens()` Note. Its Int64 `offsets()` has
`source.numel() + 1` entries, so flattened source cell `i` owns
`[offsets[i], offsets[i + 1])`. The receiver is not mutated.

Undefined yields undefined tokens with offsets `[0]`. Empty cells, malformed
markers, and cells without a `table` environment produce empty intervals.
Only Text is accepted.

## Example 1: Starred, Unstarred, Nested, Escaped, And Broken Forms

```cpp
const auto source = pluss::make_note({
    R"(\begin{table*}star\end{table*} x \begin{table}plain\end{table})",
    R"(\begin{table}outer \begin{table}inner\end{table} tail\end{table})",
    R"(\\begin{table}escaped\\end{table} \begin{table}broken)"});
const auto tables = source.tex().tables();

std::cout << tables.tokens() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["\\begin{table*}star\\end{table*}", "\\begin{table}plain\\end{table}", "\\begin{table}outer \\begin{table}inner\\end{table} tail\\end{table}"]
```

Cell 0 contributes both the starred and unstarred region as separate
tokens, in document order. Cell 1's nested tables collapse into the single
outer-spanning token. Cell 2 contributes nothing: the escaped pair and the
unterminated `\begin{table}broken` both fail to match.

## Example 2: Per-Cell Offsets

```cpp
std::cout << "offsets=[";
const auto offsets = tables.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 2, 3, 3]
```

Cell 0 owns two tokens, cell 1 owns the one outer-spanning token, and cell 2
owns none.

## Example 3: A 2-D Grid, Ragged Ownership Preserved

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{R"(\begin{table}A\end{table})", "plain",
                             R"(\begin{table*}B\end{table*})", ""},
    std::array<std::int64_t, 2>{2, 2});
const auto grid_batch = grid.tex().tables();
std::cout << grid_batch.tokens() << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["\\begin{table}A\\end{table}", "\\begin{table*}B\\end{table*}"]
```

## Example 4: Undefined And Non-Text Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().tables().tokens().defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().tables();
} catch (const pluss::Error&) {
  std::cout << "tables Int64: rejected\n";
}
```

### Output

```text
false
tables Int64: rejected
```

## See Also

- [`tabular_blocks`](tabular_blocks.md): the nested tabular content a `table`
  environment usually wraps.
- [`figures`](figures.md): the sibling floating-object environment.
- [`env("table")`](env.md): a single exact form, without the starred variant.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sup_norm.html"><span class="label">Previous</span><span class="title">sup_norm</span></a>
  <a class="next-link" href="tabular_blocks.html"><span class="label">Next</span><span class="title">tabular_blocks</span></a>
</div>

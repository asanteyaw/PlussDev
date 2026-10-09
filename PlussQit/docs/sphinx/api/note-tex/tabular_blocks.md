<!-- pluss-hand-authored -->
# Note::TexAccessor::tabular_blocks

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch Note::TexAccessor::tabular_blocks() const
```

## Extract Complete `tabular`/`tabular*` Environments

`tabular_blocks()` extracts every complete `\begin{tabular}...\end{tabular}`
and `\begin{tabular*}...\end{tabular*}` region, including its delimiters,
as one token per match, in source order. It is the `tabular`-family
counterpart of [`tables`](tables.md): a `table` environment is the floating
wrapper, while `tabular` is the actual row/column content, and the two
often appear nested — a `tabular` block inside a `table` environment is
still extracted on its own, independent of the surrounding `table`.

Both the starred and unstarred forms are recognized in the same pass and
interleaved in document order. A similarly named but distinct environment
— `array`, in particular — is not recognized; only the two exact names
above match. An unterminated `\begin{tabular}broken` with no closing tag is
dropped rather than partially returned.

The closed `TokenBatch` stores every extracted region — delimiters included
— in its rank-1 Text `tokens()` Note. Its Int64 `offsets()` has
`source.numel() + 1` entries, so flattened source cell `i` owns
`[offsets[i], offsets[i + 1])`. The receiver is not mutated.

Undefined yields undefined tokens with offsets `[0]`. Empty cells and cells
without a `tabular` environment produce empty intervals. Only Text is
accepted.

## Example 1: Starred, Unstarred, Nested-In-`table`, And A Look-Alike Environment

```cpp
const auto source = pluss::make_note({
    R"(\begin{tabular*}wide\end{tabular*} x \begin{tabular}plain\end{tabular})",
    R"(\begin{table}\begin{tabular}inside\end{tabular}\end{table})",
    R"(\begin{array}not tabular\end{array} \begin{tabular}broken)"});
const auto blocks = source.tex().tabular_blocks();

std::cout << blocks.tokens() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["\\begin{tabular*}wide\\end{tabular*}", "\\begin{tabular}plain\\end{tabular}", "\\begin{tabular}inside\\end{tabular}"]
```

Cell 0 contributes both the starred and unstarred region. Cell 1's
`tabular` block is extracted on its own, independent of the surrounding
`table` wrapper — no `\begin{table}` markers appear in the token. Cell 2
contributes nothing: `array` is a different environment name, and
`\begin{tabular}broken` has no closing tag.

## Example 2: Per-Cell Offsets

```cpp
std::cout << "offsets=[";
const auto offsets = blocks.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 2, 3, 3]
```

Cell 0 owns two tokens, cell 1 owns its one nested block, and cell 2 owns
none.

## Example 3: No Tabular Content Anywhere

```cpp
const auto empty_batch = pluss::make_note({"plain", ""}).tex().tabular_blocks();
std::cout << empty_batch.tokens() << '\n';
```

### Output

```text
Note(shape=[0], dim=1, numel=0, encoding=utf8)
[]
```

## Example 4: Undefined And Non-Text Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().tabular_blocks().tokens().defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().tabular_blocks();
} catch (const pluss::Error&) {
  std::cout << "tabular blocks Int64: rejected\n";
}
```

### Output

```text
false
tabular blocks Int64: rejected
```

## See Also

- [`tables`](tables.md): the floating environment `tabular` content
  commonly appears inside.
- [`env("tabular")`](env.md): a single exact form, without the starred
  variant.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tables.html"><span class="label">Previous</span><span class="title">tables</span></a>
  <a class="next-link" href="text_only.html"><span class="label">Next</span><span class="title">text_only</span></a>
</div>

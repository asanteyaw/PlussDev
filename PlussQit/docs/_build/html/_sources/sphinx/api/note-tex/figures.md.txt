<!-- pluss-hand-authored -->
# Note::TexAccessor::figures

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch Note::TexAccessor::figures() const
```

## Extract Complete Figure Regions

`figures()` extracts complete `figure` and `figure*` environments from every
Text cell. Each token contains the entire region, from `\begin{...}` through
the matching `\end{...}`. Retaining the delimiters matters when a caller wants
to relocate, inspect, or separately transform a float: the environment kind
and its body remain one unit.

The two environment spellings are returned in source order. `figure*` is not
treated as an ordinary `figure`, and an escaped opener such as
`\\begin{figure}` does not start a region. A same-name environment nested in
another is balanced as part of the outer token rather than terminating it at
the first closer. An opener without a matching closer contributes no token;
the method never fabricates a missing `\end` command.

This is structural extraction, not graphics discovery. It does not read image
files, render TeX, or require a `\caption` or `\includegraphics` command in the
body. Use `includes()` when only external paths are needed, `captions()` when
only caption text is needed, or [`strip_figs()`](strip_figs.md) when the regions should be
removed from otherwise retained source.

The result is ragged. `tokens()` is one flat Text Note; `offsets()` is an Int64
Note with one boundary per receiver cell plus a final boundary. Cell `i` owns
the half-open range `[offsets[i], offsets[i + 1])`. The receiver's rank is
therefore represented by lane order and offsets, not copied onto the token
Note. Undefined input returns undefined tokens and canonical offsets `[0]`;
any defined non-Text dtype is rejected.

Scanning is linear in the source bytes plus the bytes copied into extracted
regions. Nested same-name figures require only a depth counter; no document
tree is built.

## Example 1: Preserve Variant And Document Order

```cpp
const auto source = pluss::make_note({
    R"(\begin{figure*}wide\end{figure*} then \begin{figure}plain\end{figure})",
    R"(\begin{figure}outer \begin{figure}inner\end{figure} tail\end{figure})",
    R"(\begin{figure}unfinished)",
    R"(\\begin{figure}escaped\end{figure})"});
const auto blocks = source.tex().figures();

std::cout << blocks.tokens() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["\\begin{figure*}wide\\end{figure*}", "\\begin{figure}plain\\end{figure}", "\\begin{figure}outer \\begin{figure}inner\\end{figure} tail\\end{figure}"]
```

The starred block remains first. The nested ordinary figure belongs to one
balanced outer token. Neither the unmatched opener nor the escaped opener
produces a token.

## Example 2: Recover Cell Ownership

```cpp
std::cout << "offsets=[";
const auto offsets = blocks.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 2, 3, 3, 3]
```

The first cell owns two blocks, the second owns one, and the final two cells
own empty ranges.

## Example 3: Undefined And Invalid Input

```cpp
const auto absent = pluss::Note::undefined().tex().figures();
std::cout << std::boolalpha << absent.tokens().defined() << " offsets=["
          << absent.offsets().integers()[0] << "]\n";

try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().figures();
} catch (const pluss::Error&) {
  std::cout << "figures Int64: rejected\n";
}
```

### Output

```text
false offsets=[0]
figures Int64: rejected
```

## See Also

- [`captions`](captions.md): extract caption bodies without their float regions.
- [`includes`](includes.md): extract external input and graphics paths.
- [`strip_figs`](strip_figs.md): remove complete figure regions from each cell.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="escape.html"><span class="label">Previous</span><span class="title">escape</span></a>
  <a class="next-link" href="fix_linebreaks.html"><span class="label">Next</span><span class="title">fix_linebreaks</span></a>
</div>

<!-- pluss-hand-authored -->
# Note::TexAccessor::includes

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch Note::TexAccessor::includes() const
```

## Extract External Resource Targets

`includes()` extracts required path arguments from the three source commands
that bring external material into a document: `\input`, `\include`, and
`\includegraphics`. It returns the argument body, not the command spelling or
outer braces. Calls from all three families remain interleaved in source order.

An optional bracket group is accepted and skipped before the required path.
This is most useful for graphics options such as width, scale, or page. Both
the optional group and path group must balance, and nested braces in the path
are retained. Exact control-word matching prevents `\inputted` from being
misread as `\input`; an escaped `\\input` is likewise ignored. An incomplete
call contributes no partial path.

This method discovers authored targets only. It performs no filesystem access,
path normalization, extension inference, recursive inclusion, or graphics
rendering. That makes its result deterministic and safe for inventorying
untrusted source. Use `input_tags()` when input commands should be replaced by
visible markers in otherwise retained text, and `graphics_tags()` when
graphics directives should become visible markers.

Because each cell can contain a different number of targets, `includes()`
returns a closed `TokenBatch`. `tokens()` is a flat Text Note and `offsets()`
maps each original cell to a half-open token interval. Empty cells and cells
without include commands contribute repeated boundaries. Undefined input
returns undefined tokens with offsets `[0]`; a defined non-Text Note is
rejected.

The scanner is linear in source bytes and copies only extracted path bodies.
It does not allocate a syntax tree or touch external storage.

## Example 1: Extract All Supported Command Families

```cpp
const auto source = pluss::make_note({
    R"(\input{chapters/{intro}} \includegraphics[width={.5\textwidth}]{plot.pdf})",
    R"(\include{appendix} \inputted{no} \\input{escaped})",
    R"(\input{unfinished)"});
const auto paths = source.tex().includes();

std::cout << paths.tokens() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["chapters/{intro}", "plot.pdf", "appendix"]
```

The nested path braces are content and survive. Graphics options are not part
of the path. The prefix collision, escaped command, and malformed group do not
produce tokens.

## Example 2: Recover Cell Ownership

```cpp
std::cout << "offsets=[";
const auto offsets = paths.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 2, 3, 3]
```

Cell zero owns two paths, cell one owns one, and cell two owns an empty range.

## Example 3: Inventory Without Performing I/O

```cpp
for (std::int64_t i = 0; i < paths.tokens().numel(); ++i)
  std::cout << "declared: " << paths.tokens().at(i) << '\n';
```

### Output

```text
declared: chapters/{intro}
declared: plot.pdf
declared: appendix
```

No file is opened by this loop or by `includes()`; the strings are declarations
found in the Note.

## Example 4: Undefined And Invalid Input

```cpp
const auto absent = pluss::Note::undefined().tex().includes();
std::cout << std::boolalpha << absent.tokens().defined() << " offsets=["
          << absent.offsets().integers()[0] << "]\n";
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().includes();
} catch (const pluss::Error&) {
  std::cout << "includes Int64: rejected\n";
}
```

### Output

```text
false offsets=[0]
includes Int64: rejected
```

## See Also

- [`input_tags`](input_tags.md): replace input commands with visible path tags.
- [`figures`](figures.md): extract complete figure and figure-star regions.
- `graphics_tags`: replace graphics commands with visible markers.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ids.html"><span class="label">Previous</span><span class="title">ids</span></a>
  <a class="next-link" href="input_tags.html"><span class="label">Next</span><span class="title">input_tags</span></a>
</div>

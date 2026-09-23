<!-- pluss-hand-authored -->
# NoteTexAccessor::sections

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteTexAccessor::sections() const
```

## Extract Top-Level Section Titles

`sections()` extracts the required title body from every complete `\section`
or `\section*` call. It accepts an optional short title before the required
body, but returns the authored long title because that is the content attached
to the document section. Nested braces inside the title are retained.

This method targets exactly one structural depth. It does not include
`\subsection`, `\subsubsection`, `\chapter`, or longer command names such as
`\sectional`. Use [`headings(max_depth)`](headings.md) for a combined outline
through several depths. Escaped commands and incomplete optional or required
groups produce no token; source text is never repaired or modified.

The result is a closed `TokenBatch`. `tokens()` is a flat Text Note in source
order. `offsets()` is an Int64 Note of length `receiver.numel() + 1`; the titles
for flattened cell `i` occupy
`tokens()[offsets[i] ... offsets[i + 1])`. Thus a matrix receiver remains
recoverable lane by lane even though the number of titles differs per cell.
The token Note itself is always rank one.

No matches produce a defined empty Text token Note and zero-valued offsets.
An undefined receiver produces undefined tokens and the canonical offsets
`[0]`. Defined non-Text input rejects. Extraction is non-mutating, scans every
cell once, and allocates only the flattened title bytes and cumulative offsets.

## Example 1: Stars, Optional Titles, Nested Braces, And Boundaries

```cpp
const auto source = pluss::make_note({
    R"(\section[Short]{A {nested} title} \section*{Second})",
    R"(\\section{escaped} \sectional{long} \section{unfinished)",
    "plain"});
const auto batch = source.tex().sections();
std::cout << batch.tokens() << '\n';
const auto offsets = batch.offsets().integers();
std::cout << "offsets=[";
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["A {nested} title", "Second"]
offsets=[0, 2, 2, 2]
```

Both titles belong to the first input cell. The remaining cells contribute
equal adjacent offsets and therefore empty token slices.

## Example 2: Recovering Ownership From A Matrix

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{R"(\section{A})", "plain",
                             R"(\section{B}\section{C})", ""},
    std::array<std::int64_t, 2>{2, 2});
const auto batch = grid.tex().sections();
std::cout << batch.tokens() << '\n';
const auto offsets = batch.offsets().integers();
std::cout << "offsets=[";
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["A", "B", "C"]
offsets=[0, 1, 1, 3, 3]
```

Flattened matrix cell `2` owns token positions `[1, 3)`, namely `B` and `C`.

## Example 3: A Defined Input With No Sections

```cpp
const auto batch = pluss::make_note({"plain", ""}).tex().sections();
std::cout << batch.tokens() << '\n';
const auto offsets = batch.offsets().integers();
std::cout << "offsets=[";
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[0], dim=1, numel=0, encoding=utf8)
[]
offsets=[0, 0, 0]
```

## Example 4: Undefined State And Dtype

```cpp
const auto absent = pluss::Note::undefined().tex().sections();
std::cout << std::boolalpha
          << "tokens defined=" << absent.tokens().defined()
          << " offsets=" << absent.offsets().int_item() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().sections();
} catch (const pluss::Error&) {
  std::cout << "sections Int64: rejected\n";
}
```

### Output

```text
tokens defined=false offsets=0
sections Int64: rejected
```

## See Also

- [`headings`](headings.md): extract a combined outline through a chosen depth.
- `subsections`: extract only second-level titles.
- `subsubsections`: extract only third-level titles.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="render.html"><span class="label">Previous</span><span class="title">render</span></a>
  <a class="next-link" href="set_macro.html"><span class="label">Next</span><span class="title">set_macro</span></a>
</div>

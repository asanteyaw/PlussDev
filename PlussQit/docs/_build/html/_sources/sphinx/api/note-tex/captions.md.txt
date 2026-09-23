<!-- pluss-hand-authored -->
# NoteTexAccessor::captions

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteTexAccessor::captions() const
```

## Extract Displayed Caption Bodies Per Source Cell

`captions()` extracts the required brace body from each complete `\caption`
or `\caption*` command. It is useful when figure and table descriptions must
be indexed or analyzed independently of the surrounding environment.

An optional square-bracket argument is accepted but not returned: in
`\caption[Short]{Displayed title}`, the token is `Displayed title`. This
reflects the distinction between the abbreviated list entry and the caption
body presented with the object. Starred commands are recognized in the same
source order as unstarred commands, and repeated captions remain repeated.

The extractor is deliberately lexical. The required body must be a complete,
non-nested brace group. A body containing another brace group, such as
`\caption{A \textbf{bold} title}`, is not partially returned. Run a suitable
normalization first when nested formatting must be flattened, or use a
parser when arbitrary TeX nesting must be understood. Comments are ordinary
source to this method; call `strip_comments()` first if commented-out commands
must be excluded.

The closed `TokenBatch` stores all caption bodies in its rank-1 Text
`tokens()` Note. Its Int64 `offsets()` has `source.numel() + 1` entries, so
flattened source cell `i` owns `[offsets[i], offsets[i + 1])`. The original
rank and shape determine lane order but are not imposed on a ragged token
count. The receiver is not mutated.

Undefined yields undefined tokens with offsets `[0]`. Empty cells, malformed
commands, and cells without captions produce empty intervals. Only Text is
accepted. Extraction scans source bytes and allocates the captured bodies and
one offset per source cell.

## Example 1: Long, Starred, Optional, Missing, And Nested Captions

```cpp
const auto source = pluss::make_note(
    std::vector<std::string>{
        R"(\caption{Long title} \caption*[Short]{Starred long})",
        "no caption", R"(\caption[TOC title]{Displayed title})",
        R"(\caption{Nested \textbf{title}})"},
    std::array<std::int64_t, 2>{2, 2});
const auto captions = source.tex().captions();

std::cout << captions.tokens() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Long title", "Starred long", "Displayed title"]
```

The optional `Short` and `TOC title` values are not caption bodies. The nested
fourth command is ignored rather than truncated.

## Example 2: Matrix Cells Retain Ragged Ownership

```cpp
std::cout << "offsets=[";
const auto offsets = captions.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 2, 2, 3, 3]
```

Flattened matrix cell 0 owns two captions, cell 1 none, cell 2 one, and cell
3 none.

## Example 3: Native Integer Cells Reject

```cpp
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().captions();
} catch (const pluss::Error&) {
  std::cout << "captions Int64: rejected\n";
}
```

### Output

```text
captions Int64: rejected
```

## See Also

- `figures` and `tables`: extract complete floating environments.
- [`args("caption")`](args.md): select arguments through the generic command interface.
- [`TokenBatch`](../../tokenbatch_api_reference.md): slice ragged results by source lane.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="canonicalize_macros.html"><span class="label">Previous</span><span class="title">canonicalize_macros</span></a>
  <a class="next-link" href="citations.html"><span class="label">Next</span><span class="title">citations</span></a>
</div>

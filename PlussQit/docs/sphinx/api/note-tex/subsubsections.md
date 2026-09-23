<!-- pluss-hand-authored -->
# NoteTexAccessor::subsubsections

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteTexAccessor::subsubsections() const
```

## Extract Subsubsection Titles Per Source Cell

`subsubsections()` extracts the required brace body from each complete
`\subsubsection` or `\subsubsection*` command — the third heading level,
one below [`subsections`](subsections.md) and two below
[`sections`](sections.md). It shares every rule documented on
`subsections`: the optional square-bracket short-title argument is
discarded, a nested brace group inside the required body is kept whole, the
match is command-exact (a shallower `\subsection{...}` in the same source is
not a `\subsubsection` and is skipped), and an escaped or unterminated call
produces no token.

The closed `TokenBatch` stores every extracted title in its rank-1 Text
`tokens()` Note. Its Int64 `offsets()` has `source.numel() + 1` entries, so
flattened source cell `i` owns `[offsets[i], offsets[i + 1])`. The original
rank and shape determine lane order but are not imposed on a ragged token
count. The receiver is not mutated.

Undefined yields undefined tokens with offsets `[0]`. Empty cells, malformed
commands, and cells without subsubsections produce empty intervals. Only
Text is accepted.

## Example 1: Optional Argument, Nested Body, And A Shallower Heading

```cpp
const auto source = pluss::make_note({
    R"(\subsubsection[Short]{Deep {title}} \subsubsection*{Second})",
    R"(\subsection{shallower} \subsubsection{Third})", "plain"});
const auto deep = source.tex().subsubsections();

std::cout << deep.tokens() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Deep {title}", "Second", "Third"]
```

The `Short` value is discarded, `{title}` is kept whole, and the shallower
`\subsection{shallower}` in cell 1 is not a match — only its trailing
`\subsubsection{Third}` contributes a token.

## Example 2: Per-Cell Offsets

```cpp
std::cout << "offsets=[";
const auto offsets = deep.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 2, 3, 3]
```

Cell 0 owns two titles, cell 1 owns the one trailing title, and the plain
cell 2 owns none.

## Example 3: No Subsubsections In Either Cell

```cpp
const auto empty_batch = pluss::make_note({"plain", ""}).tex().subsubsections();
std::cout << empty_batch.tokens() << '\n';
```

### Output

```text
Note(shape=[0], dim=1, numel=0, encoding=utf8)
[]
```

A source with no matching command anywhere produces a defined, empty flat
token Note — not an undefined one; compare Example 4 below.

## Example 4: Undefined And Non-Text Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().subsubsections().tokens().defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().subsubsections();
} catch (const pluss::Error&) {
  std::cout << "subsubsections Int64: rejected\n";
}
```

### Output

```text
false
subsubsections Int64: rejected
```

## See Also

- [`subsections`](subsections.md): the heading level above.
- [`sections`](sections.md): the top heading level.
- [`headings`](headings.md): every heading level in one call, depth-bounded.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="subsections.html"><span class="label">Previous</span><span class="title">subsections</span></a>
  <a class="next-link" href="sum_norm.html"><span class="label">Next</span><span class="title">sum_norm</span></a>
</div>

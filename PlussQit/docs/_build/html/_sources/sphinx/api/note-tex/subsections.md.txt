<!-- pluss-hand-authored -->
# NoteTexAccessor::subsections

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteTexAccessor::subsections() const
```

## Extract Subsection Titles Per Source Cell

`subsections()` extracts the required brace body from each complete
`\subsection` or `\subsection*` command. It is useful when a document's
subsection structure must be listed or indexed independently of the body
text between headings.

An optional square-bracket short-title argument is accepted but not
returned: in `\subsection[Short]{Nested {title}}`, the token is the full
brace body, `Nested {title}`, not the abbreviated `Short` form — unlike
[`captions`](captions.md), whose required body must be non-nested, a
subsection title's brace body may itself contain a balanced nested group,
and that nested group is retained verbatim in the token.

The extractor is lexical and command-exact: `\subsection` and `\subsection*`
are matched, but the longer, unrelated command name `\subsectional{...}` in
the same source is not (it is not a `\subsection` invocation at all — the
match requires the control word to end exactly at `subsection` or
`subsection*`), and an incomplete call such as `\subsection{broken` with no
closing brace produces no token. A deeper heading, `\subsubsection{...}`, is
a different command and is not matched — see
[`subsubsections`](subsubsections.md) for that level.

The closed `TokenBatch` stores every extracted title in its rank-1 Text
`tokens()` Note. Its Int64 `offsets()` has `source.numel() + 1` entries, so
flattened source cell `i` owns `[offsets[i], offsets[i + 1])`. The original
rank and shape determine lane order but are not imposed on a ragged token
count. The receiver is not mutated.

Undefined yields undefined tokens with offsets `[0]`. Empty cells, malformed
commands, and cells without subsections produce empty intervals. Only Text
is accepted. Extraction scans source bytes and allocates the captured titles
and one offset per source cell.

## Example 1: Optional Argument, Nested Body, Escaped, Unrelated, And Broken Calls

```cpp
const auto source = pluss::make_note({
    R"(\subsection[Short]{Nested {title}} \subsection*{Second})",
    R"(\\subsection{escaped} \subsectional{long} \subsection{broken)",
    R"(\subsubsection{deeper} \subsection{Third})"});
const auto subsections = source.tex().subsections();

std::cout << subsections.tokens() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Nested {title}", "Second", "Third"]
```

The optional `Short` value is discarded and the nested `{title}` group is
kept whole. The second cell contributes nothing: the escaped `\\subsection`,
the unrelated `\subsectional`, and the unterminated `\subsection{broken` all
fail to match. The third cell's `\subsubsection{deeper}` is a different
command and is skipped; only its trailing `\subsection{Third}` matches.

## Example 2: Per-Cell Offsets

```cpp
std::cout << "offsets=[";
const auto offsets = subsections.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 2, 2, 3]
```

Cell 0 owns two titles, cell 1 owns none, and cell 2 owns exactly the one
title that matched.

## Example 3: A 2-D Grid, Ragged Ownership Preserved

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{R"(\subsection{A})", "plain",
                             R"(\subsection{B}\subsection{C})", ""},
    std::array<std::int64_t, 2>{2, 2});
const auto grid_batch = grid.tex().subsections();
std::cout << grid_batch.tokens() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["A", "B", "C"]
```

## Example 4: Undefined And Non-Text Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().subsections().tokens().defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().subsections();
} catch (const pluss::Error&) {
  std::cout << "subsections Int64: rejected\n";
}
```

### Output

```text
false
subsections Int64: rejected
```

## See Also

- [`subsubsections`](subsubsections.md): the next heading level down.
- [`sections`](sections.md): the heading level above.
- [`headings`](headings.md): every heading level in one call, depth-bounded.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sub_norm.html"><span class="label">Previous</span><span class="title">sub_norm</span></a>
  <a class="next-link" href="subsubsections.html"><span class="label">Next</span><span class="title">subsubsections</span></a>
</div>

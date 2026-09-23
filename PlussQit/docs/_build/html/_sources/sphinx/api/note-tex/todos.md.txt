<!-- pluss-hand-authored -->
# NoteTexAccessor::todos

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteTexAccessor::todos() const
```

## Extract `\todo` Note Bodies Per Source Cell

`todos()` extracts the required brace body from each `\todo{...}` command —
a common convention (from packages such as `todonotes`) for leaving
in-source review notes. It is useful for collecting outstanding
authoring notes across a document independently of the surrounding text.

Unlike [`captions`](captions.md), the match here is a plain regular
expression over a single, non-nested brace group — there is no starred
form and no optional leading argument to skip. A body containing a nested
brace group is not specially handled; the extractor's group stops at the
first unescaped `}`, the same boundary behavior as
[`footnotes`](footnotes.md).

The closed `TokenBatch` stores every extracted note body in its rank-1
Text `tokens()` Note. Its Int64 `offsets()` has `source.numel() + 1`
entries, so flattened source cell `i` owns `[offsets[i], offsets[i + 1])`.
The receiver is not mutated.

Undefined yields undefined tokens with offsets `[0]`. Empty cells and cells
without a `\todo` command produce empty intervals. Only Text is accepted.

## Example 1: Multiple Notes In One Cell, And A Lone Note

```cpp
const auto source = pluss::make_note({
    R"(\todo{fix this} text \todo{and this too})",
    "no todos here",
    R"(\todo{lone})"});
const auto todos = source.tex().todos();

std::cout << todos.tokens() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["fix this", "and this too", "lone"]
```

## Example 2: Per-Cell Offsets

```cpp
std::cout << "offsets=[";
const auto offsets = todos.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 2, 2, 3]
```

Cell 0 owns two notes, the plain-prose cell 1 owns none, and cell 2 owns
its one note.

## Example 3: No `\todo` Commands Anywhere

```cpp
const auto empty_batch = pluss::make_note({"plain", ""}).tex().todos();
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
          << pluss::Note::undefined().tex().todos().tokens().defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().todos();
} catch (const pluss::Error&) {
  std::cout << "todos Int64: rejected\n";
}
```

### Output

```text
false
todos Int64: rejected
```

## See Also

- [`warnings`](warnings.md): extract compiler/package diagnostics instead
  of authoring notes.
- [`footnotes`](footnotes.md): the same single-brace-group extraction
  pattern, for `\footnote{...}`.
- [`captions`](captions.md): a semantic extractor with an optional leading
  argument to skip.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_text.html"><span class="label">Previous</span><span class="title">to_text</span></a>
  <a class="next-link" href="unescape.html"><span class="label">Next</span><span class="title">unescape</span></a>
</div>

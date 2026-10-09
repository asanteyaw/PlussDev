<!-- pluss-hand-authored -->
# Note::TexAccessor::citations

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch Note::TexAccessor::citations() const
```

## Extract One Argument Per Citation Command

`citations()` finds `\cite`-family commands and returns each command's required
brace argument as one token. The family includes letter-suffixed and starred
forms such as `\citep`, `\citet`, and `\citet*`. Any number of optional
square-bracket notes between the command and required argument are accepted
and omitted from the result.

The unit of extraction is the *citation command*, not the individual key. A
source command `\cite{a,b}` therefore yields the single token `a,b`. This is
useful when command grouping and author-supplied key order matter. Choose
[`bibliography_keys()`](bibliography_keys.md) when each comma-separated key
should become an independent token and `\bibitem` declarations should also
participate.

Matches preserve source order and occurrence count. A malformed command, a
missing required group, or a required group containing nested braces is
ignored. The method does not expand macros, resolve keys against a database,
or suppress commands inside comments; compose with `strip_comments()` first
when comments are outside the desired source domain.

The return value follows the closed `TokenBatch` contract: `tokens()` is the
flat rank-1 Text payload, while Int64 `offsets()` maps every flattened source
cell to its half-open token interval. Cells without citations repeat the
previous offset. Undefined yields undefined tokens and offsets `[0]`.
Non-Text Notes reject. Extraction does not mutate the receiver and costs a
source scan plus storage for matched arguments and offsets.

## Example 1: Grouped Keys, Two Optional Notes, And A Starred Form

```cpp
const auto source = pluss::make_note({
    R"(\cite{a,b} \citep[see][p. 4]{c,d})", R"(\citet*{e})",
    "no citation"});
const auto citations = source.tex().citations();

std::cout << citations.tokens() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["a,b", "c,d", "e"]
```

## Example 2: Offsets Preserve Command Ownership

```cpp
std::cout << "offsets=[";
const auto offsets = citations.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 2, 3, 3]
```

## Example 3: Native Integer Cells Reject

```cpp
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().citations();
} catch (const pluss::Error&) {
  std::cout << "citations Int64: rejected\n";
}
```

### Output

```text
citations Int64: rejected
```

## See Also

- [`bibliography_keys`](bibliography_keys.md): split key lists and include `\bibitem` identifiers.
- `strip_cites`: remove citation constructs from source.
- `refs`: extract non-bibliographic cross-reference targets.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="captions.html"><span class="label">Previous</span><span class="title">captions</span></a>
  <a class="next-link" href="cmd_sub.html"><span class="label">Next</span><span class="title">cmd_sub</span></a>
</div>

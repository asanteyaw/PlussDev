<!-- pluss-hand-authored -->
# NoteTexAccessor::labels

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteTexAccessor::labels() const
```

## Extract Authored Cross-Reference Anchors

`labels()` extracts the required argument of every complete `\label{...}` call
from each Text cell. Tokens contain the argument body, not the command or outer
braces. This is the direct inventory operation for discovering the anchors a
document defines before checking references, renaming keys, or constructing a
document-level index.

Matching is exact and boundary aware. `\labeled{...}` is a different control
word and does not match; a doubled backslash marks an escaped command boundary
and does not begin a label call. Horizontal whitespace between `\label` and
its required argument is accepted. The argument scanner balances nested braces,
so `\label{eq:{energy}}` contributes `eq:{energy}` rather than stopping at the
inner closing brace. An incomplete argument contributes nothing.

This is intentionally lexical. It does not validate whether a key is unique,
whether a matching `\ref` exists, or whether TeX would execute the command in
the surrounding context. Duplicate label bodies remain duplicate tokens and
source order is stable. Use `refs()` for reference-use extraction and compare
the two inventories at the document layer when unresolved or duplicate anchors
matter.

The returned `TokenBatch` owns one flat Text Note plus Int64 offsets. The
offset range `[offsets[i], offsets[i + 1])` identifies tokens from receiver
cell `i`, including cells with none. Receiver rank is represented by row-major
lane order rather than imposed on the ragged token Note. Undefined input
returns undefined tokens with offsets `[0]`; defined non-Text input is rejected.

Runtime is linear in source bytes plus copied label-body bytes. No TeX engine,
filesystem access, or document tree is involved.

## Example 1: Exact Commands And Balanced Bodies

```cpp
const auto source = pluss::make_note({
    R"(\label{eq:{energy}} \labeled{no})",
    R"(\\label{escaped} \label {sec:intro})", R"(\label{unfinished)"});
const auto result = source.tex().labels();

std::cout << result.tokens() << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["eq:{energy}", "sec:intro"]
```

The nested braces remain part of the first key. The longer command name,
escaped command, and unfinished body produce no false tokens.

## Example 2: Preserve Per-Cell Ownership

```cpp
std::cout << "offsets=[";
const auto offsets = result.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 1, 2, 2]
```

The first two cells each own one key; the malformed third cell owns an empty
range.

## Example 3: Undefined And Invalid Input

```cpp
const auto absent = pluss::Note::undefined().tex().labels();
std::cout << std::boolalpha << absent.tokens().defined() << " offsets=["
          << absent.offsets().integers()[0] << "]\n";
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().labels();
} catch (const pluss::Error&) {
  std::cout << "labels Int64: rejected\n";
}
```

### Output

```text
false offsets=[0]
labels Int64: rejected
```

## See Also

- `refs`: extract the keys consumed by cross-reference commands.
- [`args`](args.md): extract required arguments from a caller-selected command.
- [`command_names`](command_names.md): inventory all lexical control words.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="int_norm.html"><span class="label">Previous</span><span class="title">int_norm</span></a>
  <a class="next-link" href="limit_norm.html"><span class="label">Next</span><span class="title">limit_norm</span></a>
</div>

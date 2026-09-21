<!-- pluss-hand-authored -->
# NoteTexAccessor::bibliography_keys

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteTexAccessor::bibliography_keys() const
```

## Extract Citation And Bibliography Identifiers By Source Cell

`bibliography_keys()` extracts the identifiers carried by citation commands
and `\bibitem` declarations. It is the right operation when the keys
themselves matter: building a citation index, finding unresolved references,
or comparing the references used by independently stored source cells. It
does not return rendered citation text or bibliography entries.

The recognizer accepts `\cite` and letter-or-star suffixed citation commands
such as `\citep`, `\citet`, and starred forms, with optional square-bracket
notes before the required brace group. It also accepts `\bibitem`, including
its optional display label. A brace group containing `a, b, c` yields three
keys: comma-separated fields are split, surrounding ASCII whitespace is
removed, and empty fields are discarded.

Keys retain document order and duplicates retain occurrence semantics. If a
key is cited twice, both occurrences are emitted; call a later uniqueness
operation only when multiplicity is irrelevant to the task. Commands with a
missing closing brace or nested braces in the key field are not matches. This
is a source extractor, not a TeX expansion engine or a BibTeX parser: it does
not expand macros, follow included files, or read bibliography databases.

The result is a closed [`TokenBatch`](../../tokenbatch_api_reference.md).
`tokens()` is one flat rank-1 Text Note. `offsets()` is a rank-1 Int64 Note of
length `source.numel() + 1`; source cell `i` owns
`tokens[offsets[i]:offsets[i + 1]]`. Empty source cells and cells without a
recognized command contribute no tokens and therefore repeat the preceding
offset. This representation preserves cell ownership without padding ragged
results.

Extraction does not mutate the receiver. It scans Text cells in flattened
row-major order and allocates keys plus offsets. Undefined produces an
undefined token Note and the valid sentinel offsets Note `[0]`. Bytes and
native scalar dtypes reject before extraction.

## Example 1: Lists, Optional Arguments, Duplicates, And `bibitem`

```cpp
const auto sources = pluss::make_note({
    R"(See \cite{knuth84, lamport94}; \bibitem{turing36}; \citep[sec. 2]{knuth84}.)",
    "no references",
    R"(\bibitem[Church]{church36})"});
const auto keys = sources.tex().bibliography_keys();

std::cout << keys.tokens() << '\n';
```

### Output

```text
Note(shape=[5], dim=1, numel=5, encoding=utf8)
["knuth84", "lamport94", "turing36", "knuth84", "church36"]
```

The repeated `knuth84` is intentional: the output describes occurrences, not
a set.

## Example 2: Offsets Recover The Ragged Rows

```cpp
std::cout << "offsets=[";
const auto offsets = keys.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 4, 4, 5]
```

The first source cell owns tokens `[0, 4)`, the second owns the empty interval
`[4, 4)`, and the third owns `[4, 5)`.

## Example 3: Undefined Has A Closed Empty Offset Domain

```cpp
const auto absent = pluss::Note::undefined().tex().bibliography_keys();
std::cout << std::boolalpha << "undefined tokens="
          << absent.tokens().defined() << " offsets=["
          << absent.offsets().integers()[0] << "]\n";
```

### Output

```text
undefined tokens=false offsets=[0]
```

## Example 4: Native Integer Cells Reject

```cpp
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().bibliography_keys();
} catch (const pluss::Error&) {
  std::cout << "bibliography Int64: rejected\n";
}
```

### Output

```text
bibliography Int64: rejected
```

## See Also

- `citations`: extract complete citation constructs rather than only keys.
- `refs`: extract cross-reference targets from `\ref`-family commands.
- [`TokenBatch`](../../tokenbatch_api_reference.md): token ownership and offset slicing.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="balance_math_delims.html"><span class="label">Previous</span><span class="title">balance_math_delims</span></a>
  <a class="next-link" href="canonicalize_macros.html"><span class="label">Next</span><span class="title">canonicalize_macros</span></a>
</div>

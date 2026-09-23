<!-- pluss-hand-authored -->
# NoteTexAccessor::strip_refs

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::strip_refs() const
```

## Remove Cross-Reference Calls

`strip_refs()` removes complete calls from the nine reference families
`\ref`, `\eqref`, `\autoref`, `\pageref`, `\nameref`, `\cref`, `\Cref`,
`\vref`, and `\Vref`, including starred spellings. Keys are discarded with
the calls because reference commands contribute generated text rather than
literal authored prose. Use [`refs()`](refs.md) to extract those keys, and
[`strip_links()`](strip_links.md) for links that have visible labels worth
retaining.

Required key groups are brace balanced, and matching is exact and
case-sensitive. Longer names such as `\reference` are untouched. Escaped
commands and calls without a complete key group remain unchanged. The method
does not resolve labels or replace a reference with its number; it only removes
the source call.

Deleted calls leave normalized spacing. The operation allocates a new Text
Note, preserves rank and shape, leaves the receiver unchanged, and is
idempotent. Undefined state propagates; defined non-Text dtypes reject. Work is
linear in source bytes.

## Example 1: All Families, Stars, Nested Keys, And Boundaries

```cpp
const auto source = pluss::make_note({
    R"(A \ref{eq:{one}} B \eqref*{eq:two} C \cref{sec:a} D)",
    R"(\Cref{x} \vref{y} \Vref{z} \autoref{a} \pageref{p} \nameref{n})",
    R"(\\ref{escaped} \reference{long} \ref{unfinished)"});
std::cout << source.tex().strip_refs() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["A B C D", "", "\\\\ref{escaped} \\reference{long} \\ref{unfinished"]
```

## Example 2: Empty Results Preserve Matrix Position

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{R"(x \ref{a} y)", R"(\nameref{b})", "plain", ""},
    std::array<std::int64_t, 2>{2, 2});
const auto clean = grid.tex().strip_refs();
std::cout << clean << '\n';
std::cout << "shape=[" << clean.size(0) << ", " << clean.size(1) << "]\n";
std::cout << std::boolalpha
          << "idempotent=" << ((clean == clean.tex().strip_refs()).all().bool_item())
          << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["x y", ""], 
  ["plain", ""]
]
shape=[2, 2]
idempotent=true
```

## Example 3: Undefined And Invalid Dtype

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().strip_refs().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().strip_refs();
} catch (const pluss::Error&) {
  std::cout << "strip refs Int64: rejected\n";
}
```

### Output

```text
false
strip refs Int64: rejected
```

## See Also

- [`refs`](refs.md): extract reference keys with source-cell offsets.
- [`strip_labels`](strip_labels.md): remove the anchors those references target.
- [`strip_links`](strip_links.md): retain visible labels while removing destinations.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_preamble.html"><span class="label">Previous</span><span class="title">strip_preamble</span></a>
  <a class="next-link" href="strip_size.html"><span class="label">Next</span><span class="title">strip_size</span></a>
</div>

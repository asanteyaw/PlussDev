<!-- pluss-hand-authored -->
# NoteTexAccessor::refs

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteTexAccessor::refs() const
```

## Extract Cross-Reference Targets

`refs()` extracts the required key argument from complete cross-reference
commands. It recognizes `\ref`, `\eqref`, `\autoref`, `\pageref`, `\nameref`,
`\cref`, `\Cref`, `\vref`, and `\Vref`, including starred forms such as
`\eqref*`. Each token is the argument body without the command or outer braces.

The method inventories uses of anchors; [`labels`](labels.md) inventories their
definitions. Comparing the two at document scope supports unresolved-reference
checks, while preserving duplicates and source order allows callers to retain
usage frequency and first-use position. `refs()` itself does not resolve keys,
enforce uniqueness, follow included files, or determine whether TeX executes a
command in its surrounding context.

Matching is exact and balanced. Nested braces in a key are retained,
whitespace between command and argument is accepted, and a doubled backslash,
longer name such as `\reference`, or incomplete braced argument contributes no
token. Optional arguments are not part of these supported call forms and are
not skipped.

The returned `TokenBatch` stores flat Text `tokens()` and Int64 `offsets()`.
Cell `i` owns `[offsets[i], offsets[i + 1])`; this retains lane ownership for
scalar, vector, matrix, and rank-three receivers without padding. Undefined
input returns undefined tokens and canonical offsets `[0]`. Defined non-Text
input is rejected. Scanning and copying are linear in source and retained-key
bytes.

## Example 1: Reference Families, Stars, And Balanced Keys

```cpp
const auto source = pluss::make_note({
    R"(\ref{eq:{energy}} \eqref* {eq:two} \cref{sec:a})",
    R"(\Cref{sec:b} \vref{fig:a} \Vref{fig:b} \nameref{name:a})",
    R"(\\ref{escaped} \reference{long} \ref{unfinished)"});
const auto result = source.tex().refs();

std::cout << result.tokens() << '\n';
```

### Output

```text
Note(shape=[7], dim=1, numel=7, encoding=utf8)
["eq:{energy}", "eq:two", "sec:a", "sec:b", "fig:a", "fig:b", "name:a"]
```

The first key retains its nested braces. The escaped, longer, and unfinished
forms do not create false targets.

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
offsets=[0, 3, 7, 7]
```

The first cell owns three targets, the second owns four, and the malformed-only
third cell owns an empty range.

## Example 3: Undefined And Invalid Input

```cpp
const auto absent = pluss::Note::undefined().tex().refs();
std::cout << std::boolalpha << absent.tokens().defined() << " offsets=["
          << absent.offsets().integers()[0] << "]\n";
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().refs();
} catch (const pluss::Error&) {
  std::cout << "refs Int64: rejected\n";
}
```

### Output

```text
false offsets=[0]
refs Int64: rejected
```

## See Also

- [`labels`](labels.md): extract anchor definitions.
- [`citations`](citations.md): extract bibliography-oriented citation arguments.
- [`strip_refs`](strip_refs.md): remove supported cross-reference commands from retained source.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="prime_norm.html"><span class="label">Previous</span><span class="title">prime_norm</span></a>
  <a class="next-link" href="render.html"><span class="label">Next</span><span class="title">render</span></a>
</div>

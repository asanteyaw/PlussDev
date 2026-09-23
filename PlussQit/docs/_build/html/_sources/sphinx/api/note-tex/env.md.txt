<!-- pluss-hand-authored -->
# NoteTexAccessor::env

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteTexAccessor::env(std::string_view name) const
```

## Extract One Environment Kind

`env()` returns complete `\begin{name}...\end{name}` regions for one requested
environment. Use it when the environment kind is known and the full authored
region, including delimiters, must survive. Unlike `env_names()`, it requires a
closing command; unlike `envs()`, it excludes every other environment kind.

Matching is exact and case-sensitive. `proof` does not select `proof*`, while
`align*` can be requested directly. The parser balances nested regions with
the same name, so an inner `proof` belongs to its outer `proof` token rather
than prematurely ending it. Different nested environment kinds are simply
content. Doubled-backslash text is not treated as an opener, and an incomplete
outer region produces no partial token.

Multiple non-overlapping matches in one cell are emitted in source order.
`tokens()` contains the complete regions and `offsets()` retains cell
ownership, including empty lanes. This is the canonical generic operation;
there are deliberately no per-environment aliases.

`name` accepts letters and `@`, plus an optional final `*`; it must not include
a leading backslash or braces. Invalid names throw. Undefined input returns an
undefined token Note with offsets `[0]`, and non-text input is rejected. The
balanced scan is linear and copies only matched regions.

## Example 1: Balance A Nested Environment

```cpp
const auto source = pluss::make_note({
    R"(\begin{proof}outer \begin{proof}inner\end{proof} tail\end{proof})",
    R"(\begin{lemma}other\end{lemma})", R"(\begin{proof}unfinished)",
    R"(\\begin{proof}escaped\end{proof})"});
const auto proofs = source.tex().env("proof");
std::cout << proofs.tokens() << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["\\begin{proof}outer \\begin{proof}inner\\end{proof} tail\\end{proof}"]
```

## Example 2: Empty Lanes Remain Addressable

```cpp
const auto offsets = proofs.offsets().integers();
std::cout << "offsets=[";
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 1, 1, 1, 1]
```

Only cell zero owns a match. The lemma, incomplete proof, and escaped opener
do not become proof tokens.

## Example 3: Reject A Malformed Name

```cpp
try { (void)source.tex().env("bad name"); }
catch (const pluss::Error&) { std::cout << "env(bad name): rejected\n"; }
```

### Output

```text
env(bad name): rejected
```

## See Also

- [`env_names`](env_names.md): inventory openers, including incomplete ones.
- [`envs`](envs.md): return all complete environment kinds.
- [`align_blocks`](align_blocks.md): select both `align` spellings semantically.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dots_norm.html"><span class="label">Previous</span><span class="title">dots_norm</span></a>
  <a class="next-link" href="env_names.html"><span class="label">Next</span><span class="title">env_names</span></a>
</div>

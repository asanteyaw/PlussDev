<!-- pluss-hand-authored -->
# Note::TexAccessor::env_names

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch Note::TexAccessor::env_names() const
```

## Inventory Environment Openers

`env_names()` extracts the name carried by each `\begin{name}` command. It is
the compact inventory operation for answering which environments a source cell
opens without copying complete environment bodies. Starred names remain
distinct: `align` and `align*` are different tokens.

The method reports openers, not validated pairs. A name is returned even when a
matching `\end{name}` is absent, because the method's contract is lexical
inventory rather than structural validation. Use `envs()` when only complete
begin/end regions should be returned, and `env(name)` when the
caller already knows the one environment kind it needs.

Names are emitted in source order and duplicates are preserved. Results from
all cells are flattened into `tokens()`, while `offsets()` provides one
cumulative boundary per logical input cell plus the initial zero. This keeps
empty cells distinguishable from neighboring cells without manufacturing an
empty-string token.

The receiver is never changed. Undefined input yields undefined tokens and
offsets `[0]`; Bool, Int64, and Float64 Notes are rejected because environment
syntax is defined only for Text and Bytes. The scan is linear in source bytes,
with allocation proportional to the extracted names and offsets.

## Example 1: Names, Stars, And Source Order

```cpp
const auto source = pluss::make_note({
    R"(\begin{figure}A\end{figure} \begin{align*}x&=1\end{align*})",
    R"(\begin{table}B\end{table})", "no environment"});
const auto names = source.tex().env_names();

std::cout << names.tokens() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["figure", "align*", "table"]
```

## Example 2: Offsets Include Empty Cells

```cpp
std::cout << "offsets=[";
const auto offsets = names.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 2, 3, 3]
```

The repeated final boundary says that the third source cell contributed no
environment name.

## Example 3: An Opener Does Not Need A Closer

```cpp
const auto partial = pluss::make_note(
    {R"(\begin{proof}unfinished)", R"(\end{proof})"})
    .tex().env_names();
std::cout << partial.tokens() << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["proof"]
```

This differs intentionally from `envs()`, which requires a complete pair.

## Example 4: Undefined And Invalid Dtypes

```cpp
const auto absent = pluss::Note::undefined().tex().env_names();
std::cout << std::boolalpha << absent.tokens().defined() << " offsets=["
          << absent.offsets().integers()[0] << "]\n";
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().env_names();
} catch (const pluss::Error&) {
  std::cout << "environment names Int64: rejected\n";
}
```

### Output

```text
false offsets=[0]
environment names Int64: rejected
```

## See Also

- [`envs`](envs.md): return complete environment regions.
- [`env`](env.md): select complete regions of one named kind.
- `commands`: inventory all TeX control words, not only environment openers.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="env.html"><span class="label">Previous</span><span class="title">env</span></a>
  <a class="next-link" href="envs.html"><span class="label">Next</span><span class="title">envs</span></a>
</div>

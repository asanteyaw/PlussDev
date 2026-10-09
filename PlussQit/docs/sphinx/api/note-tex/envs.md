<!-- pluss-hand-authored -->
# Note::TexAccessor::envs

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch Note::TexAccessor::envs() const
```

## Extract Complete Environment Regions

`envs()` extracts complete `\begin{name}...\end{name}` regions from every Text
cell. Each token retains the opener, body, and closer. This makes the result
suitable for relocating or separately processing authored blocks without
reconstructing delimiters or losing the environment name.

Unlike `env_names()`, this method requires a matching closer with the same
name. An unmatched opener or closer contributes no token. Starred environment
names are matched literally, so `\begin{figure*}` closes only at
`\end{figure*}`. Results retain source order and duplicates.

The return type is a `TokenBatch`, not a shape-preserving Note. `tokens()` is a
flat Text Note containing every extracted region; `offsets()` maps each logical
receiver cell to its half-open token range. This representation handles a
different number of environments per cell without nested host containers.

`envs()` performs extraction only. It does not render content, remove the
regions from the receiver, or validate commands inside a body. Use
`strip_env(name)` for removal and `env(name)` when a
known environment kind should be selected directly. Undefined input returns an
undefined token Note with offsets `[0]`; non-text input is rejected.

## Example 1: Return Whole Regions

```cpp
const auto source = pluss::make_note({
    R"(\begin{figure}A\end{figure} \begin{align*}x&=1\end{align*})",
    R"(\begin{table}B\end{table})", "no environment"});
const auto blocks = source.tex().envs();

std::cout << blocks.tokens() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["\\begin{figure}A\\end{figure}", "\\begin{align*}x&=1\\end{align*}", "\\begin{table}B\\end{table}"]
```

## Example 2: Map Regions Back To Cells

```cpp
std::cout << "offsets=[";
const auto offsets = blocks.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 2, 3, 3]
```

## Example 3: Incomplete Regions Are Not Fabricated

```cpp
const auto partial = pluss::make_note(
    {R"(\begin{proof}unfinished)", R"(\end{proof})"})
    .tex().envs();
std::cout << partial.tokens() << '\n';
std::cout << "offsets=[";
for (const auto value : partial.offsets().integers())
  std::cout << value << ' ';
std::cout << "]\n";
```

### Output

```text
Note(shape=[0], dim=1, numel=0, encoding=utf8)
[]
offsets=[0, 0, 0]
```

## Example 4: Undefined And Invalid Dtypes

```cpp
const auto absent = pluss::Note::undefined().tex().envs();
std::cout << std::boolalpha << absent.tokens().defined() << " offsets=["
          << absent.offsets().integers()[0] << "]\n";
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().envs();
} catch (const pluss::Error&) {
  std::cout << "environments Int64: rejected\n";
}
```

### Output

```text
false offsets=[0]
environments Int64: rejected
```

## See Also

- [`env_names`](env_names.md): inventory openers without requiring closers.
- [`env`](env.md): extract only one requested environment kind.
- `strip_env`: remove one requested environment kind.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="env_names.html"><span class="label">Previous</span><span class="title">env_names</span></a>
  <a class="next-link" href="escape.html"><span class="label">Next</span><span class="title">escape</span></a>
</div>

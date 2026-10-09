<!-- pluss-hand-authored -->
# Note::TexAccessor::strip_envs

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::strip_envs() const
```

## Remove Complete Environment Regions

`strip_envs()` removes every complete `\begin{name}...\end{name}` region,
regardless of the environment name. It is the broad counterpart to
[`strip_env(name)`](strip_env.md): use the named form when one structural kind
must disappear and unrelated environments must survive; use `strip_envs()`
when the desired result is prose outside all complete environments.

The parser balances a stack of environment names. Consequently, nested regions
with different names and same-name recursion are consumed as one outer region,
rather than stopping at the first `\end{...}`. Names are lexical TeX names:
ASCII letters and `@`, with an optional terminal `*`. An end tag closes only
the matching top-of-stack name. Escaped openers are ordinary text, and an
unclosed outer region is retained in full; broad cleanup never treats the end
of a cell as an implied close.

Each cell is processed independently. Deleted regions leave one separator,
then ASCII whitespace is collapsed and the result is trimmed. The returned
Text Note has the receiver's exact rank and shape, including cells that become
empty. The method is out-of-place and idempotent. Undefined input propagates;
defined Bool, Int64, Float64, and Bytes Notes reject. The scan is linear in the
number of source bytes, with stack memory proportional to nesting depth.

## Example 1: Mixed Nesting, Multiple Regions, And Malformed Source

```cpp
const auto source = pluss::make_note({
    R"(A \begin{quote}outer \begin{emph}inner\end{emph} tail\end{quote} B)",
    R"(C \begin{proof}one\end{proof} D \begin{lemma}two\end{lemma} E)",
    R"(\\begin{proof}escaped\end{proof} \begin{proof}unfinished)"});
std::cout << source.tex().strip_envs() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["A B", "C D E", "\\\\begin{proof}escaped\\end{proof} \\begin{proof}unfinished"]
```

The nested `emph` region belongs to the outer `quote` region, both complete
regions in the second lane are removed, and neither escaped nor incomplete
syntax is guessed away.

## Example 2: Matrices Keep Empty Cells

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{R"(x \begin{a}1\end{a} y)", "plain",
                             R"(\begin{b}2\end{b})", ""},
    std::array<std::int64_t, 2>{2, 2});
const auto clean = grid.tex().strip_envs();
std::cout << clean << '\n';
std::cout << "shape=[" << clean.size(0) << ", " << clean.size(1) << "]\n";
std::cout << std::boolalpha
          << "idempotent=" << ((clean == clean.tex().strip_envs()).all().bool_item())
          << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["x y", "plain"], 
  ["", ""]
]
shape=[2, 2]
idempotent=true
```

## Example 3: Undefined And Invalid Dtype

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().strip_envs().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().strip_envs();
} catch (const pluss::Error&) {
  std::cout << "strip envs Int64: rejected\n";
}
```

### Output

```text
false
strip envs Int64: rejected
```

## See Also

- [`strip_env`](strip_env.md): remove only one caller-selected environment.
- [`envs`](envs.md): extract complete environments with source-cell offsets.
- [`strip_figs`](strip_figs.md): remove only figure-family regions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_env.html"><span class="label">Previous</span><span class="title">strip_env</span></a>
  <a class="next-link" href="strip_figs.html"><span class="label">Next</span><span class="title">strip_figs</span></a>
</div>

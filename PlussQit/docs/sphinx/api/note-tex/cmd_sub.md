<!-- pluss-hand-authored -->
# NoteTexAccessor::cmd_sub

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::cmd_sub(std::string_view cmd,
                             std::string_view repl) const
```

## Substitute One Control Word

`cmd_sub(cmd, repl)` replaces each lexical occurrence of one TeX control word
with caller-provided text. It is the precise choice when the command name is
known and its replacement is independent of attached arguments. It does not
parse or consume `{...}` or `[...]` groups after the command, and it does not
expand macro definitions. Use [`macro_subs()`](macro_subs.md) when declarations
inside each cell should determine expansion; use [`strip_cmds()`](strip_cmds.md)
when command calls and their attached groups should disappear.

`cmd` accepts either `alpha` or `\alpha`. A terminal `*` is significant:
`alpha` and `alpha*` select different tokens. The selector otherwise contains
only ASCII letters or `@`; an empty selector, whitespace, punctuation, or an
embedded backslash rejects before the receiver is read. Matching is exact and
case-sensitive, so `\alpha` does not match `\alphabet`, `\Alpha`, or
`\alpha*`. A doubled backslash protects the following text from being read as
a control word.

`repl` is copied literally. Dollar signs, backslashes, braces, and `$&` have no
regex replacement meaning. An empty replacement therefore removes only the
selected control word while retaining any following groups. The operation is
out-of-place, preserves dtype and shape, and leaves the receiver unchanged.
It is not inherently idempotent: a replacement that contains the selected
command can be matched by a later call.

Undefined state propagates after selector validation; defined non-Text input
rejects. The kernel scans each cell once and allocates one output string per
cell. Runtime is linear in source bytes plus bytes copied from `repl`.

## Example 1: Exact Matching And Literal Replacement

```cpp
const auto source = pluss::make_note({
    R"(\alpha + \alpha* + \alphabet + \\alpha)",
    R"(\alpha* + \alpha)",
    "plain"});
std::cout << source.tex().cmd_sub("alpha", "[$&]") << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["[$&] + \\alpha* + \\alphabet + \\\\alpha", "\\alpha* + [$&]", "plain"]
```

`$&` is copied as two ordinary characters. The starred token, longer command,
and escaped spelling remain untouched.

## Example 2: Starred Selection And Leading Backslash

```cpp
const auto source = pluss::make_note({
    R"(\alpha + \alpha* + \alphabet + \\alpha)",
    R"(\alpha* + \alpha)",
    "plain"});
std::cout << source.tex().cmd_sub("alpha*", "STAR") << '\n';
std::cout << std::boolalpha
          << "leading slash same="
          << ((source.tex().cmd_sub("alpha", "A") ==
               source.tex().cmd_sub(R"(\alpha)", "A")).all().bool_item())
          << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["\\alpha + STAR + \\alphabet + \\\\alpha", "STAR + \\alpha", "plain"]
leading slash same=true
```

## Example 3: Matrix Shape Is Preserved

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{R"(\alpha)", R"(\beta)", "",
                             R"(\alpha+\alpha)"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << grid.tex().cmd_sub("alpha", "A") << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["A", "\\beta"],
  ["", "A+A"]
]
```

## Example 4: Validation, Undefined State, And Dtype

```cpp
try {
  (void)pluss::Note::undefined().tex().cmd_sub("bad name", "x");
} catch (const pluss::Error&) {
  std::cout << "cmd_sub(bad name): rejected\n";
}
std::cout << std::boolalpha << "undefined="
          << pluss::Note::undefined().tex().cmd_sub("alpha", "A").defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1})
      .tex().cmd_sub("alpha", "A");
} catch (const pluss::Error&) {
  std::cout << "cmd sub Int64: rejected\n";
}
```

### Output

```text
cmd_sub(bad name): rejected
undefined=false
cmd sub Int64: rejected
```

## See Also

- [`macro_subs`](macro_subs.md): expand simple declarations found in each cell.
- [`canonicalize_macros`](canonicalize_macros.md): normalize a fixed family of equivalent spellings.
- [`strip_cmds`](strip_cmds.md): remove commands together with attached groups.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="citations.html"><span class="label">Previous</span><span class="title">citations</span></a>
  <a class="next-link" href="collapse_macros.html"><span class="label">Next</span><span class="title">collapse_macros</span></a>
</div>

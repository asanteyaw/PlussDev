<!-- pluss-hand-authored -->
# Note::TexAccessor::strip_size

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::strip_size() const
```

## Remove Explicit Delimiter-Size Commands

`strip_size()` removes the twelve delimiter-sizing control words `\big`,
`\Big`, `\bigg`, `\Bigg`, and their `l`/`r` forms. The delimiter characters
that follow remain, so `\Bigl( x \Bigr)` becomes `( x )` rather than losing
the parentheses or their contents.

Matching is by complete control-word name. Escaped commands and longer names
such as `\BiglExtra` are untouched. The method does not remove `\left` or
`\right`, alter delimiter characters, or validate pairing; it addresses only
explicit `big`-family size annotations.

ASCII whitespace is collapsed and cells are trimmed after rewriting. The
operation allocates a shape-preserving Text Note and leaves its receiver
unchanged. Undefined input propagates and non-Text input rejects. The scanner
is linear and performs no regular-expression replacement passes.

## Example 1: Every Match Leaves Its Delimiter

```cpp
const auto source = pluss::make_note({
    R"(\Bigl( x \Bigr) + \bigg[y\bigg])",
    R"(\\Bigl literal \BiglExtra keep)", "plain"});
std::cout << source.tex().strip_size() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["( x ) + [y]", "\\\\Bigl literal \\BiglExtra keep", "plain"]
```

## Example 2: Delimiter Families And Non-Targets

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{R"(\bigl\{a\bigr\})", R"(\Bigg|x\Bigg|)",
                             R"(\left(y\right))", ""},
    std::array<std::int64_t, 2>{2, 2});
const auto clean = grid.tex().strip_size();
std::cout << clean << '\n';
std::cout << "shape=[" << clean.size(0) << ", " << clean.size(1) << "]\n";
std::cout << std::boolalpha
          << "idempotent=" << ((clean == clean.tex().strip_size()).all().bool_item())
          << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["\\{a\\}", "|x|"], 
  ["\\left(y\\right)", ""]
]
shape=[2, 2]
idempotent=true
```

Escaped braces and vertical bars are delimiter tokens and remain. Automatic
`\left`/`\right` sizing is a separate construct and is deliberately untouched.

## Example 3: Undefined And Invalid Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().strip_size().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().strip_size();
} catch (const pluss::Error&) {
  std::cout << "strip size Int64: rejected\n";
}
```

### Output

```text
false
strip size Int64: rejected
```

## See Also

- [`strip_lr`](strip_lr.md): remove automatic delimiter sizing commands.
- [`balance_braces`](balance_braces.md): repair brace structure without changing sizing.
- [`strip_cmds`](strip_cmds.md): remove general control-word calls.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_refs.html"><span class="label">Previous</span><span class="title">strip_refs</span></a>
  <a class="next-link" href="strip_space.html"><span class="label">Next</span><span class="title">strip_space</span></a>
</div>

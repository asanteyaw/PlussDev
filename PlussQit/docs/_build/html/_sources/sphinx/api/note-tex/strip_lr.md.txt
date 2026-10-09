<!-- pluss-hand-authored -->
# Note::TexAccessor::strip_lr

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::strip_lr() const
```

## Remove Automatic Delimiter Sizing

`strip_lr()` removes the exact control words `\left` and `\right`. Their
following delimiter tokens and enclosed expression remain. It converts source
such as `\left( x \right)` to `( x )` without changing the expression, and is
useful when delimiter size is presentation noise rather than authored data.
Explicit `big`-family sizing is separate; remove it with
[`strip_size()`](strip_size.md).

Matching is lexical, not substring based. Commands such as `\leftarrow`,
`\rightarrow`, and `\leftover` remain intact, as do escaped command text.
`strip_lr()` does not validate that left and right commands are paired. It also
does not reinterpret delimiters: the dot in `\left.` is TeX's invisible
delimiter token, so removing only `\left` leaves that dot visible in source.

The result is whitespace-normalized, out-of-place, shape preserving, and
idempotent. Undefined input remains undefined and defined non-Text Notes
reject. The implementation performs one linear control-word scan.

## Example 1: Delimiters, Invisible Sides, And Prefix Safety

```cpp
const auto source = pluss::make_note({
    R"(\left( x \right) + \left\{ y \right\})",
    R"(\left. f \right|_0^1 + \leftarrow + \rightarrow)",
    R"(\\left literal \leftover kept)"});
std::cout << source.tex().strip_lr() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["( x ) + \\{ y \\}", ". f |_0^1 + \\leftarrow + \\rightarrow", "\\\\left literal \\leftover kept"]
```

## Example 2: Named Delimiters And Shape

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{R"(\left[a\right])", "plain",
                             R"(\left\langle b\right\rangle)", ""},
    std::array<std::int64_t, 2>{2, 2});
const auto clean = grid.tex().strip_lr();
std::cout << clean << '\n';
std::cout << "shape=[" << clean.size(0) << ", " << clean.size(1) << "]\n";
std::cout << std::boolalpha
          << "idempotent=" << ((clean == clean.tex().strip_lr()).all().bool_item())
          << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["[a]", "plain"], 
  ["\\langle b\\rangle", ""]
]
shape=[2, 2]
idempotent=true
```

## Example 3: Undefined And Invalid Dtype

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().strip_lr().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().strip_lr();
} catch (const pluss::Error&) {
  std::cout << "strip lr Int64: rejected\n";
}
```

### Output

```text
false
strip lr Int64: rejected
```

## See Also

- [`strip_size`](strip_size.md): remove explicit `big`-family sizing commands.
- [`strip_delims`](strip_delims.md): remove math-region markers rather than delimiters.
- [`balance_braces`](balance_braces.md): repair lexical brace balance.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_links.html"><span class="label">Previous</span><span class="title">strip_links</span></a>
  <a class="next-link" href="strip_macro.html"><span class="label">Next</span><span class="title">strip_macro</span></a>
</div>

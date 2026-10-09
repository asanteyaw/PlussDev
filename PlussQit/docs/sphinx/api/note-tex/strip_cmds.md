<!-- pluss-hand-authored -->
# Note::TexAccessor::strip_cmds

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::strip_cmds() const
```

## Remove Control Words And Their Attached Groups

`strip_cmds()` removes every complete TeX control word and all immediately
attached balanced optional or required groups. A call such as
`\frac{one}{two}` therefore disappears as one unit; its argument text is not
promoted into prose. Bare control words are removed by themselves.

Control symbols are deliberately preserved. Escaped literal forms such as
`\$`, and doubled-backslash boundaries, are not ordinary control-word calls.
Malformed attached groups are not consumed, although the recognized command
name itself is removed. This broad operation is appropriate for discarding
command-bearing content; use `to_text()` or a focused wrapper remover when
visible argument text should survive.

The result collapses ASCII whitespace and trims each cell after deletion. It
is out-of-place and preserves rank and shape. Undefined input stays undefined;
defined non-Text input rejects. The scanner is linear and balances groups
without constructing a syntax tree.

## Example 1: Multiple Arguments And Escaped Symbols

```cpp
const auto source = pluss::make_note({
    R"(A \frac{one {nested}}{two} B \alpha C)",
    R"(price \$5 and \\alpha literal)", R"(\broken{unfinished)"});
std::cout << source.tex().strip_cmds() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["A B C", "price \\$5 and \\\\alpha literal", "{unfinished"]
```

The complete fraction and bare `\alpha` disappear. Escaped syntax survives,
while the malformed command loses only its valid control-word prefix.

## Example 2: Starred Calls, Unknown Commands, And Shape

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{R"(A \section*[short]{Long} B)",
                             R"(x \unknown y)", "plain", ""},
    std::array<std::int64_t, 2>{2, 2});
const auto clean = grid.tex().strip_cmds();
std::cout << clean << '\n';
std::cout << "shape=[" << clean.size(0) << ", " << clean.size(1) << "]\n";
std::cout << std::boolalpha
          << "idempotent=" << ((clean == clean.tex().strip_cmds()).all().bool_item())
          << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["A B", "x y"], 
  ["plain", ""]
]
shape=[2, 2]
idempotent=true
```

Recognition is syntactic rather than dictionary based: an unknown control
word is still a control word, and a starred command consumes its attached
optional and required groups.

## Example 3: Undefined And Invalid Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().strip_cmds().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().strip_cmds();
} catch (const pluss::Error&) {
  std::cout << "strip cmds Int64: rejected\n";
}
```

### Output

```text
false
strip cmds Int64: rejected
```

## See Also

- [`commands`](commands.md): extract command lexemes or complete calls.
- [`to_text`](to_text.md): retain readable argument content during broader conversion.
- [`strip_color`](strip_color.md): remove color syntax while retaining visible content.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_cites.html"><span class="label">Previous</span><span class="title">strip_cites</span></a>
  <a class="next-link" href="strip_color.html"><span class="label">Next</span><span class="title">strip_color</span></a>
</div>

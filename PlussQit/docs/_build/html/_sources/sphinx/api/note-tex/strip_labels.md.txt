<!-- pluss-hand-authored -->
# Note::TexAccessor::strip_labels

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::strip_labels() const
```

## Remove Anchor Declarations

`strip_labels()` removes complete `\label{key}` calls, including starred
spelling, while retaining the prose and TeX structures around them. Labels
declare cross-reference anchors but normally contribute no visible text, so
their key is discarded with the call. Use [`labels()`](labels.md) when those
keys are data to extract, and [`strip_refs()`](strip_refs.md) for the commands
that point to anchors.

The required key group is brace balanced. Nested braces therefore do not end a
call early. Matching uses an exact control-word boundary: `\labelled` is not a
label. Escaped and incomplete calls remain unchanged rather than losing a
prefix or a partial key.

After deletion, ASCII whitespace is collapsed and each cell is trimmed. The
method allocates a new Text Note, leaves its receiver unchanged, preserves rank
and shape, and is idempotent. Undefined state propagates; defined non-Text
dtypes reject. Work is linear in source bytes.

## Example 1: Nested Keys, Stars, And Boundaries

```cpp
const auto source = pluss::make_note({
    R"(A \label{sec:{nested}} B \label*{starred} C)",
    R"(\\label{escaped} \labelled{long} \label{unfinished)"});
std::cout << source.tex().strip_labels() << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["A B C", "\\\\label{escaped} \\labelled{long} \\label{unfinished"]
```

## Example 2: Matrix Position And Allocation

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{R"(x \label{a} y)", R"(\label{b})", "plain", ""},
    std::array<std::int64_t, 2>{2, 2});
const auto clean = grid.tex().strip_labels();
std::cout << clean << '\n';
std::cout << "shape=[" << clean.size(0) << ", " << clean.size(1) << "]\n";
std::cout << std::boolalpha
          << "receiver unchanged=" << (grid.at(0).find("label") != std::string_view::npos)
          << " idempotent=" << ((clean == clean.tex().strip_labels()).all().bool_item())
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
receiver unchanged=true idempotent=true
```

## Example 3: Undefined And Invalid Dtype

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().strip_labels().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().strip_labels();
} catch (const pluss::Error&) {
  std::cout << "strip labels Int64: rejected\n";
}
```

### Output

```text
false
strip labels Int64: rejected
```

## See Also

- [`labels`](labels.md): extract label keys with source-cell offsets.
- [`strip_refs`](strip_refs.md): remove calls that consume reference keys.
- [`strip_cmds`](strip_cmds.md): remove arbitrary commands and attached groups.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_graphics.html"><span class="label">Previous</span><span class="title">strip_graphics</span></a>
  <a class="next-link" href="strip_links.html"><span class="label">Next</span><span class="title">strip_links</span></a>
</div>

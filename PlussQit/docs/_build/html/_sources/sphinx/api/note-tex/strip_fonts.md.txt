<!-- pluss-hand-authored -->
# Note::TexAccessor::strip_fonts

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::strip_fonts() const
```

## Remove Wrapper And Declaration Font Syntax

`strip_fonts()` first unwraps every formatting command supported by
[`strip_fmt()`](strip_fmt.md), then removes the declaration switches
`\rmfamily`, `\sffamily`, `\ttfamily`, `\bfseries`, `\itshape`, and
`\scshape`. It is intended for extracting authored text from source that mixes
argument-style formatting (`\textbf{...}`) with scope-style declarations
(`{\bfseries ...}`).

The distinction matters: a wrapper owns a balanced body, which is retained;
a declaration changes the surrounding TeX scope and has no argument for this
method to consume. Therefore declaration removal leaves surrounding braces in
place. `strip_fonts()` is not a TeX group simplifier, and it does not infer
where a declaration's visual effect ends. Exact control-word boundaries keep
names such as `\bfseriesExtra` intact, while escaped commands and malformed
wrappers are preserved.

The result is a newly allocated, whitespace-normalized Text Note with the same
rank, shape, and cell order. Reapplying the method has no further effect.
Undefined input remains undefined and every defined non-Text dtype rejects.
The work is linear in source bytes and requires no rendered-font engine.

## Example 1: Scoped Declarations And Wrappers

```cpp
const auto source = pluss::make_note({
    R"(A {\bfseries bold} B {\itshape italic} C)",
    R"(\textbf{wrapped} \ttfamily mono)",
    R"(\\bfseries escaped \bfseriesExtra kept)"});
const auto clean = source.tex().strip_fonts();
std::cout << clean << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["A { bold} B { italic} C", "wrapped mono", "\\\\bfseries escaped \\bfseriesExtra kept"]
```

The braces in the first lane remain because they are source grouping, not part
of the declaration token. Use a separate brace-aware transformation only if
removing redundant groups is also intended.

## Example 2: The Supported Declaration Family

```cpp
std::cout << pluss::make_note(
    R"(\rmfamily roman \sffamily sans \scshape caps)").tex().strip_fonts()
          << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"roman sans caps"
```

## Example 3: Idempotence, Undefined State, And Dtype

```cpp
const auto once = pluss::make_note(
    R"(\textbf{bold} \bfseries tail)").tex().strip_fonts();
std::cout << std::boolalpha
          << "idempotent=" << ((once == once.tex().strip_fonts()).all().bool_item())
          << " undefined="
          << pluss::Note::undefined().tex().strip_fonts().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().strip_fonts();
} catch (const pluss::Error&) {
  std::cout << "strip fonts Int64: rejected\n";
}
```

### Output

```text
idempotent=true undefined=false
strip fonts Int64: rejected
```

## See Also

- [`strip_fmt`](strip_fmt.md): unwrap formatting commands but retain declarations.
- [`strip_color`](strip_color.md): remove color styling without removing visible bodies.
- [`to_text`](to_text.md): perform a broader readable-text projection.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_fmt.html"><span class="label">Previous</span><span class="title">strip_fmt</span></a>
  <a class="next-link" href="strip_graphics.html"><span class="label">Next</span><span class="title">strip_graphics</span></a>
</div>

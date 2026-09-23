<!-- pluss-hand-authored -->
# NoteTexAccessor::graphics_tags

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::graphics_tags() const
```

## Retain Graphic Locations As Text

`graphics_tags()` rewrites each complete `\includegraphics[options]{path}` call
as `[graphic:path]`. It exists for pipelines that cannot render or load an
asset but must retain where the source referred to one. The method performs no
filesystem access, path normalization, existence check, image decoding, or
option interpretation. The path is authored text, not a resolved resource.

The command name is matched exactly. Its optional group and required path group
are parsed with balanced delimiters, so nested braces in an option or path do
not truncate the call. Options disappear from the result; whitespace and text
around the call remain. Escaped commands, longer names such as
`\includegraphicsx`, and incomplete option or path groups are preserved
verbatim because the method cannot identify a complete replaceable construct.

Choose [`includes()`](includes.md) when paths should be extracted into a
lane-aware `TokenBatch`, [`input_tags()`](input_tags.md) when `input`,
`include`, and graphic directives all need visible tags, or
[`strip_graphics()`](strip_graphics.md) when graphic directives and
figure-family environments should be removed.

The result is a newly allocated Text Note with the receiver's shape. Empty
cells stay empty, and applying the operation again is stable because generated
tags contain no TeX command. Undefined state propagates; defined non-Text input
rejects. The balanced scanner is linear in source bytes and uses nesting-depth
storage for the current option or path.

## Example 1: Balanced Options, Nested Paths, And Boundaries

```cpp
const auto source = pluss::make_note({
    R"(A \includegraphics[width={.5\textwidth}]{plots/{loss}.pdf} B)",
    R"(\\includegraphics{escaped} \includegraphicsx{long})",
    R"(\includegraphics[width=2cm]{unfinished)"});
std::cout << source.tex().graphics_tags() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["A [graphic:plots/{loss}.pdf] B", "\\\\includegraphics{escaped} \\includegraphicsx{long}", "\\includegraphics[width=2cm]{unfinished"]
```

## Example 2: Batched Shape Is Preserved

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{R"(\includegraphics{a.png})", "plain",
                             R"(x \includegraphics{b.pdf} y)", ""},
    std::array<std::int64_t, 2>{2, 2});
std::cout << grid.tex().graphics_tags() << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["[graphic:a.png]", "plain"],
  ["x [graphic:b.pdf] y", ""]
]
```

## Example 3: Undefined State, Dtype, And Idempotence

```cpp
const auto once = pluss::make_note(R"(\includegraphics{plot.pdf})")
                      .tex().graphics_tags();
std::cout << std::boolalpha
          << "idempotent="
          << ((once == once.tex().graphics_tags()).all().bool_item())
          << " undefined="
          << pluss::Note::undefined().tex().graphics_tags().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1})
      .tex().graphics_tags();
} catch (const pluss::Error&) {
  std::cout << "graphics tags Int64: rejected\n";
}
```

### Output

```text
idempotent=true undefined=false
graphics tags Int64: rejected
```

## See Also

- [`includes`](includes.md): extract external-resource targets with lane offsets.
- [`input_tags`](input_tags.md): tag source includes and graphics together.
- [`strip_graphics`](strip_graphics.md): remove graphic calls and figure regions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="frac_slash.html"><span class="label">Previous</span><span class="title">frac_slash</span></a>
  <a class="next-link" href="greek.html"><span class="label">Next</span><span class="title">greek</span></a>
</div>

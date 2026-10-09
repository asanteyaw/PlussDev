<!-- pluss-hand-authored -->
# Note::TexAccessor::wrap_display_math

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::wrap_display_math(std::string_view delim) const
```

## Wrap Each Cell As A Display Math Block

`wrap_display_math()` trims each cell and wraps it as a TeX display math
block, inserting a newline between each delimiter and the content:
`open\ncontent\nclose`. An empty `delim` defaults to `\[`/`\]`; the literal
two-character delimiter `"$$"` is special-cased to `$$`/`$$` (the same
string on both sides); any other non-empty `delim` is used verbatim as
**both** the opening and closing delimiter — there is no asymmetric
open/close pairing for a custom delimiter, so a value such as `"|"` wraps
as `|\ncontent\n|`, not a matched pair of distinct open/close characters.

**This method does not check whether the cell already contains math
delimiters.** Contrast this directly with
[`wrap_inline_math`](wrap_inline_math.md), which skips a cell that already
has a `$`, `\(`, or `\[` anywhere in it: `wrap_display_math()` always wraps
a non-empty cell, even one that is already `$already$` — the existing
dollar signs end up nested one level inside the new outer delimiters. A
cell that is empty after trimming is returned empty, with no delimiters
added.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. Undefined input remains undefined; Bool, Int64, Float64, and Bytes
receivers are rejected because TeX source is Text.

## Example 1: Default Delimiter, Empty Cell, And Unconditional Wrapping

```cpp
std::cout << pluss::make_note({"x+y", "", "$already$"})
              .tex().wrap_display_math("")
          << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["\\[\nx+y\n\\]", "", "\\[\n$already$\n\\]"]
```

The empty second cell stays empty. The third cell, already containing `$`
delimiters, is still wrapped — unlike `wrap_inline_math`, there is no
existing-math check here.

## Example 2: The `"$$"` Special Case

```cpp
std::cout << pluss::make_note({"x+y"}).tex().wrap_display_math("$$") << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["$$\nx+y\n$$"]
```

## Example 3: A Custom Delimiter Is Used On Both Sides

```cpp
std::cout << pluss::make_note({"x+y"}).tex().wrap_display_math("~") << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["~\nx+y\n~"]
```

`~` is not paired with a distinct closing character; the same delimiter
opens and closes.

## Example 4: Undefined And Non-Text Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().wrap_display_math("").defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().wrap_display_math("");
} catch (const pluss::Error&) {
  std::cout << "wrap display math Int64: rejected\n";
}
```

### Output

```text
false
wrap display math Int64: rejected
```

## See Also

- [`wrap_inline_math`](wrap_inline_math.md): the inline counterpart, which
  skips cells that already contain a math delimiter.
- [`add_math_delimiters`](add_math_delimiters.md): choose inline or display
  wrapping automatically per cell instead of committing to one form.
- [`balance_math_delims`](balance_math_delims.md): repair unbalanced
  delimiters already present, rather than adding new ones.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="warnings.html"><span class="label">Previous</span><span class="title">warnings</span></a>
  <a class="next-link" href="wrap_inline_math.html"><span class="label">Next</span><span class="title">wrap_inline_math</span></a>
</div>

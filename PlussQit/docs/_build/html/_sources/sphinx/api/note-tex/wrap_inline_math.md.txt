<!-- pluss-hand-authored -->
# NoteTexAccessor::wrap_inline_math

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::wrap_inline_math(std::string_view delim) const
```

## Wrap Each Cell As Inline Math, Unless It Already Looks Like Math

`wrap_inline_math()` trims each cell and wraps it as inline TeX math:
`delim + content + delim`. An empty `delim` defaults to `$`; any non-empty
`delim` is used verbatim as **both** the leading and trailing delimiter —
there is no asymmetric open/close pairing, so `wrap_inline_math("~")` on
`a=b` produces `~a=b~`, not a matched pair of distinct characters.

**This method skips a cell that already contains a math marker.** If the
trimmed cell contains a `$`, `\(`, or `\[` anywhere in it, the cell is
returned completely unchanged — no delimiter is added, and the check does
not require the existing marker to be balanced or even at the edges of the
cell. Contrast this with [`wrap_display_math`](wrap_display_math.md), which
has no such check and always wraps a non-empty cell, nesting any existing
delimiters one level inside the new ones. A cell that is empty after
trimming is returned empty either way, with no delimiter added.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. Undefined input remains undefined; Bool, Int64, Float64, and Bytes
receivers are rejected because TeX source is Text.

## Example 1: Default Delimiter, Empty Cell, And Two Kinds Of Existing Math

```cpp
std::cout << pluss::make_note({"x+y", "", "$already$", R"(\(also\))"})
              .tex().wrap_inline_math("")
          << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["$x+y$", "", "$already$", "\\(also\\)"]
```

The first cell, with no existing math marker, gets `$...$`. The empty
second cell stays empty. The third and fourth cells already contain a `$`
and a `\(` respectively, so both are returned completely unmodified — not
even a check for whether the marker is balanced.

## Example 2: Idempotent By Construction

```cpp
const auto wrapped = pluss::make_note({"x+y"}).tex().wrap_inline_math("");
std::cout << std::boolalpha
          << (wrapped.tex().wrap_inline_math("") == wrapped).all().bool_item()
          << '\n';
```

### Output

```text
true
```

Once a cell has been wrapped, it contains `$`, so a second call's
existing-marker check leaves it unchanged.

## Example 3: A Custom Delimiter Is Used On Both Sides

```cpp
std::cout << pluss::make_note({"a=b"}).tex().wrap_inline_math("~") << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["~a=b~"]
```

## Example 4: Undefined And Non-Text Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().wrap_inline_math("").defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().wrap_inline_math("");
} catch (const pluss::Error&) {
  std::cout << "wrap inline math Int64: rejected\n";
}
```

### Output

```text
false
wrap inline math Int64: rejected
```

## See Also

- [`wrap_display_math`](wrap_display_math.md): the display counterpart,
  which always wraps regardless of existing delimiters.
- [`add_math_delimiters`](add_math_delimiters.md): choose inline or display
  wrapping automatically per cell instead of committing to one form.
- [`math_only`](math_only.md): extract existing math content rather than
  adding new delimiters.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="wrap_display_math.html"><span class="label">Previous</span><span class="title">wrap_display_math</span></a>
</div>

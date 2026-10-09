<!-- pluss-hand-authored -->
# Note::TexAccessor::dedup_blank_lines

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::dedup_blank_lines() const
```

## Cap Vertical Whitespace Without Reflowing Source

`dedup_blank_lines()` reduces every run of two or more blank lines to one
blank line. In byte terms, three or more consecutive line-ending sequences,
with optional spaces or tabs on the intervening empty lines, become exactly
two line endings. The first ends the preceding content line; the second is the
single retained blank line.

A blank line may be empty or contain only horizontal ASCII whitespace. Spaces
and tabs on a collapsed blank run are removed. Content-line indentation,
single line endings, and an already singular blank line remain untouched. The
method does not trim the document, wrap text, collapse ordinary spaces, or
interpret TeX environments; it is intentionally narrower than
`fix_linebreaks()`.

LF (`\n`), CRLF (`\r\n`), and CR (`\r`) are all recognized. Deduplication
preserves the original line-ending bytes of the two retained boundaries; it
does not silently convert CRLF to LF. Use `line_norm()` when a uniform
LF representation is required, and choose the order deliberately: normalize
then deduplicate for canonical LF output, or deduplicate alone to retain the
source convention.

Each Text cell is processed independently. The result is a newly allocated
Note with the same rank and shape; empty cells, cells without excess blank
lines, and undefined state are preserved. Native non-Text dtypes reject. The
kernel scans each byte once and reserves at most the original byte count, so
its working cost is linear without per-line container allocation.

## Example 1: Collapse An LF Run

```cpp
const auto source = pluss::make_note("alpha\n\n\n\nbeta");
std::cout << source.tex().dedup_blank_lines() << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"alpha\n\nbeta"
```

## Example 2: Whitespace-Only Blank Lines

```cpp
const auto source = pluss::make_note("one\n  \n\t\n\n\ntwo");
std::cout << source.tex().dedup_blank_lines() << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"one\n\ntwo"
```

## Example 3: CRLF Is Preserved

```cpp
const auto source = pluss::make_note("one\r\n \r\n\t\r\ntwo");
std::cout << source.tex().dedup_blank_lines() << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"one\r\n\r\ntwo"
```

## Example 4: Batched Shape And Stable Cells

```cpp
const auto spaced = pluss::make_note(
    std::vector<std::string>{"alpha\n\n\n\nbeta", "one\r\n \r\n\t\r\ntwo",
                             "already\n\ncompact", "single line"},
    std::array<std::int64_t, 2>{2, 2});
const auto compact = spaced.tex().dedup_blank_lines();
std::cout << compact << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["alpha\n\nbeta", "one\r\n\r\ntwo"], 
  ["already\n\ncompact", "single line"]
]
```

## Example 5: Compose With Newline Normalization

```cpp
const auto canonical =
    pluss::make_note("one\r\n \r\n\t\r\ntwo")
        .tex().line_norm().tex().dedup_blank_lines();
std::cout << canonical << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"one\n\ntwo"
```

## Example 6: Undefined And Invalid Dtype

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().dedup_blank_lines().defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().dedup_blank_lines();
} catch (const pluss::Error&) {
  std::cout << "blank lines Int64: rejected\n";
}
```

### Output

```text
false
blank lines Int64: rejected
```

## See Also

- `line_norm`: convert CRLF and CR endings to LF.
- `fix_linebreaks`: perform broader line-ending and trailing-space cleanup.
- [`Note::collapse_whitespace`](../note/collapse_whitespace.md): collapse all
  ASCII whitespace rather than preserving document lines.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="deaccent.html"><span class="label">Previous</span><span class="title">deaccent</span></a>
  <a class="next-link" href="display_math.html"><span class="label">Next</span><span class="title">display_math</span></a>
</div>

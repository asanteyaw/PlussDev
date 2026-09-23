<!-- pluss-hand-authored -->
# NoteTexAccessor::fix_linebreaks

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::fix_linebreaks() const
```

## Canonicalize Physical Lines

`fix_linebreaks()` performs three source-level repairs in one shape-preserving
pass: it converts CRLF and bare carriage returns to `\n`, removes spaces and
tabs immediately before each newline, and limits a run of physical newline
records to two. Two retained newlines represent one blank line, so paragraph
separation survives while accidental vertical gaps are compacted.

The method operates on physical line endings, not TeX's `\\` control symbol.
It does not reflow prose, insert or remove TeX linebreak commands, trim leading
indentation, or remove spaces elsewhere in a line. A final physical newline is
also retained after its trailing horizontal whitespace is removed. Use
`line_norm()` when only line-ending spelling should change and
`dedup_blank_lines()` when line-ending spelling itself must remain unchanged.

Every cell is transformed independently. Shape, rank, and Text dtype are
preserved, including scalar and higher-rank Notes; the result owns its output
storage and the receiver is unchanged. Empty strings remain empty. The
transform is idempotent: applying it to its own result makes no further
change. Undefined input returns an undefined Note, while a defined non-Text
Note is rejected.

Runtime is linear in the number of bytes. The implementation reserves output
storage from the input size and performs no parsing of TeX commands, making it
appropriate as an early cleanup step before structural extraction.

## Example 1: Apply All Three Repairs

```cpp
const auto rough = pluss::make_note(
    std::vector<std::string>{"alpha  \r\n\r\n \r\n\r\nbeta\t \r",
                             "already\n\nclean", "tail   \n", ""},
    std::array<std::int64_t, 2>{2, 2});
const auto fixed = rough.tex().fix_linebreaks();

std::cout << fixed << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["alpha\n\nbeta\n", "already\n\nclean"],
  ["tail\n", ""]
]
```

The first cell demonstrates all three rules. The second and fourth cells need
no repair, and the third retains its final newline after removing three
trailing spaces.

## Example 2: Verify Idempotence

```cpp
const auto twice = fixed.tex().fix_linebreaks();
std::cout << std::boolalpha << (twice == fixed).all().bool_item() << '\n';
```

### Output

```text
true
```

## Example 3: Undefined And Invalid Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().fix_linebreaks().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().fix_linebreaks();
} catch (const pluss::Error&) {
  std::cout << "linebreaks Int64: rejected\n";
}
```

### Output

```text
false
linebreaks Int64: rejected
```

## See Also

- [`dedup_blank_lines`](dedup_blank_lines.md): cap blank lines while retaining the existing newline convention.
- `line_norm`: normalize CRLF and carriage-return records without compacting blank lines.
- `strip_comments`: remove TeX comments after physical lines have been normalized.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="figures.html"><span class="label">Previous</span><span class="title">figures</span></a>
  <a class="next-link" href="footnotes.html"><span class="label">Next</span><span class="title">footnotes</span></a>
</div>

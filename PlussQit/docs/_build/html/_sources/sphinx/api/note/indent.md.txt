<!-- pluss-hand-authored -->
# Note::indent

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::indent() const
```

## Add A Fixed Indent To Every Line

`indent()` returns a new `Note`, same shape as the input, with two spaces
prepended to the first line of each cell and to every line that follows a
`\n`. Use it to nest a block under a heading, quote a snippet, or format
multi-line content for a list or log entry.

Lines are split on `\n`. Every line gets the indent, including a trailing
empty line after a final `\n`. Nothing else in the cell is changed.

`indent` is the inverse of [`dedent`](dedent.md) for a block that had a
uniform two-space indent. It is shape-preserving, cell-independent, and
returns a fresh `Note`.

## Example 1: A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"line1\nline2", "solo"}).indent())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["  line1\n  line2", "  solo"]
```

## See Also

- [`Note::dedent`](dedent.md): the inverse — strip the common leading
  indent.
- [`Note::ensure_prefix`](ensure_prefix.md): add a prefix to the whole cell
  once, not per line.
- [`Note::surround`](surround.md): wrap the whole cell in a prefix/suffix
  pair.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="html_unescape.html">
    <span class="label">Previous</span>
    <span class="title">html_unescape</span>
  </a>
  <a class="next-link" href="int_item.html">
    <span class="label">Next</span>
    <span class="title">int_item</span>
  </a>
</div>

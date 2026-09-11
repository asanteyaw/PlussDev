<!-- pluss-hand-authored -->
# Note::dedent

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::dedent() const
```

## Remove The Common Leading Indentation From Every Line

`dedent()` returns a new `Note`, same shape as the input, with the largest
run of leading whitespace that **every non-blank line shares** removed from
each line of a cell. It is the standard fix for text captured from an
indented source block — a here-doc, a code snippet, a quoted paragraph — so
the content starts at column zero without disturbing its internal structure.

Lines are split on `\n`. Only whitespace (spaces and tabs) counts as
indentation, and only the *common* amount is stripped: a line indented more
than the others keeps the extra. Blank lines are ignored when measuring the
common indent. If the lines share no leading whitespace, the cell is
returned unchanged.

`dedent` operates within each cell and does not merge or reflow lines. It is
shape-preserving, cell-independent, and returns a fresh `Note`.

## Example 1: A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"    line1\n    line2\n      line3", "no indent"}).dedent())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["line1\nline2\n  line3", "no indent"]
```

Four spaces — the common indent — are removed from every line; `"line3"`,
indented six, keeps the extra two. The second cell has no shared indent and
is unchanged.

## See Also

- [`Note::indent`](indent.md): the inverse — add a fixed indent to every
  line.
- [`Note::strip`](strip.md): trim the whole cell's edges rather than each
  line's.
- [`Note::split_lines`](split_lines.md): work with the lines individually.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count_substring.html">
    <span class="label">Previous</span>
    <span class="title">count_substring</span>
  </a>
  <a class="next-link" href="deduplicate.html">
    <span class="label">Next</span>
    <span class="title">deduplicate</span>
  </a>
</div>

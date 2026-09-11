<!-- pluss-hand-authored -->
# Note::collapse_newlines

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::collapse_newlines() const
```

## Fold Runs Of Line Breaks To A Single Newline

`collapse_newlines()` returns a new `Note`, same shape as the input, with
every run of consecutive `\n` and `\r` characters in each cell replaced by a
**single `\n`**. Use it to squeeze the blank lines out of prose while
keeping paragraph breaks — logs with double-spacing, markdown with stray
blank lines, text glued together from several sources.

Only line-break characters are affected. Spaces, tabs, and everything else
are left exactly as they are, and the cell is **not** trimmed. A `\r\n` pair
collapses to one `\n` like any other run.

`collapse_newlines` is shape-preserving, cell-independent, and returns a
fresh `Note`.

## Example 1: A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"para1\n\n\n\npara2", "a\nb"}).collapse_newlines())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["para1\npara2", "a\nb"]
```

Four blank lines become one break; a single break is unchanged.

## See Also

- [`Note::collapse_spaces`](collapse_spaces.md): the horizontal counterpart.
- [`Note::collapse_whitespace`](collapse_whitespace.md): fold **all**
  whitespace and trim.
- [`Note::split_lines`](split_lines.md): break a cell into its lines.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cell_at.html">
    <span class="label">Previous</span>
    <span class="title">cell_at</span>
  </a>
  <a class="next-link" href="collapse_punct.html">
    <span class="label">Next</span>
    <span class="title">collapse_punct</span>
  </a>
</div>

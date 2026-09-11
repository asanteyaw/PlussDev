<!-- pluss-hand-authored -->
# Note::collapse_spaces

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::collapse_spaces() const
```

## Fold Runs Of Spaces And Tabs To One Space

`collapse_spaces()` returns a new `Note`, same shape as the input, with every
run of one or more spaces or tabs in each cell replaced by a **single
space**. Newlines and carriage returns are **not** touched, and the cell is
**not** trimmed — so multi-line layout and any leading indent survive.

This is the narrower cousin of
[`collapse_whitespace`](collapse_whitespace.md): use `collapse_spaces` when
you want to normalise horizontal spacing inside lines while keeping the line
structure, and `collapse_whitespace` when the whole thing should become one
trimmed line.

`collapse_spaces` changes only horizontal spacing; letters, punctuation, and
non-ASCII are untouched. It is shape-preserving, cell-independent, and
returns a fresh `Note`.

## Example 1: A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"a   b     c", "keep\nthe\nnewlines"}).collapse_spaces())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["a b c", "keep\nthe\nnewlines"]
```

The multi-space runs in the first cell collapse; the newlines in the second
are left alone.

## See Also

- [`Note::collapse_whitespace`](collapse_whitespace.md): also folds newlines
  and trims the edges.
- [`Note::collapse_newlines`](collapse_newlines.md): the vertical
  counterpart.
- [`Note::strip`](strip.md): trim the edges only.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="collapse_repeats.html">
    <span class="label">Previous</span>
    <span class="title">collapse_repeats</span>
  </a>
  <a class="next-link" href="collapse_whitespace.html">
    <span class="label">Next</span>
    <span class="title">collapse_whitespace</span>
  </a>
</div>

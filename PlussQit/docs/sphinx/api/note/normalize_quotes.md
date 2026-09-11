<!-- pluss-hand-authored -->
# Note::normalize_quotes

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::normalize_quotes() const
```

## Convert Curly Quotes To Straight ASCII Quotes

`normalize_quotes()` returns a new `Note`, same shape as the input, with
typographic quotation marks replaced by their straight ASCII equivalents:
the curly double quotes `“ ” „ ‟` all become `"`, and the curly single
quotes / apostrophes `‘ ’ ‚ ‛` all become `'`. Use it to normalise text
pasted from word processors, PDFs, or the web before matching, tokenizing,
or feeding a parser that expects ASCII quotes.

Only quote characters are affected; everything else in the cell is
unchanged. `normalize_quotes` is shape-preserving, cell-independent, and
returns a fresh `Note`.

## Example 1: A Batch

```cpp
const auto n = pluss::make_note(
    {std::string("\xE2\x80\x9C") + "hi" + "\xE2\x80\x9D" + " it" + "\xE2\x80\x99" + "s",
     "plain"}).normalize_quotes();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["\"hi\" it's", "plain"]
```

The curly `“ ”` become `"` and the curly `’` becomes `'`; `print` shows the
straight double quotes escaped as `\"` inside its own quoting.

## See Also

- [`Note::normalize_dashes`](normalize_dashes.md) /
  [`Note::normalize_ellipsis`](normalize_ellipsis.md): the sibling
  punctuation normalisers.
- `pluss::coden::Note::unquote`: strip a surrounding quote pair once the
  quotes are ASCII.
- [`Note::to_nfkc`](to_nfkc.md): a broader compatibility normalisation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="normalize_ellipsis.html">
    <span class="label">Previous</span>
    <span class="title">normalize_ellipsis</span>
  </a>
  <a class="next-link" href="numel.html">
    <span class="label">Next</span>
    <span class="title">numel</span>
  </a>
</div>

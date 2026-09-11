<!-- pluss-hand-authored -->
# Note::normalize_dashes

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::normalize_dashes() const
```

## Convert Typographic Dashes To The ASCII Hyphen-Minus

`normalize_dashes()` returns a new `Note`, same shape as the input, with
en dashes (`–`, U+2013), em dashes (`—`, U+2014), and related dash
characters replaced by the ASCII hyphen-minus `-`. Use it to normalise
ranges, compound words, and punctuation copied from typeset sources before
splitting on `-` or matching against ASCII data.

Only dash characters are affected. `normalize_dashes` is shape-preserving,
cell-independent, and returns a fresh `Note`.

## Example 1: A Batch

```cpp
const auto n = pluss::make_note(
    {std::string("a ") + "\xE2\x80\x93" + " b " + "\xE2\x80\x94" + " c", "plain"})
    .normalize_dashes();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["a - b - c", "plain"]
```

Both the en dash and the em dash become `-`; the surrounding spaces are
untouched.

## See Also

- [`Note::normalize_quotes`](normalize_quotes.md) /
  [`Note::normalize_ellipsis`](normalize_ellipsis.md): the sibling
  punctuation normalisers.
- [`Note::replace`](replace.md): map a specific dash to something other than
  `-`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nlp_remove_urls.html">
    <span class="label">Previous</span>
    <span class="title">nlp_remove_urls</span>
  </a>
  <a class="next-link" href="normalize_ellipsis.html">
    <span class="label">Next</span>
    <span class="title">normalize_ellipsis</span>
  </a>
</div>

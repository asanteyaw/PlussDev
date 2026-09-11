<!-- pluss-hand-authored -->
# Note::normalize_ellipsis

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::normalize_ellipsis() const
```

## Convert The Ellipsis Character To Three ASCII Dots

`normalize_ellipsis()` returns a new `Note`, same shape as the input, with
every horizontal ellipsis character (`…`, U+2026) replaced by three ASCII
periods `...`. Use it to normalise text from typeset sources before a
tokenizer or a display that expects plain ASCII, or so that
[`collapse_punct`](collapse_punct.md) can then thin the run.

Only the ellipsis character is affected; runs of literal periods are left as
they are. `normalize_ellipsis` is shape-preserving, cell-independent, and
returns a fresh `Note`.

## Example 1: A Batch

```cpp
const auto n = pluss::make_note(
    {std::string("wait") + "\xE2\x80\xA6" + " more", "plain"}).normalize_ellipsis();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["wait... more", "plain"]
```

## See Also

- [`Note::normalize_quotes`](normalize_quotes.md) /
  [`Note::normalize_dashes`](normalize_dashes.md): the sibling punctuation
  normalisers.
- [`Note::collapse_punct`](collapse_punct.md): reduce the resulting `...` to
  a single `.`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="normalize_dashes.html">
    <span class="label">Previous</span>
    <span class="title">normalize_dashes</span>
  </a>
  <a class="next-link" href="normalize_quotes.html">
    <span class="label">Next</span>
    <span class="title">normalize_quotes</span>
  </a>
</div>

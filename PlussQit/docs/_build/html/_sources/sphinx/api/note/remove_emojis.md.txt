<!-- pluss-hand-authored -->
# Note::remove_emojis

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::remove_emojis() const
```

## Delete Emoji Characters From Every Cell

`remove_emojis()` returns a new `Note`, same shape as the input, with emoji
characters removed from each cell. Use it to clean user-generated text —
social posts, chat logs, reviews — before language processing or display in
a context that cannot render emoji.

Unlike the ASCII character-class filters, this one is **Unicode-aware**: it
decodes each character and drops it only if its code point is in an emoji
range. Ordinary text, including accented and non-Latin letters, is kept
exactly. Nothing is trimmed or collapsed, so removing an emoji from the
middle of a sentence can leave a double space.

`remove_emojis` is shape-preserving, cell-independent, and returns a fresh
`Note`. A cell that is entirely emoji reduces to `""`.

## Example 1: A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"ship it 🚀 today 🎉", "plain text", "👍👍👍"}).remove_emojis())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["ship it  today ", "plain text", ""]
```

The emoji are gone; the spaces around them remain, and `"👍👍👍"` becomes
empty.

## See Also

- [`Note::keep_ascii`](keep_ascii.md): a blunter clean — drop **all**
  non-ASCII, not just emoji.
- [`NoteNlpAccessor::clean`](../note-nlp/clean.md): a broader user-text cleanup pass.
- [`Note::strip`](strip.md) / `pluss::coden::Note::collapse_whitespace`:
  tidy the spacing left behind.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_digits.html">
    <span class="label">Previous</span>
    <span class="title">remove_digits</span>
  </a>
  <a class="next-link" href="remove_letters.html">
    <span class="label">Next</span>
    <span class="title">remove_letters</span>
  </a>
</div>

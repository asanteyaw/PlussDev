<!-- pluss-hand-authored -->
# Note::keep_ascii

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::keep_ascii() const
```

## Drop Every Non-ASCII Byte From Every Cell

`keep_ascii()` returns a new `Note`, same shape as the input, with every byte
`>= 0x80` deleted from each cell. What remains is exactly the ASCII content —
letters, digits, whitespace, punctuation, and ASCII control bytes are all
kept; accented letters, non-Latin scripts, and emoji are removed.

This is a **lossy** coercion, not a transliteration: `"café"` becomes
`"caf"`, not `"cafe"`. When you want accented letters folded to their base
ASCII forms, use `transliterate_ascii()` (or `to_ascii()`) instead; use
`keep_ascii()` only when non-ASCII content is genuinely unwanted.

`keep_ascii` is shape-preserving, cell-independent, and returns a fresh
`Note`.

## Example 1: A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"café résumé", "plain", "日本語 text"}).keep_ascii())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["caf rsum", "plain", " text"]
```

The accented letters and the CJK characters are dropped; the ASCII spaces
between them stay.

## See Also

- `pluss::coden::Note::transliterate_ascii`: fold non-ASCII to ASCII
  equivalents instead of deleting them.
- [`Note::is_ascii`](is_ascii.md): test whether a cell is already pure
  ASCII.
- [`Note::keep_printable`](keep_printable.md): also drop ASCII control
  bytes.
- [`Note::remove_emojis`](remove_emojis.md): drop only emoji, keep other
  non-ASCII text.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="keep_alpha.html">
    <span class="label">Previous</span>
    <span class="title">keep_alpha</span>
  </a>
  <a class="next-link" href="keep_digits.html">
    <span class="label">Next</span>
    <span class="title">keep_digits</span>
  </a>
</div>

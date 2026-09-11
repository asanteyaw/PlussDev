<!-- pluss-hand-authored -->
# Note::to_ascii

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::to_ascii() const
```

## Replace Every Non-ASCII Character With `?`

`to_ascii()` returns a new `Note`, same shape as the input, in which ASCII
bytes pass through and each non-ASCII **character** is replaced by a single
`?`. Use it when a downstream system is strictly ASCII and you want a
visible, fixed-width marker for every character that could not be
represented, rather than silently dropping it.

The replacement is per character, not per byte: a two- or three-byte UTF-8
character becomes one `?`, so `"café"` is four characters and yields
`"caf?"`. This is a **lossy** conversion and not a transliteration — it does
not know that `é` "means" `e`. When you want accented letters folded to
their base ASCII forms (`café` → `cafe`), use `transliterate_ascii()`; when
you want non-ASCII simply removed with no marker, use
[`keep_ascii`](keep_ascii.md).

`to_ascii` is shape-preserving, cell-independent, and returns a fresh
`Note`.

## Example 1: A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"café résumé", "naïve", "日本語", "plain"}).to_ascii())
    << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["caf? r?sum?", "na?ve", "???", "plain"]
```

Each accented letter and each CJK character becomes one `?`; `"plain"` is
untouched.

## See Also

- `pluss::coden::Note::transliterate_ascii`: fold non-ASCII to ASCII
  equivalents instead of marking them.
- [`Note::keep_ascii`](keep_ascii.md): drop non-ASCII with no replacement
  marker.
- [`Note::is_ascii`](is_ascii.md): test whether a cell is already pure
  ASCII.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="title.html">
    <span class="label">Previous</span>
    <span class="title">title</span>
  </a>
  <a class="next-link" href="to_latin1_lossy.html">
    <span class="label">Next</span>
    <span class="title">to_latin1_lossy</span>
  </a>
</div>

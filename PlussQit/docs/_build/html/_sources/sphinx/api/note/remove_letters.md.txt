<!-- pluss-hand-authored -->
# Note::remove_letters

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::remove_letters() const
```

## Delete ASCII Letters From Every Cell

`remove_letters()` returns a new `Note`, same shape as the input, with every
ASCII letter (`A`–`Z`, `a`–`z`) deleted from each cell. It leaves the
numeric, punctuation, whitespace, and non-ASCII skeleton — useful for
extracting the shape of a coded value, or for measuring how "numeric" a
field is.

The removal is **ASCII-only**: a non-ASCII letter such as `é` or `本` is not
an ASCII letter, so its bytes are kept. Nothing is trimmed or collapsed.

`remove_letters` is shape-preserving, cell-independent, and returns a fresh
`Note`.

## Example 1: A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"Order 17!", "café", "42.0"}).remove_letters())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
[" 17!", "é", "42.0"]
```

`"café"` keeps only the non-ASCII `é`; `"42.0"` has no letters and is
unchanged.

## See Also

- [`Note::keep_alpha`](keep_alpha.md): the complement — keep **only** the
  ASCII letters.
- [`Note::remove_digits`](remove_digits.md) /
  [`Note::remove_punctuation`](remove_punctuation.md): drop other classes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_emojis.html">
    <span class="label">Previous</span>
    <span class="title">remove_emojis</span>
  </a>
  <a class="next-link" href="remove_prefix.html">
    <span class="label">Next</span>
    <span class="title">remove_prefix</span>
  </a>
</div>

<!-- pluss-hand-authored -->
# Note::remove_symbols

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::remove_symbols() const
```

## Delete ASCII Punctuation And Control Bytes From Every Cell

`remove_symbols()` returns a new `Note`, same shape as the input, keeping
only ASCII letters, ASCII digits, ASCII whitespace, and **every non-ASCII
byte**. Everything else — ASCII punctuation (`!`, `#`, `%`, `_`, `-`, `.`, …)
**and** ASCII control bytes (`\t`, `\n`, other C0 controls) — is deleted.

It is very close to [`remove_punctuation`](remove_punctuation.md); the extra
step is that `remove_symbols` also strips ASCII control characters. Both keep
non-ASCII text intact, so accented and non-Latin words survive. If you want
to drop non-ASCII too, use [`keep_alnum`](keep_alnum.md) (which also drops
whitespace) or [`keep_ascii`](keep_ascii.md) first.

`remove_symbols` is shape-preserving, cell-independent, and returns a fresh
`Note`. Nothing is trimmed or collapsed.

## Example 1: A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"100% (sure)!", "a_b-c", "café #9"}).remove_symbols())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["100 sure", "abc", "café 9"]
```

## Example 2: Non-ASCII Text Is Kept

```cpp
const pluss::Note n = pluss::make_note({"café — résumé"}).remove_symbols();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["café — résumé"]
```

The em dash and the accented letters are non-ASCII, so they stay; only
ASCII punctuation/controls would have been removed.

## See Also

- [`Note::remove_punctuation`](remove_punctuation.md): the same idea without
  also stripping ASCII control bytes.
- [`Note::keep_alnum`](keep_alnum.md): keep only letters and digits (drops
  whitespace and non-ASCII).
- [`Note::keep_printable`](keep_printable.md): keep only visible ASCII.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_suffix.html">
    <span class="label">Previous</span>
    <span class="title">remove_suffix</span>
  </a>
  <a class="next-link" href="repeat.html">
    <span class="label">Next</span>
    <span class="title">repeat</span>
  </a>
</div>

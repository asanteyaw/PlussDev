<!-- pluss-hand-authored -->
# Note::keep_alpha

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::keep_alpha() const
```

## Keep Only ASCII Letters In Every Cell

`keep_alpha()` returns a new `Note`, same shape as the input, with every
non-letter deleted from each cell — digits, whitespace, punctuation, and all
non-ASCII bytes. What remains is a pure run of ASCII letters. Use it to
reduce a value to its "word" content for a loose match or a phonetic key.

The scope is **ASCII letters only** (`A`–`Z`, `a`–`z`). A non-ASCII letter is
not kept, so `"café"` becomes `"caf"`. Word boundaries are lost — `"A B C"`
becomes `"ABC"`.

`keep_alpha` is shape-preserving, cell-independent, and returns a fresh
`Note`. A cell with no ASCII letters becomes `""`.

## Example 1: A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"abc123", "A B C", "café", "42"}).keep_alpha())
    << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["abc", "ABC", "caf", ""]
```

`"42"` has no letters and reduces to `""`.

## See Also

- [`Note::keep_alnum`](keep_alnum.md): also keep digits.
- [`Note::remove_letters`](remove_letters.md): the complement.
- `pluss::coden::Note::transliterate_ascii`: fold `é` to `e` first if you
  want accented letters to survive as ASCII.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="keep_alnum.html">
    <span class="label">Previous</span>
    <span class="title">keep_alnum</span>
  </a>
  <a class="next-link" href="keep_ascii.html">
    <span class="label">Next</span>
    <span class="title">keep_ascii</span>
  </a>
</div>

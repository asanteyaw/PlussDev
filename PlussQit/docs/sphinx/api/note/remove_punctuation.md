<!-- pluss-hand-authored -->
# Note::remove_punctuation

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::remove_punctuation() const
```

## Delete ASCII Punctuation From Every Cell

`remove_punctuation()` returns a new `Note`, same shape as the input, with
every ASCII punctuation character deleted from each cell. Use it as a
normalization step before tokenization, hashing, or comparison, when marks
like `,`, `.`, `!`, `-`, `_`, `#`, `%` are noise.

The characters removed are the ASCII punctuation class — the visible ASCII
symbols that are not letters or digits (`!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~`).
Everything else is **kept**: letters, digits, whitespace, and — importantly
— **all non-ASCII bytes**, so accented and non-Latin text (`café`, `日本語`)
passes through unchanged. Nothing is trimmed or collapsed, so removing a
mid-string mark can leave the surrounding spaces.

`remove_punctuation` is shape-preserving, cell-independent, and returns a
fresh `Note`. Contrast [`remove_symbols`](remove_symbols.md), which also
strips ASCII control bytes but likewise keeps non-ASCII.

## Example 1: A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"Hello, World! 42", "a_b-c.d", "café #99"}).remove_punctuation())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Hello World 42", "abcd", "café 99"]
```

Punctuation is gone; letters, digits, spaces, and the non-ASCII `é` remain.

## Example 2: Whitespace And Shape Are Preserved

```cpp
const pluss::Note n = pluss::make_note({{"a, b", "c.d"}}).remove_punctuation();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[1, 2], dim=2, numel=2, encoding=utf8)
[
  ["a b", "cd"]
]
```

`"a, b"` keeps its space; `"c.d"` becomes `"cd"`.

## See Also

- [`Note::remove_symbols`](remove_symbols.md): also removes ASCII control
  bytes.
- [`Note::keep_alnum`](keep_alnum.md): keep **only** letters and digits
  (drops whitespace and non-ASCII too).
- [`Note::remove_all`](remove_all.md): delete a specific literal string.
- [`Note::strip`](strip.md): remove characters at the boundaries only.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_prefix.html">
    <span class="label">Previous</span>
    <span class="title">remove_prefix</span>
  </a>
  <a class="next-link" href="remove_suffix.html">
    <span class="label">Next</span>
    <span class="title">remove_suffix</span>
  </a>
</div>

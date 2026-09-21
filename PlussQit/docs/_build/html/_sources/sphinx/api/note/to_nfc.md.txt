<!-- pluss-hand-authored -->
# Note::to_nfc

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::to_nfc() const
```

## Compose Every Cell To Unicode Normalization Form C

`to_nfc()` returns a new `Note`, same shape as the input, with each cell
converted to **NFC** — the canonical composed form. Where a base letter and
its combining marks can be written as one precomposed character, NFC uses
the single character: `e` + U+0301 (combining acute) becomes the single code
point `é` (U+00E9).

NFC is the form the web and most systems expect, and the one to normalise to
before storing text, comparing strings byte-for-byte, or computing a hash —
two visually identical strings that differ only in composition compare equal
after both are put through `to_nfc()`. It does **not** change how the text
renders, only its bytes, so pair it with
[`lengths_bytes`](lengths_bytes.md) or [`lengths_chars`](lengths_chars.md)
to see the effect.

`to_nfc` is shape-preserving, cell-independent, idempotent,
and returns a fresh `Note`. Requires the Unicode backend.

## Example 1: A Decomposed Accent Is Composed

```cpp
const std::string decomposed = std::string("caf") + "e\xCC\x81";  // "e" + U+0301
const pluss::Note n = pluss::make_note({decomposed, "abc"}).to_nfc();
std::cout << pluss::note::print(n) << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["café", "abc"]
```

The render is unchanged; the bytes are not.

## Example 2: The Byte Count Shows The Change

```cpp
const std::string decomposed = std::string("caf") + "e\xCC\x81";
const auto in = pluss::make_note({decomposed, "abc"});

const auto before = in.lengths_bytes();
const auto after = in.to_nfc().lengths_bytes();
std::cout << "[" << before[0] << ", " << before[1] << "] -> ["
          << after[0] << ", " << after[1] << "]\n";
```

### Output

```text
[6, 3] -> [5, 3]
```

`"café"` drops from 6 bytes (`c a f e` + 2-byte combining mark) to 5 (`c a f`
+ 2-byte precomposed `é`); the ASCII cell is untouched.

## See Also

- [`Note::to_nfd`](to_nfd.md): the opposite direction — canonical
  decomposition.
- [`Note::to_nfkc`](to_nfkc.md): compose **and** apply compatibility folds.
- [`Note::casefold`](casefold.md): the case half of a comparison key; run
  both.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_latin1_lossy.html">
    <span class="label">Previous</span>
    <span class="title">to_latin1_lossy</span>
  </a>
  <a class="next-link" href="to_nfd.html">
    <span class="label">Next</span>
    <span class="title">to_nfd</span>
  </a>
</div>

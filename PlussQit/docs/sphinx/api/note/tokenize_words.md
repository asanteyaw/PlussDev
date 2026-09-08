<!-- pluss-hand-authored -->
# Note::tokenize_words

[Back to Note API Reference](../../note_api_reference)

## Signatures

```cpp
TokenBatch Note::tokenize_words() const
TokenBatch Note::tokenize_words(std::int64_t flags) const
```

## Tokenize Each Cell Into Word-Like Units

`tokenize_words()` splits each text cell into word tokens and returns them as
a `TokenBatch`. It sits one step above [`split_whitespace`](split_whitespace.md):
instead of "everything between spaces," it recognizes word shapes and drops
the punctuation between them, so `"One.\nTwo!"` tokenizes to `One`, `Two` —
no trailing `.` or `!`.

### What counts as a word

The tokenizer is **ASCII-oriented**. A word is a run of ASCII letters and
digits, with a few extensions:

- `_` is part of a word (`snake_case` is one token).
- A single `-` or `'` **between** two alphanumeric characters stays inside
  the word (`state-of-the-art`, `don't`).
- Trailing `+` or `#` after a letter is kept (`C++`, `C#`).
- URLs, email addresses, and common dotted abbreviations are recognized and
  kept whole rather than shattered on their `.` and `/`.

Everything else — spaces, standalone punctuation, and **every byte outside
ASCII** — is a separator. That last point matters: a non-ASCII letter breaks
the word around it, so `"café2"` tokenizes to `caf`, `2` and `"über-cool"` to
`ber-cool`. For Unicode-aware segmentation, use `regex_split` /
[`regex_findall`](regex_findall.md) with an explicit pattern, or normalize
the text first.

### `flags`

`flags` is a bitmask; `flags <= 0` (the default) leaves tokens as they appear
in the source. Bit `1 << 0` (value `1`) case-folds every emitted token
(ASCII lowercase) without changing the token boundaries — convenient for
building a bag-of-words key in one call instead of chaining
[`lower`](lower.md).

### The `TokenBatch` result

Tokens from all cells are concatenated into one flat rank-`1` token `Note`
(`batch.tokens()`); `batch.offsets()` (length `numel + 1`) marks each cell's
run: cell `r` is `tokens()[offsets()[r] .. offsets()[r+1])`. A cell with no
word contributes an empty range.

## Example 1: Punctuation Is Dropped

```cpp
const auto batch =
    pluss::make_note({"One.\nTwo!", "price=12.50, paid!"}).tokenize_words();

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[6], dim=1, numel=6, encoding=utf8)
["One", "Two", "price", "12", "50", "paid"]
offsets: [0, 2, 6]
```

The `.`, `!`, `=`, and `,` are all separators; `12.50` splits at the `.`.

## Example 2: The Case-Fold Flag

```cpp
const auto batch =
    pluss::make_note({"Ada LOVELACE", "Grace Hopper"}).tokenize_words(1);
std::cout << pluss::note::print(batch.tokens()) << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["ada", "lovelace", "grace", "hopper"]
```

## Example 3: Without The Flag

```cpp
const auto batch = pluss::make_note({"Ada LOVELACE"}).tokenize_words();
std::cout << pluss::note::print(batch.tokens()) << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["Ada", "LOVELACE"]
```

## Example 4: Per-Cell Runs

```cpp
const auto batch = pluss::make_note({{"a b", "c d e"}}).tokenize_words();

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[5], dim=1, numel=5, encoding=utf8)
["a", "b", "c", "d", "e"]
offsets: [0, 2, 5]
```

The `1×2` input flattens row-major: cell `0` → `[0, 2)`, cell `1` →
`[2, 5)`.

## See Also

- [`Note::split_whitespace`](split_whitespace.md): faster, purely
  whitespace-delimited, keeps punctuation attached.
- [`Note::regex_findall`](regex_findall.md) / `regex_split`: define your own
  token pattern, including Unicode ranges.
- `pluss::coden::Note::tokenize_sentences` / `tokenize_punct`: sibling
  tokenizers for other units.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="title.html">
    <span class="label">Previous</span>
    <span class="title">title</span>
  </a>
  <a class="next-link" href="unquote.html">
    <span class="label">Next</span>
    <span class="title">unquote</span>
  </a>
</div>

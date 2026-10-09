<!-- pluss-hand-authored -->
# Note::NlpAccessor::normalize_punct

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note Note::NlpAccessor::normalize_punct(bool ascii_only = false) const
```

## Canonicalize Common Typographic Punctuation

`normalize_punct()` reduces visually equivalent punctuation to stable byte
sequences before matching, tokenization, hashing, or equality checks. It
combines three focused transformations in one public operation:

| Input family | Recognized characters | Output |
|---|---|---|
| Single quotes | `‘ ’ ‚ ‛` | `'` |
| Double quotes | `“ ” „ ‟` | `"` |
| Dashes | hyphen, non-breaking hyphen, figure dash, en dash, em dash, horizontal bar, minus sign | `-` |
| Ellipsis | `…` | `...` |

ASCII punctuation already in canonical form is unchanged. The method does not
collapse repeated ASCII punctuation, remove punctuation, normalize whitespace,
or infer paired quotation structure.

The overloads differ only in their treatment of all remaining non-ASCII
characters:

| Call form | `ascii_only` | Remaining non-ASCII text |
|---|---:|---|
| `normalize_punct()` | `false` | Preserved as UTF-8. |
| `normalize_punct(false)` | `false` | Identical to the no-argument form. |
| `normalize_punct(true)` | `true` | Each UTF-8 code point becomes one `?`. |

The `true` mode is intentionally lossy ASCII enforcement, not transliteration.
For example, the single code point `ï` becomes one question mark rather than
`i`. Typographic quotes and dashes are converted before that enforcement, so
they retain their meaningful ASCII equivalents instead of becoming `?`.

Each cell is independent, and both overloads allocate a new shape-identical
Text Note without mutating the receiver. The operation is idempotent in either
mode. Empty Text Notes remain empty, undefined propagates, and Bytes, Bool,
Int64, and Float64 Notes reject.

## Example 1: Preserve Non-ASCII Vocabulary By Default

```cpp
const auto punct = pluss::note::make({"“naïve”—wait…"});
std::cout << pluss::note::print(
    punct.nlp().normalize_punct()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["\"naïve\"-wait..."]
```

## Example 2: Spell Out The Default Explicitly

```cpp
std::cout << pluss::note::print(
    punct.nlp().normalize_punct(false)) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["\"naïve\"-wait..."]
```

## Example 3: Request Lossy ASCII Output

```cpp
std::cout << pluss::note::print(
    punct.nlp().normalize_punct(true)) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["\"na?ve\"-wait..."]
```

## Example 4: Punctuation Normalization Does Not Clean Other Content

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{" can't ", "12", "“yes”", "plain"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << pluss::note::print(
    matrix.nlp().normalize_punct()) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  [" can't ", "12"], 
  ["\"yes\"", "plain"]
]
```

The spaces, contraction, digits, and case remain because this method changes
only its documented punctuation families.

## See Also

- [`normalize_quotes`](../note/normalize_quotes.md): normalize only quote characters.
- [`normalize_dashes`](../note/normalize_dashes.md): normalize only dash characters.
- [`normalize_ellipsis`](../note/normalize_ellipsis.md): normalize only the ellipsis code point.
- [`clean`](clean.md): add control, zero-width, and whitespace cleanup.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="normalize_numbers.html"><span class="label">Previous</span><span class="title">normalize_numbers</span></a>
  <a class="next-link" href="numbers.html"><span class="label">Next</span><span class="title">numbers</span></a>
</div>

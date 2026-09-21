<!-- pluss-hand-authored -->
# NoteNlpAccessor::clean

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::clean() const
```

## Establish A Conservative Text Baseline

`clean()` prepares untidy prose for later text operations without erasing
its words, numbers, letter case, or non-ASCII vocabulary. It is the convenient
choice when input has passed through editors, copied documents, or transport
systems and several low-level artifacts need the same standard treatment.

The method performs these transformations in order:

| Stage | Effect |
|---|---|
| Punctuation normalization | Curly quotes become straight quotes, dash variants become `-`, and the ellipsis character becomes `...`. |
| Control removal | C0 controls and `DEL` are removed, except tab, carriage return, and line feed, which proceed to whitespace normalization. |
| Zero-width removal | Known zero-width Unicode formatting characters are deleted. |
| Whitespace normalization | Runs of ordinary spaces, tabs, carriage returns, and line feeds collapse to one ordinary space. Form feed and vertical tab were already removed as controls. |
| Edge stripping | Leading and trailing ASCII whitespace is removed. |

This is deliberately a conservative pipeline. It does **not** lowercase text,
expand contractions, normalize numbers, remove accents, erase punctuation, or
perform general Unicode canonicalization. Those choices can change lexical
meaning and therefore remain explicit operations. For example, `CAN'T` and
`1,250` survive cleaning unchanged.

Each cell is processed independently. The result is a newly allocated Text
`Note` with the same rank, extents, and row-major cell order as the receiver;
the receiver is unchanged. An empty Text Note remains empty with its shape
intact. An undefined receiver propagates undefined. Bytes, Bool, Int64, and
Float64 Notes are rejected before any cells are transformed.

The operation is idempotent: once its five invariants hold, applying it again
does not alter the text. Use the individual punctuation or whitespace methods
when only one stage is wanted, because `clean()` always applies the whole
pipeline.

## Example 1: Remove Several Kinds Of Transport Noise

```cpp
const std::string dirty =
    " \t\xE2\x80\x9CShip\xE2\x80\x9D\xE2\x80\x94now\xE2\x80\xA6"
    "\xE2\x80\x8B\nnext\x01 ";
std::cout << pluss::note::print(
    pluss::note::make({dirty}).nlp().clean()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["\"Ship\"-now... next"]
```

The curly punctuation, zero-width space, line break, and control byte each
exercise a different stage. The visible words remain.

## Example 2: See What Cleaning Intentionally Retains

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"  I CAN'T pay 1,250  "}).nlp().clean()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["I CAN'T pay 1,250"]
```

Only the edge spaces are cleanup artifacts. Case, apostrophe, comma, and
digits are content, so this method leaves them alone.

## Example 3: Preserve A Matrix Rather Than Flattening It

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{" can't ", "12", "“yes”", "plain"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << pluss::note::print(matrix.nlp().clean()) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["can't", "12"], 
  ["\"yes\"", "plain"]
]
```

## Example 4: Idempotence, Empty Input, Absence, And Dtype Failure

```cpp
const auto once = matrix.nlp().clean();
std::cout << "clean idempotent=" << std::boolalpha
          << (once.nlp().clean() == once).all().bool_item() << '\n';
std::cout << pluss::note::print(
    pluss::note::make(std::vector<std::string>{}).nlp().clean()) << '\n';
std::cout << "undefined defined="
          << pluss::Note::undefined().nlp().clean().defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"text"},
      std::array<std::int64_t, 1>{1}).nlp().clean();
} catch (const pluss::Error&) {
  std::cout << "Bytes: rejected\n";
}
```

### Output

```text
clean idempotent=true
Note(shape=[0], dim=1, numel=0, encoding=utf8)
[]
undefined defined=false
Bytes: rejected
```

## See Also

- [`expand_contractions`](expand_contractions.md): expand a defined set of English contractions without performing general cleanup.
- [`normalize_punct`](normalize_punct.md): run only the punctuation stage, optionally forcing ASCII output.
- [`collapse_whitespace`](../note/collapse_whitespace.md): normalize whitespace without changing punctuation or controls.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="citations.html"><span class="label">Previous</span><span class="title">citations</span></a>
  <a class="next-link" href="definitions.html"><span class="label">Next</span><span class="title">definitions</span></a>
</div>

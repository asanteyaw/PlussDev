<!-- pluss-hand-authored -->

# DataFrameNoteAccessor::normalize

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signature

```cpp
DataFrameNoteAccessor::normalize(UnicodeForm form = UnicodeForm::NFC) const -> DataFrame
```

## What It Does

Applies Unicode text normalization to every selected feature's cells,
choosing one of the four standard Unicode Normalization Forms via `form`:

```cpp
enum class UnicodeForm {
  NFC = 0,   // Canonical Decomposition, followed by Canonical Composition
  NFD = 1,   // Canonical Decomposition
  NFKC = 2,  // Compatibility Decomposition, followed by Canonical Composition
  NFKD = 3,  // Compatibility Decomposition
};
```

Normalization matters for text that might contain the same visible
character represented in more than one way — most commonly, a precomposed
accented letter (`é` as a single codepoint) versus a base letter plus a
combining mark (`e` + a combining acute accent, two codepoints that render
identically). Two byte-for-byte-different strings can look identical and
still compare unequal with a plain [`eq()`](../dataframe/eq.md); running
both through the same normalization form first makes such comparisons
consistent.

```{important} Requires the optional Unicode backend (GNU libunistring) for anything beyond plain ASCII
`normalize()`'s real behavior depends entirely on whether this build links
libunistring:
- **With the backend**: all four forms are fully implemented via the
  standard `uninorm_normalize` routine, correctly handling precomposed and
  decomposed Unicode text.
- **Without the backend**: `normalize()` still accepts pure-ASCII input —
  and, since plain ASCII has no combining marks, ligatures, or
  compatibility characters to normalize in the first place, every form
  behaves as an identity transform and returns the input unchanged (see
  Example 1). But any **non-ASCII** input throws
  `"Note::normalize: non-ASCII text requires a Unicode backend"` — there is
  no partial or best-effort fallback for non-ASCII text.

Confirm this build's Unicode support before relying on `normalize()` for
any text that might contain non-ASCII characters.
```

## Parameters

**form** (`UnicodeForm`, default `UnicodeForm::NFC`)
    Which normalization form to apply: `NFC`, `NFD`, `NFKC`, or `NFKD`.

## Returns

`DataFrame`
    A new frame with every selected feature's text normalized to the
    requested form.

## Examples

Example 1. Plain ASCII text under the default form (`NFC`) — a no-op,
since ASCII has nothing to normalize, and this behavior holds regardless
of whether this build links the Unicode backend.

```cpp
using namespace pluss::table;

auto titles = DataFrame({
    {"title", Thread(pluss::make_note(
        {"Hello World", "ASCII text only", "x"}),
        pluss::make_series(std::vector<bool>{true, true, false}))},
});
std::cout << titles;

auto out1 = titles.nt().normalize();
std::cout << out1;
```

### Output (before, identical to output after)

```text
======================
     | title          
----------------------
0    | Hello World    
1    | ASCII text only
2    | NULL           
----------------------
[3 cs x 1 feat]
```

The output is byte-for-byte identical to the input: `NFC` (like every
other form, on ASCII) has nothing to canonicalize when there are no
combining marks or compatibility characters present. The invalid third row
stays `NULL`, same as every other method on this accessor.

Example 2. Explicitly requesting each of the four forms on the same ASCII
input — all four produce the identical result, since the distinctions
between NFC/NFD/NFKC/NFKD only matter for text containing decomposable or
compatibility characters, none of which plain ASCII has.

```cpp
auto nfc = titles.nt().normalize(UnicodeForm::NFC);
auto nfd = titles.nt().normalize(UnicodeForm::NFD);
auto nfkc = titles.nt().normalize(UnicodeForm::NFKC);
auto nfkd = titles.nt().normalize(UnicodeForm::NFKD);
// nfc, nfd, nfkc, and nfkd are all identical to titles here.
```

Example 3. Non-ASCII input without the Unicode backend throws — shown here
as the documented failure mode rather than a specific transformed value,
since the exact behavior depends on this build's optional dependency (see
the important callout above).

```cpp
auto accented = DataFrame({
    {"word", Thread(pluss::make_note({"caf\xC3\xA9"}))},  // "café"
});

try {
  auto out3 = accented.nt().normalize();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output (build without the Unicode backend)

```text
Note::normalize: non-ASCII text requires a Unicode backend
```

A build with libunistring linked would instead perform genuine Unicode
normalization on this input rather than throwing — but the exact
byte-level output in that case isn't asserted here, since it depends on a
build-time optional dependency rather than anything guaranteed by the
public API alone.

## Notes

- All four `UnicodeForm` values are identity transforms on pure ASCII
  text, with or without the Unicode backend.
- Non-ASCII input throws outright when this build lacks libunistring —
  there is no partial fallback. See the important callout above.
- Applies to every feature the accessor was built with, in one call — see
  [`strip()`](strip.md)'s Notes for the mechanics shared by every method on
  this accessor.
- A null cell on input stays null on output.

## See Also

- [`DataFrameNoteAccessor::casefold`](casefold.md): a related
  build-dependent Unicode operation, with the same libunistring caveat.
- [`DataFrame::eq`](../dataframe/eq.md): the comparison operation
  normalization is usually a preprocessing step for.
- [`DataFrameNoteAccessor::encode`](encode.md): byte-level encodings, unlike
  `normalize()`'s codepoint-level text transform.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="matches.html">
    <span class="label">Previous</span>
    <span class="title">matches</span>
  </a>
  <a class="next-link" href="pad.html">
    <span class="label">Next</span>
    <span class="title">pad</span>
  </a>
</div>

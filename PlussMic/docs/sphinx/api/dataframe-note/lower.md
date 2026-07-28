<!-- pluss-hand-authored -->

# DataFrameNoteAccessor::lower

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signature

```cpp
DataFrameNoteAccessor::lower() const -> DataFrame
```

## What It Does

Lowercases every character in every selected text feature's cells, returning
a new frame — the classic normalization step before a case-insensitive
comparison, a `group()` key that shouldn't be split by casing accidents,
or a lookup against a canonical lowercase reference list.

Like every method on this accessor, `lower()` applies to **every feature the
accessor was built with** in one call — see [`strip()`](strip.md) for the
full explanation of that shared mechanic (including the two ways to reach
the accessor) and how null cells pass through unaffected.

```{note} ASCII vs. Unicode case mapping
On plain ASCII text, `lower()` behaves exactly as you'd expect: `A`-`Z`
becomes `a`-`z`, everything else is untouched. Non-ASCII (accented Latin,
Cyrillic, Greek, and so on) case mapping depends on whether this build was
linked against the optional Unicode backend (GNU libunistring); when it
isn't, `lower()` only recognizes ASCII letters and leaves non-ASCII bytes as
they are (or, depending on build configuration, rejects them). Confirm your
build's Unicode support before relying on `lower()` for non-Latin or
accented text.
```

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with every selected feature's text lowercased; every
    non-alphabetic character (digits, punctuation, whitespace) is left
    exactly as it was.

## Examples

Example 1. Lowercase two text features in one call, with a null cell
preserved.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"city", Thread(
        pluss::make_note({"Accra", "TEMA", "Kumasi"}),
        pluss::make_series(std::vector<bool>{true, true, false}))},
    {"code", Thread(pluss::make_note({"GH-001", "Gh-002", "gh-003"}))},
}, Thread(pluss::make_note({"p1", "p2", "p3"})));
std::cout << orders;

auto out = orders.nt().lower();
std::cout << out;
```

### Output (before)

```text
====================
     | city   code  
--------------------
p1   | Accra  GH-001
p2   | TEMA   Gh-002
p3   | NULL   gh-003
--------------------
[3 cs x 2 feat]
```

### Output (after)

```text
====================
     | city   code  
--------------------
p1   | accra  gh-001
p2   | tema   gh-002
p3   | NULL   gh-003
--------------------
[3 cs x 2 feat]
```

Both `city` and `code` are lowercased in the same call — every selected
feature participates, not just one. `p3`'s `city` cell was constructed
invalid; it stays `NULL` rather than becoming an empty string or the literal
text `"kumasi"` that happened to sit underneath it. `code`'s hyphen and
digits are untouched throughout, since `lower()` only ever changes letters.

## Notes

- Applies to every feature the accessor was built with, in one call — see
  [`strip()`](strip.md)'s Notes for the mechanics shared by every method on
  this accessor.
- A null cell on input stays null on output.
- For the opposite transform, see [`upper()`](upper.md). For a stricter,
  Unicode-aware case-insensitive comparison basis, see
  [`casefold()`](casefold.md).

## See Also

- [`DataFrameNoteAccessor::upper`](upper.md): the inverse transform.
- [`DataFrameNoteAccessor::casefold`](casefold.md): case folding intended for
  comparison rather than display, and how it differs from `lower()`.
- [`DataFrameNoteAccessor::titlecase`](titlecase.md): capitalize the first
  letter of each word instead of lowercasing everything.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="length.html">
    <span class="label">Previous</span>
    <span class="title">length</span>
  </a>
  <a class="next-link" href="lstrip.html">
    <span class="label">Next</span>
    <span class="title">lstrip</span>
  </a>
</div>

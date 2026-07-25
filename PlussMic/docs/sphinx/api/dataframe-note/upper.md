<!-- pluss-hand-authored -->

# DataFrameNoteAccessor::upper

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signature

```cpp
DataFrameNoteAccessor::upper() const -> DataFrame
```

## What It Does

Uppercases every character in every selected text feature's cells, returning
a new frame — the mirror image of [`lower()`](lower.md). Common uses include
normalizing free-typed codes or abbreviations (`"gh"`, `"Gh"`, `"GH"` all
becoming `"GH"`) before comparing or grouping by them, or matching a display
convention that expects codes in all caps.

Like every method on this accessor, `upper()` applies to **every feature the
accessor was built with** in one call — see [`strip()`](strip.md) for the
full explanation of that shared mechanic and how null cells pass through
unaffected.

```{note} ASCII vs. Unicode case mapping
Exactly the same caveat as [`lower()`](lower.md): on ASCII text, `A`-`Z`
mapping is unambiguous and always available; correct handling of accented or
non-Latin uppercase mapping depends on whether this build links the optional
Unicode backend (GNU libunistring).
```

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with every selected feature's text uppercased; every
    non-alphabetic character is left exactly as it was.

## Examples

Example 1. Uppercase two text features in one call, with a null cell
preserved.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"city", Thread(
        pluss::make_note({"Accra", "tema", "Kumasi"}),
        pluss::make_series(std::vector<bool>{true, true, false}))},
    {"code", Thread(pluss::make_note({"gh-001", "Gh-002", "GH-003"}))},
}, Thread(pluss::make_note({"p1", "p2", "p3"})));

auto out = orders.note().upper();
std::cout << out;
```

### Output

```text
====================
     | city   code  
--------------------
p1   | ACCRA  GH-001
p2   | TEMA   GH-002
p3   | NULL   GH-003
--------------------
[3 cs x 2 feat]
```

Both `city` and `code` are uppercased in the same call. `p3`'s `city` cell was
constructed invalid; it stays `NULL` rather than becoming `"KUMASI"`. The
hyphens and digits in `code` are untouched throughout, since `upper()` only
ever changes letters — exactly the same guarantee [`lower()`](lower.md)
makes, in the opposite direction.

## Notes

- Applies to every feature the accessor was built with, in one call — see
  [`strip()`](strip.md)'s Notes for the mechanics shared by every method on
  this accessor.
- A null cell on input stays null on output.
- For the opposite transform, see [`lower()`](lower.md).

## See Also

- [`DataFrameNoteAccessor::lower`](lower.md): the inverse transform.
- [`DataFrameNoteAccessor::casefold`](casefold.md): case folding intended for
  comparison rather than display.
- [`DataFrameNoteAccessor::titlecase`](titlecase.md): capitalize just the
  first letter of each word instead of every letter.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="titlecase.html">
    <span class="label">Previous</span>
    <span class="title">titlecase</span>
  </a>
</div>

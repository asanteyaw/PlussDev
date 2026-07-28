<!-- pluss-hand-authored -->

# DataFrameNoteAccessor::length

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signature

```cpp
DataFrameNoteAccessor::length() const -> DataFrame
```

## What It Does

Replaces every selected text feature with its character count, as an `Int64`
`Series` — same feature name, same row order, but the feature's payload kind
changes from `Note` to `Series`. Unlike most of this accessor's methods,
`length()` doesn't return text at all.

```{important} Counts Unicode codepoints, not bytes
`length()` counts *characters* (Unicode codepoints), not raw UTF-8 storage
bytes. For plain ASCII text the two are identical, so this distinction is
invisible — but any character outside the ASCII range (an accented letter, a
currency symbol, an emoji, and so on) is encoded as more than one byte in
UTF-8 while still counting as exactly one character. See Example 2.
```

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with every selected feature replaced by its `Int64`
    character count; every other feature is left unchanged.

## Examples

Example 1. Character count of a plain ASCII text feature.

```cpp
using namespace pluss::table;

auto cities = DataFrame({
    {"city", Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
std::cout << cities;

auto out = cities.nt().length();
std::cout << out;
```

### Output (before)

```text
=============
     | city  
-------------
0    | Accra 
1    | Tema  
2    | Kumasi
-------------
[3 cs x 1 feat]
```

### Output (after)

```text
===========
     | city
-----------
0    | 5   
1    | 4   
2    | 6   
-----------
[3 cs x 1 feat]
```

`city` is now `Series`-backed `Int64` instead of `Note` text — `"Accra"` is 5
characters, `"Tema"` is 4, `"Kumasi"` is 6.

Example 2. Codepoints, not bytes: a cell containing a multi-byte UTF-8
character still counts as just the characters a person would see.

```cpp
auto labels = DataFrame({
    {"label", Thread(pluss::make_note({"Price: \xE2\x82\xAC""50", "Cost: $20", "Fee: \xC2\xA3""5"}))},
});

auto out2 = labels.nt().length();
std::cout << out2;
```

### Output

```text
============
     | label
------------
0    | 10   
1    | 9    
2    | 7    
------------
[3 cs x 1 feat]
```

`labels`' first row is `"Price: €50"` (the Euro sign, `\xE2\x82\xAC` in UTF-8
— three storage bytes) followed by `"50"`: 10 characters total (`P`, `r`,
`i`, `c`, `e`, `:`, a space, `€`, `5`, `0`), even though the underlying byte
count is 12. The third row, `"Fee: £5"`, has the two-byte Pound sign (`£`,
`\xC2\xA3`) and comes to 7 characters against 8 storage bytes. The middle
row, pure ASCII, has an identical character and byte count (`9` either way)
— it's included as a control case showing where the two measures agree.

```{note} A related renderer quirk
The plain-text table renderer used throughout these docs pads and aligns
table cells by raw byte length, not by character count, when it lays out a
column's width — a cosmetic-only detail unrelated to `length()`'s own
result, but worth knowing if a column containing multi-byte UTF-8 text ever
looks slightly misaligned when printed.
```

## Notes

- Changes the feature's payload kind from `Note` to `Series` (`Int64`) — chain
  a numeric operation (comparison, [`bucketize()`](../dataframe/bucketize.md),
  arithmetic) directly onto the result if needed.
- Counts Unicode codepoints, not raw UTF-8 storage bytes — see the caveat and
  Example 2 above.
- A null cell on input stays null on output, same as every other method on
  this accessor.

## See Also

- [`DataFrameNoteAccessor::pad`](pad.md): uses the same character-count
  notion of "width" when padding text to a target length.
- [`DataFrameNoteAccessor::chars`](chars.md): slice out a character range
  instead of just counting characters.
- [`DataFrame::bucketize`](../dataframe/bucketize.md): a natural next step —
  bin the resulting lengths into bands.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="indicate.html">
    <span class="label">Previous</span>
    <span class="title">indicate</span>
  </a>
  <a class="next-link" href="lower.html">
    <span class="label">Next</span>
    <span class="title">lower</span>
  </a>
</div>

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 975d9abc571b -->

# DataFrame::nt

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::nt() const -> DataFrameNoteAccessor
```

## What It Does

Enters the dedicated text-operations accessor for the current frame — string
stripping, casing, length, pattern matching, splitting, padding, and more, one method
per operation. `nt()` itself does no work beyond validating the frame and
constructing the accessor: **every feature currently on the frame must already be
`Note`-kind**, or it throws immediately. In practice this means selecting the text
feature(s) you want to operate on first, then calling `.nt()`:
`df.features({"city"}).nt().lower()`, not `df.nt()` directly on a frame that
still has numeric or other non-text columns.

This page previews the accessor's most common entry points. Every one of its methods
has its own dedicated, example-rich reference page — start at the
[DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference) and
use its left sidebar to jump straight to any method (`starts`, `has`, `count`,
`encode`, `matches`, `split`, `pad`, `replace`, and more).

## Parameters

`nt()` itself takes no parameters. See each accessor method's own signature for what
it accepts.

## Returns

`DataFrameNoteAccessor`
    A lightweight accessor bound to the current (all-`Note`) frame. Nearly every
    method on it returns a new `DataFrame` with the text operation applied; the
    source frame is never mutated.

## Examples

Example 1. Lowercase a text feature — after selecting it first.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",   Thread(pluss::make_series({1, 2, 3}))},
    {"city", Thread(pluss::make_note({"Accra", "TEMA", "Kumasi"}))},
});
std::cout << orders;
```

### Output (before)

```text
===================
     | id    city  
-------------------
0    | 1     Accra 
1    | 2     TEMA  
2    | 3     Kumasi
-------------------
[3 cs x 2 feat]
```

```cpp
auto lowered = orders.features({"city"}).nt().lower();
std::cout << lowered;
```

### Output (after)

```text
=============
     | city  
-------------
0    | accra 
1    | tema  
2    | kumasi
-------------
[3 cs x 1 feat]
```

`orders.nt()` directly would have thrown, since `orders` still has a `Series`
feature (`id`) at the time of the call — `.features({"city"})` narrows the frame down
to just the `Note` column first, which is what `.nt()` requires.

Example 2. Compute each string's length.

```cpp
auto lengths = orders.features({"city"}).nt().length();
std::cout << lengths;
```

### Output

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

`.length()` replaces the text with its character count as an `Int64` `Series` —
`"Accra"` -> `5`, `"TEMA"` -> `4`, `"Kumasi"` -> `6`.

## Notes

- Every feature on the frame `.nt()` is called on must already be `Note`-kind;
  select the text feature(s) first with [`features()`](features.md) or
  [`feataxis().note()`](feataxis.md) if the source frame has other kinds mixed in.
- The accessor's methods each apply to every feature on the (already-`Note`-only)
  frame, returning a new frame with the transformation applied — some methods (like
  `.length()`) change the resulting kind from `Note` to `Series`.
- `nt()` on a frame with no features at all, or a frame containing a non-`Note`
  feature, throws before constructing the accessor.

## See Also

- [`DataFrame::features`](features.md): the usual way to narrow a frame down to just
  its text features before calling `.nt()`.
- [`DataFrame::feataxis`](feataxis.md): `feataxis().note()` selects every `Note`
  feature on a frame directly, another way to arrive at an all-text frame.
- [`DataFrame::tl`](tl.md): the equivalent dedicated accessor for `Timeline` /
  `TimeIndex` features, instead of `Note` text.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nfeatures.html">
    <span class="label">Previous</span>
    <span class="title">nfeatures</span>
  </a>
  <a class="next-link" href="notnull.html">
    <span class="label">Next</span>
    <span class="title">notnull</span>
  </a>
</div>

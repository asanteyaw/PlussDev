<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3267059ba9c9 -->

# DataFrame::length

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::length() const -> std::int64_t
```

## What It Does

Returns the total cell count of the frame: [`width()`](width.md) times
[`height()`](height.md) — equivalently, `nfeatures() * ncases()`. This is the
number of individual scalar cells across the whole frame, not a row or column
count on its own; a `10`-row, `4`-feature frame has a `length()` of `40`.

```{note} Not to be confused with DataFrameNoteAccessor::length()
`.nt().length()` is an entirely different method on a different class: a
per-cell text-length accessor reached through the [`nt()`](nt.md) accessor,
returning one integer per case for a given text feature. `DataFrame::length()`
(this page) is a single whole-frame scalar with no accessor involved.
```

## Parameters

`None`

## Returns

`std::int64_t`
    `width() * height()` — the total number of cells in the frame.

## Examples

Example 1. Read the total cell count of a frame with several rows and columns.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Thread(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"city",       Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema", "Kumasi", "Kumasi"}))},
    {"units",      Thread(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Thread(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, 20.0}))},
});

auto out = orders.length();
std::cout << out;
```

### Output

```text
24
```

`orders` has `6` cases and `4` features (its own `[6 cs x 4 feat]` footer would
confirm both), so `length()` reports `6 * 4 = 24` total cells — the same value
`orders.width() * orders.height()` would compute directly.

## Notes

- `length() == width() * height() == nfeatures() * ncases()` always; there is no
  separate stored count, it is computed fresh from the two dimensions each call.
- Not related to [`DataFrameNoteAccessor::length()`](../dataframe-note/length.md)
  (reached via `.nt().length()`), which reports a per-case text length for one
  `Note` feature rather than a single whole-frame total.
- A frame with `0` cases or `0` features has `length() == 0`, even if the other
  dimension is non-zero.

## See Also

- [`DataFrame::width`](width.md) / [`DataFrame::height`](height.md): the two factors
  `length()` multiplies together.
- [`DataFrame::shape`](shape.md): the same two dimensions as a pair, without
  multiplying them.
- [`DataFrame::info`](info.md): a fuller structural report, including an estimated
  memory footprint that scales with `length()`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="le.html">
    <span class="label">Previous</span>
    <span class="title">le</span>
  </a>
  <a class="next-link" href="lengthen.html">
    <span class="label">Next</span>
    <span class="title">lengthen</span>
  </a>
</div>

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d39f9e630f2c -->

# DataFrame::observe

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::observe(std::int64_t row, bool render = false) const -> DataFrameView
```

## What It Does

Transposes **one row** into a readable, feature-by-feature record — a `DataFrameView`
with one case per source *feature* and two columns:

| column  | contents |
|---------|----------|
| `dtype` | that feature's stored type (`Series<float64>`, `Note<utf8>`, `Timeline<datetime64[ns, UTC]>`, …) |
| `value` | that feature's cell at `row`, stringified; an invalid cell shows `<null>` |

The card's case-label axis is the source feature names, so you read it top to bottom
like a record card. This is the shape you actually want when a frame is dozens of
columns wide and `view(row, row + 1)` (or a bare `slice`) hands back an unreadable
one-row-tall strip you have to scroll sideways through.

`observe` is **position-addressed**, not label-addressed: `row` is an integer offset.
Negative values count back from the end (`observe(-1)` is the last row). An
out-of-range `row` — including any `row` on a frame with no cases — throws
`"DataFrame: row index out of bounds"`. To pick the row by its case label first, go
through [`case_study().label(...)`](case_study.md) and `observe` position `0` of the
result, or [`label().pos(...)`](label.md) to turn a label into an offset.

Use [`view()`](view.md) / [`slice()`](slice.md) when you want a *range* of rows back
as a frame to keep computing on; use `observe` when you want to *look at* a single
observation. It does not mutate the source.

## Parameters

**row** (`std::int64_t`)

Zero-based position of the row to inspect. Negative counts from the end. Out of range
throws.

**render** (`bool`, default `false`)

Display toggle for the returned `DataFrameView`, identical in behavior to
[`head()`](head.md)'s `render` parameter — `true` prints the card immediately on
construction.

## Returns

`DataFrameView`
    A two-column (`dtype`, `value`) view, one case per source feature, case-labelled by
    feature name, carrying the source frame's title. `nfeatures()` of the source
    becomes `ncases()` of the card.

## Examples

Example 1. Read one observation out of a wide frame.

```cpp
using namespace pluss::table;

auto homes = DataFrame({
    {"home_id",   Thread(pluss::make_note({"H-104", "H-887", "H-231"}))},
    {"built_on",  Thread(pluss::make_timeline({"2021-06-14T00:00:00Z",
                                               "2019-02-03T00:00:00Z",
                                               "2022-11-30T00:00:00Z"}))},
    {"price",     Thread(pluss::make_series({412000.0, 268500.0, 559900.0}))},
    {"bedrooms",  Thread(pluss::make_series(std::vector<std::int64_t>{3, 2, 4}))},
    {"inspected", Thread(pluss::make_series(std::vector<bool>{true, false, true}))},
}).title("homes");

std::cout << homes.observe(1);
```

### Output

```text
=========================================================================
          | dtype                          value
-------------------------------------------------------------------------
home_id   | Note<utf8>                     H-887
built_on  | Timeline<datetime64[ns, UTC]>  2019-02-03T00:00:00.000000000Z
price     | Series<float64>                268500
bedrooms  | Series<int64>                  2
inspected | Series<bool>                   false
-------------------------------------------------------------------------
[5 cs x 2 feat]
```

Example 2. A missing cell renders as `<null>`; negative `row` counts from the end.

```cpp
auto with_gap = homes.assign(
    "bedrooms",
    homes.features("bedrooms").put(homes.features("home_id").eq("H-887"),
                                   pluss::null_t{}));

std::cout << with_gap.observe(-1);   // last row -> H-231
std::cout << with_gap.observe(1);    // the row whose bedrooms is now null
```

### Output

```text
=========================================================================
          | dtype                          value
-------------------------------------------------------------------------
home_id   | Note<utf8>                     H-231
built_on  | Timeline<datetime64[ns, UTC]>  2022-11-30T00:00:00.000000000Z
price     | Series<float64>                559900
bedrooms  | Series<int64>                  4
inspected | Series<bool>                   true
-------------------------------------------------------------------------
[5 cs x 2 feat]
=========================================================================
          | dtype                          value
-------------------------------------------------------------------------
home_id   | Note<utf8>                     H-887
built_on  | Timeline<datetime64[ns, UTC]>  2019-02-03T00:00:00.000000000Z
price     | Series<float64>                268500
bedrooms  | Series<int64>                  <null>
inspected | Series<bool>                   false
-------------------------------------------------------------------------
[5 cs x 2 feat]
```

Example 3. The card is an ordinary `DataFrameView` — pull a column out to work with
one feature's value as text (handy when a long value is clipped by the display width).

```cpp
auto card  = DataFrame(homes.observe(0));
auto value = card.features("value");   // a Note-kind Thread, one entry per feature
std::cout << value;
```

### Output

```text
  home_id  H-104
 built_on  2021-06-14T00:00:00.000000000Z
    price  412000
 bedrooms  3
inspected  true
Name: value, ptype: Note
```

Example 4. Out-of-range position throws rather than clamping.

```cpp
try {
  auto bad = homes.observe(9);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
DataFrame: row index out of bounds
```

## Notes

- One card per row by design — `observe` has no range overload. For a contiguous
  window of rows as a frame, use [`view()`](view.md) or [`slice()`](slice.md).
- `dtype` reports the full stored type (`Series<float64>` etc.), not just the payload
  kind — enough to tell an `int64` column from a `float64` one, or a numeric column
  from a stringified `Note`.
- The `value` column obeys the normal per-cell display width, so a very long field
  (a full-precision timestamp, a long string) can show clipped. Pull the column out as
  in Example 3 for the untruncated text.
- The card carries the source frame's title; `observe` never mutates the source.

## See Also

- [`DataFrame::view`](view.md) / [`DataFrame::slice`](slice.md): keep a *range* of rows
  as a frame for further computation.
- [`DataFrame::head`](head.md) / [`DataFrame::tail`](tail.md): preview rows at the ends.
- [`DataFrame::transpose`](transpose.md): flip the whole frame, every row to a column.
- [`DataFrame::case_study`](case_study.md): select rows by position, label, or mask
  when the result feeds later work.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nt.html">
    <span class="label">Previous</span>
    <span class="title">nt</span>
  </a>
  <a class="next-link" href="percentiles.html">
    <span class="label">Next</span>
    <span class="title">percentiles</span>
  </a>
</div>

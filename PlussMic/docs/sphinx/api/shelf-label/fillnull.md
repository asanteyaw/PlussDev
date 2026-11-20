<!-- pluss-hand-authored -->

# ShelfLabelAccessor::fillnull

[Back to ShelfLabelAccessor API Reference](../../shelf_label_accessor_api_reference)

## Signatures

```cpp
Shelf ShelfLabelAccessor::fillnull(double value) const
Shelf ShelfLabelAccessor::fillnull(std::string_view value) const
Shelf ShelfLabelAccessor::fillnull(const pluss::Timeline& value) const
Shelf ShelfLabelAccessor::fillnull(const pluss::TimeIndex& value) const
```

## What It Does

Replaces every null case label with `value` and applies the result back to
the bound shelf directly, returning the full shelf (same values, filled
case labels) — a direct forward to
`Shelf::caselbl(Shelf::caselbl().fillnull(value))`.

```{note} Applies immediately — unlike DataFrameLabelAccessor::fillnull
`DataFrameLabelAccessor::fillnull` returns only the filled label **values**,
requiring a follow-up `.label().set(...)` call to actually update the frame.
`ShelfLabelAccessor::fillnull` skips that step: it fills and reattaches the
labels in the same call, so the returned `Shelf` is already fully updated.
```

Pick the overload matching the case labels' own kind: `double` for a numeric
(`Series`) axis, `std::string_view` for `Note`, `Timeline`/`TimeIndex` for
their respective temporal kinds.

## Parameters

**value**

The replacement for every null case label, typed to match the case labels'
own kind.

## Returns

`Shelf`
    A new shelf, holding the same values as `*this`, with null case labels
    replaced by `value`.

## Examples

Example 1. Fill a null in a numeric case-label axis.

```cpp
using namespace pluss::table;

auto readings = Shelf(pluss::make_series({1.0, 2.0, 3.0}))
    .label().set(Shelf(pluss::make_series(
        {10.0, 0.0, 30.0}), pluss::make_series(std::vector<bool>{true, false, true})));
std::cout << readings;

auto filled = readings.label().fillnull(0.0);
std::cout << filled;
```

### Output (before)

```text
  10  1
NULL  2
  30  3
ptype: Series
```

### Output (after)

```text
10  1
 0  2
30  3
ptype: Series
```

The middle case label's `NULL` became `0`, and — unlike the DataFrame-level
version — `filled` is already the complete, updated shelf; no separate
`set()` call was needed.

Example 2. Fill a `NaT` in a `Timeline` case-label axis.

```cpp
auto with_nat = Shelf(pluss::make_series({1.0, 2.0, 3.0}))
    .label().set(Shelf(pluss::make_timeline(
        {"2024-01-01T00:00:00Z", "", "2024-01-03T00:00:00Z"})));
std::cout << with_nat;

auto filled2 = with_nat.label().fillnull(pluss::make_timeline("2024-01-02T00:00:00Z"));
std::cout << filled2;
```

### Output (before)

```text
2024-01-01 00:00:00.0~  1
                   NaT  2
2024-01-03 00:00:00.0~  3
ptype: Series
```

### Output (after)

```text
2024-01-01 00:00:00.0~  1
                   NaT  2
2024-01-03 00:00:00.0~  3
ptype: Series
```

## Notes

- Applies immediately — the returned shelf already carries the filled
  labels; no follow-up `set()` call is needed (contrast with
  `DataFrameLabelAccessor::fillnull`, see the note above).
- Choose the overload matching the case labels' own kind — passing a
  `double` to a `Note` axis (or vice versa) doesn't fit any overload and
  won't compile.
- The `pluss::TimeIndex` overload behaves identically to the `pluss::Timeline`
  overload — the same fill mechanics apply to either temporal kind.

## See Also

- [`ShelfLabelAccessor::isnull`](isnull.md),
  [`ShelfLabelAccessor::notnull`](notnull.md): check which case labels are
  null before (or instead of) filling them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="duplicated.html">
    <span class="label">Previous</span>
    <span class="title">duplicated</span>
  </a>
  <a class="next-link" href="find.html">
    <span class="label">Next</span>
    <span class="title">find</span>
  </a>
</div>

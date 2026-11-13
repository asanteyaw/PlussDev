<!-- pluss-hand-authored -->

# SlateLabelAccessor::fillnull

[Back to SlateLabelAccessor API Reference](../../slate_label_accessor_api_reference)

## Signatures

```cpp
Slate SlateLabelAccessor::fillnull(double value) const
Slate SlateLabelAccessor::fillnull(std::string_view value) const
Slate SlateLabelAccessor::fillnull(const pluss::Timeline& value) const
Slate SlateLabelAccessor::fillnull(const pluss::TimeIndex& value) const
```

## What It Does

Replaces every null case label with `value` and applies the result back to
the bound slate directly, returning the full slate (same values, filled
case labels) — a direct forward to
`Slate::caselbl(Slate::caselbl().fillnull(value))`.

```{note} Applies immediately — unlike DataFrameLabelAccessor::fillnull
`DataFrameLabelAccessor::fillnull` returns only the filled label **values**,
requiring a follow-up `.label().set(...)` call to actually update the frame.
`SlateLabelAccessor::fillnull` skips that step: it fills and reattaches the
labels in the same call, so the returned `Slate` is already fully updated.
```

Pick the overload matching the case labels' own kind: `double` for a numeric
(`Series`) axis, `std::string_view` for `Note`, `Timeline`/`TimeIndex` for
their respective temporal kinds.

## Parameters

**value**

The replacement for every null case label, typed to match the case labels'
own kind.

## Returns

`Slate`
    A new slate, holding the same values as `*this`, with null case labels
    replaced by `value`.

## Examples

Example 1. Fill a null in a numeric case-label axis.

```cpp
using namespace pluss::table;

auto readings = Slate(pluss::make_series({1.0, 2.0, 3.0}))
    .label().set(Slate(pluss::make_series(
        {10.0, std::numeric_limits<double>::quiet_NaN(), 30.0})));
std::cout << readings;

auto filled = readings.label().fillnull(0.0);
std::cout << filled;
```

### Output (before)

```text
10    1
NULL  2
30    3

ptype: Series
```

### Output (after)

```text
10  1
0   2
30  3

ptype: Series
```

The middle case label's `NULL` became `0`, and — unlike the DataFrame-level
version — `filled` is already the complete, updated slate; no separate
`set()` call was needed.

Example 2. Fill a `NaT` in a `Timeline` case-label axis.

```cpp
auto with_nat = Slate(pluss::make_series({1.0, 2.0, 3.0}))
    .label().set(Slate(pluss::make_timeline(
        {"2024-01-01T00:00:00Z", "", "2024-01-03T00:00:00Z"})));
std::cout << with_nat;

auto filled2 = with_nat.label().fillnull(pluss::make_timeline("2024-01-02T00:00:00Z"));
std::cout << filled2;
```

### Output (before)

```text
2024-01-01  1
NULL        2
2024-01-03  3

ptype: Series
```

### Output (after)

```text
2024-01-01  1
2024-01-02  2
2024-01-03  3

ptype: Series
```

## Notes

- Applies immediately — the returned slate already carries the filled
  labels; no follow-up `set()` call is needed (contrast with
  `DataFrameLabelAccessor::fillnull`, see the note above).
- Choose the overload matching the case labels' own kind — passing a
  `double` to a `Note` axis (or vice versa) doesn't fit any overload and
  won't compile.
- The `pluss::TimeIndex` overload behaves identically to the `pluss::Timeline`
  overload — the same fill mechanics apply to either temporal kind.

## See Also

- [`SlateLabelAccessor::isnull`](isnull.md),
  [`SlateLabelAccessor::notnull`](notnull.md): check which case labels are
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

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 659443fa590b -->

# DataFrameLabelAccessor::fillnull

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signatures

```cpp
DataFrameLabelAccessor::fillnull(double value) const -> Thread
DataFrameLabelAccessor::fillnull(std::string_view value) const -> Thread
DataFrameLabelAccessor::fillnull(const char* value) const -> Thread
DataFrameLabelAccessor::fillnull(const pluss::Timeline& value) const -> Thread
DataFrameLabelAccessor::fillnull(const pluss::TimeIndex& value) const -> Thread
```

## What It Does

Replaces every null case label with `value` and returns the filled case-label
**values** — it does **not** apply the fill back to the frame automatically.
To actually change the frame's case labels, pass the result to
[`set()`](set.md).

Pick the overload matching the case labels' own kind: `double` for a numeric
(`Series`) axis, `std::string_view`/`const char*` for `Note`, `Timeline`/
`TimeIndex` for their respective temporal kinds.

## Parameters

**value**

The replacement for every null case label, typed to match the case labels'
own kind.

## Returns

`Thread`
    The case-label values with nulls replaced — not yet applied back to the
    frame.

## Examples

Example 1. Fill a null in a numeric case-label axis.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"value", Thread(pluss::make_series({1.0, 2.0, 3.0}))}},
    Thread(pluss::make_series({10.0, std::numeric_limits<double>::quiet_NaN(), 30.0})));
std::cout << readings;

auto filled = readings.label().fillnull(0.0);
std::cout << filled;
```

### Output (before)

```text
============
     | value
------------
10   | 1    
NULL | 2    
30   | 3    
------------
[3 cs x 1 feat]
```

### Output (filled values, not yet applied)

```text
0  10
1  0
2  30
ptype: Series
```

`filled` is a bare `Thread` — the middle case label's `NULL` became `0`, but
`readings` itself is unchanged unless you call
`readings.label().set(filled)` to apply it.

Example 2. Fill a `NaT` in a `Timeline` case-label axis, using the
`pluss::Timeline` overload.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"value", Thread(pluss::make_series({1.0, 2.0, 3.0}))}},
    Thread(pluss::make_timeline({"2024-01-01T00:00:00Z", "", "2024-01-03T00:00:00Z"})));
std::cout << readings;

auto filled = readings.label().fillnull(pluss::make_timeline("2024-01-02T00:00:00Z"));
std::cout << filled;
```

### Output (before)

```text
==================
           | value
------------------
2024-01-01 | 1    
NULL       | 2    
2024-01-03 | 3    
------------------
[3 cs x 1 feat]
```

### Output (filled values, not yet applied)

```text
0  2024-01-01
1  2024-01-02
2  2024-01-03
ptype: Timeline
```

The empty string in the constructor produced a `NaT` middle label, displayed
as `NULL`; `fillnull` replaces it with the single supplied `pluss::Timeline`
value, `2024-01-02`. As in Example 1, this is still just the filled values —
`readings` itself needs `readings.label().set(filled)` to actually change.

## Notes

- Returns values only; chain into [`set()`](set.md) to actually update the
  frame's case labels.
- Choose the overload matching the case labels' `ThreadKind` — passing a
  `double` to a `Note` axis (or vice versa) doesn't fit any overload and won't
  compile.
- The `std::string_view`/`const char*` overloads work the same way as
  Example 1's `double` overload, just for a `Note`-kind axis instead of a
  numeric one — pass the replacement text directly, e.g.
  `readings.label().fillnull("unknown")`.
- The `pluss::TimeIndex` overload behaves identically to Example 2's
  `pluss::Timeline` overload — the same fill mechanics apply to either
  temporal kind.

## See Also

- [`DataFrameLabelAccessor::isnull`](isnull.md),
  [`DataFrameLabelAccessor::notnull`](notnull.md): check which case labels are
  null before (or instead of) filling them.
- [`DataFrameLabelAccessor::set`](set.md): apply the filled values back to the
  frame.

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

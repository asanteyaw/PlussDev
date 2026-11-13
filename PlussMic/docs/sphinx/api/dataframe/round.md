<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: edcf831687c0 -->

# DataFrame::round

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame DataFrame::round(std::int64_t decimals = 0) const
```

## What It Does

Rounds every numeric feature to `decimals` decimal places, cell by cell. With the
default `decimals = 0`, each value rounds to the nearest whole number. A null cell
stays null — `round()` only ever changes an observed number's precision, never invents
a value for a missing one.

## Parameters

**decimals** (`std::int64_t`, default `0`)

Number of decimal places to keep. `0` rounds to the nearest integer. A positive value
keeps that many digits after the decimal point (internally, the value is scaled by
`10^decimals`, rounded to the nearest whole number, then scaled back down).

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature rounded to
    `decimals` places. Non-numeric features (`Note`, `Timeline`, `TimeIndex`) are left
    exactly as they were.

## Examples

Example 1. Round to whole numbers with the default `decimals = 0`.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"reading", pluss::make_series({3.14159, 2.71828, std::numeric_limits<double>::quiet_NaN(), -1.4142})},
});

std::cout << readings;
```

### Output (before)

```text
==============
     | reading
--------------
0    | 3.14159
1    | 2.71828
2    | NULL   
3    | -1.4142
--------------
[4 cs x 1 feat]
```

```cpp
auto whole = readings.round();
std::cout << whole;
```

### Output (after)

```text
==============
     | reading
--------------
0    | 3      
1    | 3      
2    | NULL   
3    | -1     
--------------
[4 cs x 1 feat]
```

Every observed value rounded to the nearest integer; the `NULL` at case `2` stayed
`NULL`.

Example 2. Round to two decimal places instead.

```cpp
auto two_places = readings.round(2);
std::cout << two_places;
```

### Output

```text
==============
     | reading
--------------
0    | 3.14   
1    | 2.72   
2    | NULL   
3    | -1.41  
--------------
[4 cs x 1 feat]
```

`2.71828` rounds up to `2.72` and `-1.4142` rounds up (in magnitude) to `-1.41` — each
value keeps exactly two digits after the decimal point instead of being collapsed to a
whole number.

## Notes

- `round()` applies to **every** numeric feature on the frame at once; there is no
  per-feature selector. Round one feature at a time via [`assign()`](assign.md) if only
  some columns should change.
- Non-numeric features pass through completely unchanged.
- Nulls pass through completely unchanged — `round()` is not a substitute for
  [`fillnull()`](fillnull.md).
- This is the numeric, whole-frame `round()`. `DataFrame::tl()`'s
  [`DataFrameTimelineAccessor::round`](../dataframe-timeline/round.md) is a different,
  unrelated method that rounds timestamps to a calendar unit.

## See Also

- [`DataFrame::abs`](abs.md): a different elementwise numeric cleanup, taking a
  magnitude instead of rounding.
- [`DataFrame::clip`](clip.md): bound values into a range instead of rounding their
  precision.
- [`DataFrame::astype`](astype.md): convert a feature's dtype outright, if rounding to
  an integer dtype (rather than a rounded double) is the actual goal.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ror.html">
    <span class="label">Previous</span>
    <span class="title">ror</span>
  </a>
  <a class="next-link" href="rpow.html">
    <span class="label">Next</span>
    <span class="title">rpow</span>
  </a>
</div>

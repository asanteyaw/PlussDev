<!-- pluss-hand-authored -->

# DataFrame::shift

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::shift(std::int64_t periods = 1) const -> DataFrame
DataFrame::shift(pluss::RangeView<std::string_view> features, std::int64_t periods = 1) const -> DataFrame
```

## What It Does

Shifts each selected feature's values down (or up) by `periods` positions along the
case axis — case `i`'s new value becomes whatever was at case `i - periods` before
the shift, with `NULL` filling in the positions that have no source row to pull
from. This is the standard "lag/lead" primitive: a positive `periods` produces a lag
(each row now shows an earlier row's value; the first `periods` rows become `NULL`),
a negative `periods` produces a lead (each row shows a later row's value; the last
`|periods|` rows become `NULL`).

The single-argument overload shifts every feature; the range overload restricts the
shift to the named `features`, leaving every other feature completely untouched.

## Parameters

**periods** (`std::int64_t`, default `1`)

How many positions to shift by. Positive values move values downward (lag):
case `i` becomes case `i - periods`'s old value, and the first `periods` rows become
`NULL`. Negative values move values upward (lead), with `NULL` filling the last
`|periods|` rows instead.

**features** (`pluss::RangeView<std::string_view>`)

Which features to shift. Only present on the range overload; every feature not
listed here is left completely unchanged. Omitting this argument entirely (the
single-argument overload) shifts every feature.

## Returns

`DataFrame`
    A new frame, same shape as the source, with the selected feature(s)' values
    shifted and `NULL`-padded at the vacated end.

## Examples

Example 1. Shift one selected feature, leaving the rest of the frame untouched.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Thread(pluss::make_series({1, 2, 3}))},
    {"sales", Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
std::cout << orders;
```

### Output (before)

```text
==========================
     | id    sales  city  
--------------------------
0    | 1     100    Accra 
1    | 2     120    Tema  
2    | 3     90     Kumasi
--------------------------
[3 cs x 3 feat]
```

```cpp
auto shifted_sales = orders.shift({"sales"}, 2);
std::cout << shifted_sales;
```

### Output (after)

```text
==========================
     | id    sales  city  
--------------------------
0    | 1     NULL   Accra 
1    | 2     NULL   Tema  
2    | 3     100    Kumasi
--------------------------
[3 cs x 3 feat]
```

Only `sales` moved: case `2` now shows case `0`'s old value (`100`), and cases `0`
and `1` — which have no source row two positions back — become `NULL`. `id` and
`city` are completely unaffected.

Example 2. Shift every feature by the same amount.

```cpp
auto shifted_all = orders.shift(2);
std::cout << shifted_all;
```

### Output

```text
=========================
     | id    sales  city 
-------------------------
0    | NULL  NULL   NULL 
1    | NULL  NULL   NULL 
2    | 1     100    Accra
-------------------------
[3 cs x 3 feat]
```

Every feature — including `city`, a `Note` — shifted together; case `2` now carries
case `0`'s complete original row, and cases `0` and `1` are entirely `NULL`.

Example 3. A negative `periods` shifts upward (a lead) instead.

```cpp
auto led = orders.shift(-1);
std::cout << led;
```

### Output

```text
==========================
     | id    sales  city  
--------------------------
0    | 2     120    Tema  
1    | 3     90     Kumasi
2    | NULL  NULL   NULL  
--------------------------
[3 cs x 3 feat]
```

With `periods = -1`, case `0` now shows what used to be at case `1`, and case `1`
shows what used to be at case `2` — the vacated gap lands at the **end** (case `2`)
instead of the start.

## Notes

- `periods = 0` (not the default) or an empty frame both return `*this` unchanged
  rather than doing any work.
- `shift()` never reorders case labels — only the feature values move; the case
  axis stays exactly as it was.
- Shifting a `Note` feature fills the vacated positions with a missing (`NULL`)
  text value, the same missing-value convention used across every other kind.

## See Also

- [`DataFrame::ts`](ts.md): `ts().lag(periods)` is equivalent to this method scoped
  to numeric features, as part of the broader time-series accessor.
- [`DataFrame::cumsum`](cumsum.md), [`DataFrame::scan`](scan.md): other case-axis
  running computations, rather than a plain positional shift.
- [`DataFrame::fillnull`](fillnull.md): resolve the `NULL`s a shift introduces, if a
  gap at the edge should not appear at all.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shape.html">
    <span class="label">Previous</span>
    <span class="title">shape</span>
  </a>
  <a class="next-link" href="show.html">
    <span class="label">Next</span>
    <span class="title">show</span>
  </a>
</div>

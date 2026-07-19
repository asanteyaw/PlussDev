<!-- pluss-hand-authored -->

# DataFrameTsAccessor::log_returns

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signature

```cpp
DataFrameTsAccessor::log_returns(std::int64_t periods = 1) const -> DataFrame
```

## What It Does

Computes the logarithmic period-over-period change of every bound feature:
`out[i] = ln(x[i] / base[i])`, where `base` is `x` shifted by `periods` (a lag when
`periods > 0`, a lead when `periods < 0`). A cell is `NULL` wherever `x[i]` or
`base[i]` is missing or non-finite, **or either value is not strictly positive** —
unlike [`returns()`](returns.md), a zero or negative value anywhere in the
comparison always yields `NULL`, since the logarithm would otherwise be undefined.

## Parameters

**periods** (`std::int64_t`, default `1`)

The shift distance used to build the comparison base. Positive shifts compare each
row against an earlier one (a lag); negative shifts compare against a later one (a
lead). Must not be `0`, or the call throws.

## Returns

`DataFrame`
    A new frame, same shape and case labels as the accessor's bound features, with
    each feature replaced by its period-over-period log return.

## Examples

Example 1. Compute the one-period log return of a numeric feature.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"sensor_id", Thread(pluss::make_series({1, 2, 3, 4}))},
    {"load",      Thread(pluss::make_series({10.0, 20.0, 15.0, 30.0}))},
});

auto out = readings.features({"load"}).ts().log_returns(1);
std::cout << out;
```

### Output

```text
====================
     | load         
--------------------
0    | NULL         
1    | 0.6931471806 
2    | -0.2876820725
3    | 0.6931471806 
--------------------
[4 cs x 1 feat]
```

Case `0` has no prior row, so it's `NULL`. Case `1` is `ln(20 / 10) = ln(2) ≈
0.6931`; case `2` is `ln(15 / 20) = ln(0.75) ≈ -0.2877`; case `3` is
`ln(30 / 15) = ln(2) ≈ 0.6931` — the same log return as case `1`, since both moves
double the value, whereas [`returns()`](returns.md) would report the same `1.0`
fractional return for both but a different one for the halving case.

Example 2. A negative value in the comparison — [`returns()`](returns.md)
still returns a number, but `log_returns()` is `NULL`.

```cpp
auto signed_readings = DataFrame({
    {"sensor_id", Thread(pluss::make_series({1, 2, 3}))},
    {"delta",     Thread(pluss::make_series({10.0, -5.0, 20.0}))},
});

auto pct = signed_readings.features({"delta"}).ts().returns(1);
std::cout << pct;

auto log_pct = signed_readings.features({"delta"}).ts().log_returns(1);
std::cout << log_pct;
```

### Output (returns)

```text
============
     | delta
------------
0    | NULL 
1    | -1.5 
2    | -5   
------------
[3 cs x 1 feat]
```

### Output (log_returns)

```text
============
     | delta
------------
0    | NULL 
1    | NULL 
2    | NULL 
------------
[3 cs x 1 feat]
```

`returns()` only requires the base to be nonzero — case `1` (`-5` vs. base
`10`) is `-5/10 - 1 = -1.5`, and case `2` (`20` vs. base `-5`) is
`20/-5 - 1 = -5`, both perfectly well-defined ratios. `log_returns()` instead
requires **both** sides to be strictly positive; since every comparison here
involves the negative `-5`, all three cases come back `NULL`.

Example 3. Pass `periods = 0`, to see the call throw.

```cpp
try {
  auto out = readings.features({"load"}).ts().log_returns(0);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
subsp::stats::ts::log_returns: periods cannot be zero
```

Comparing each row against itself isn't a meaningful log return, so `periods =
0` is rejected outright rather than silently returning all-zero output.

## Notes

- Unlike [`returns()`](returns.md), `log_returns()` requires **both** `x[i]` and
  `base[i]` to be strictly greater than `0`; a zero or negative value in either
  produces `NULL` rather than an error.
- Log returns are additive across periods (the sum of consecutive one-period log
  returns equals the multi-period log return), which plain fractional
  [`returns()`](returns.md) are not — a common reason to prefer this method for
  compounding analysis.
- The result keeps the accessor's original case labels; nothing is dropped or
  re-indexed.

## See Also

- [`DataFrameTsAccessor::returns`](returns.md): the fractional (non-logarithmic)
  counterpart, which tolerates zero or negative values more permissively.
- [`DataFrameTsAccessor::diff`](diff.md): the absolute period-over-period change.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lead.html">
    <span class="label">Previous</span>
    <span class="title">lead</span>
  </a>
  <a class="next-link" href="mul.html">
    <span class="label">Next</span>
    <span class="title">mul</span>
  </a>
</div>

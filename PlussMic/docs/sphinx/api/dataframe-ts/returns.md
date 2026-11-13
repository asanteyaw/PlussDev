<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5ae6f36f52c2 -->

# DataFrameTsAccessor::returns

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameTsAccessor::returns(std::int64_t periods = 1) const
```

## What It Does

Computes the fractional period-over-period change of every bound feature:
`out[i] = x[i] / base[i] - 1`, where `base` is `x` shifted by `periods` (a lag when
`periods > 0`, a lead when `periods < 0`). A cell is `NULL` wherever `x[i]` or
`base[i]` is missing, `base[i]` is `0`, or either value is non-finite.

## Parameters

**periods** (`std::int64_t`, default `1`)

The shift distance used to build the comparison base. Positive shifts compare each
row against an earlier one (a lag); negative shifts compare against a later one (a
lead). Must not be `0`, or the call throws.

## Returns

`DataFrame`
    A new frame, same shape and case labels as the accessor's bound features, with
    each feature replaced by its period-over-period fractional return.

## Examples

Example 1. Compute the one-period return of a numeric feature.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"sensor_id", Slate(pluss::make_series({1, 2, 3, 4}))},
    {"load",      Slate(pluss::make_series({10.0, 20.0, 15.0, 30.0}))},
});
std::cout << readings;
```

### Output (before)

```text
======================
     | sensor_id  load
----------------------
0    | 1          10  
1    | 2          20  
2    | 3          15  
3    | 4          30  
----------------------
[4 cs x 2 feat]
```

```cpp
auto out = readings.features({"load"}).ts().returns(1);
std::cout << out;
```

### Output (after)

```text
============
     | load 
------------
0    | NULL 
1    | 1    
2    | -0.25
3    | 1    
------------
[4 cs x 1 feat]
```

`readings.features({"load"})` scopes the accessor to just `load`, since `ts()`
requires every bound feature to already be a numeric `Series`. Case `0` has no
prior row to compare against, so it's `NULL`. Case `1` is `20 / 10 - 1 = 1.0` (a
100% increase); case `2` is `15 / 20 - 1 = -0.25` (a 25% decrease); case `3` is
`30 / 15 - 1 = 1.0`.

Example 2. Pass a negative `periods`, comparing each row against a *later* one
(a lead) instead of an earlier one.

```cpp
auto out_lead = readings.features({"load"}).ts().returns(-1);
std::cout << out_lead;
```

### Output

```text
===================
     | load        
-------------------
0    | -0.5        
1    | 0.3333333333
2    | -0.5        
3    | NULL        
-------------------
[4 cs x 1 feat]
```

With `periods = -1`, each row's `base` is the *next* row instead of the
previous one: case `0` is `10 / 20 - 1 = -0.5`; case `1` is
`20 / 15 - 1 = 0.3333`; case `2` is `15 / 30 - 1 = -0.5`. Case `3` has no later
row to serve as `base`, so it's `NULL` this time — the mirror image of
Example 1's default (`periods = 1`), where it was case `0` that had no prior
row.

Example 3. A `base` of exactly `0`, to see it come back `NULL` rather than
infinity.

```cpp
using namespace pluss::table;

auto zero_base = DataFrame({
    {"sensor_id", Slate(pluss::make_series({1, 2, 3}))},
    {"delta",     Slate(pluss::make_series({10.0, 0.0, 20.0}))},
});

auto out2 = zero_base.features({"delta"}).ts().returns(1);
std::cout << out2;
```

### Output

```text
============
     | delta
------------
0    | NULL 
1    | -1   
2    | NULL 
------------
[3 cs x 1 feat]
```

Case `1` (`x = 0`, base `= 10`) is an ordinary `0 / 10 - 1 = -1`. Case `2`
(`x = 20`, base `= 0`) has a `base` of exactly `0` — rather than dividing by
zero, `returns()` reports `NULL` directly, per the rule stated above.

Example 4. Pass `periods = 0`, to see the call throw.

```cpp
try {
  auto out3 = readings.features({"load"}).ts().returns(0);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
subsp::stats::ts::returns: periods cannot be zero
```

Comparing each row against itself isn't a meaningful return, so `periods = 0`
is rejected outright rather than silently returning all-zero output.

## Notes

- `returns()` operates independently, feature by feature — every bound feature is
  shifted and divided against its own prior (or later) values.
- A `base` value of exactly `0` always produces `NULL`, regardless of `x[i]`, since
  the fractional change would otherwise be undefined or infinite.
- The result keeps the accessor's original case labels; nothing is dropped or
  re-indexed.

## See Also

- [`DataFrameTsAccessor::log_returns`](log_returns.md): the logarithmic counterpart,
  additionally requiring both compared values to be strictly positive.
- [`DataFrameTsAccessor::diff`](diff.md): the absolute (non-fractional) period-over-
  period change.
- [`DataFrameTsAccessor::lag`](lag.md), [`DataFrameTsAccessor::lead`](lead.md): the
  underlying shift this method's comparison base is built from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="resample.html">
    <span class="label">Previous</span>
    <span class="title">resample</span>
  </a>
  <a class="next-link" href="rolling.html">
    <span class="label">Next</span>
    <span class="title">rolling</span>
  </a>
</div>

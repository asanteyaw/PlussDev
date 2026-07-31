<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f780f5a72430 -->

# AtlasTsAccessor::log_returns

[Back to AtlasTsAccessor API Reference](../../atlas_ts_accessor_api_reference)

## Signature

```cpp
AtlasTsAccessor::log_returns(std::int64_t periods = 1) const -> Atlas
```

## What It Does

Computes the logarithmic period-over-period change of every bound feature in
**every frame of the stack**: `out[i] = ln(x[i] / base[i])`, where `base` is `x`
shifted by `periods` within that same frame — the same transform
[`DataFrameTsAccessor::log_returns`](../dataframe-ts/log_returns.md) applies to a
single frame, broadcast across the whole `Atlas` via
[`Atlas::map_frames()`](../atlas/map_frames.md). A cell is `NULL` wherever `x[i]` or
`base[i]` is missing or non-finite, **or either value is not strictly positive**.

## Parameters

**periods** (`std::int64_t`, default `1`)
    The shift distance used to build the comparison base, within each frame
    independently. Must not be `0`, or the call throws.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's bound
    feature replaced by its period-over-period log return.

## Examples

Example 1. Compute the one-period log return of a numeric feature stacked across two
frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"load", Thread(pluss::make_series({10.0, 20.0, 15.0, 30.0}))}});
auto q2 = DataFrame({{"load", Thread(pluss::make_series({12.0, 18.0, 14.0, 28.0}))}});
auto atlas = Atlas({q1, q2});

auto out = atlas.ts().log_returns();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
====================
     | load         
--------------------
0    |  nan         
1    |  0.6931471806
2    | -0.2876820725
3    |  0.6931471806
--------------------
[4 cs x 1 feat]
====================
     | load         
--------------------
0    |  nan         
1    |  0.4054651081
2    | -0.2513144283
3    |  0.6931471806
--------------------
[4 cs x 1 feat]
```

Each frame's log return is computed independently, feature by feature — `frame(1)`'s
row `1` is `ln(18 / 12) ≈ 0.4055`, unrelated to `frame(0)`'s own values.

## Notes

- Unlike [`returns()`](returns.md), `log_returns()` requires **both** `x[i]` and
  `base[i]` to be strictly greater than `0`; a zero or negative value in either
  produces `NULL`.
- Log returns are additive across periods, which plain fractional
  [`returns()`](returns.md) are not.
- `Atlas::ts()` auto-binds the numeric `Series` features on each frame, same as
  [`DataFrame::ts()`](../dataframe/ts.md) does for a single frame.

## See Also

- [`DataFrameTsAccessor::log_returns`](../dataframe-ts/log_returns.md): the
  single-frame operation this method broadcasts across the stack.
- [`AtlasTsAccessor::returns`](returns.md): the fractional (non-logarithmic)
  counterpart, which tolerates zero or negative values more permissively.
- [`AtlasTsAccessor::diff`](diff.md): the absolute period-over-period change.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lead.html">
    <span class="label">Previous</span>
    <span class="title">lead</span>
  </a>
  <a class="next-link" href="resample.html">
    <span class="label">Next</span>
    <span class="title">resample</span>
  </a>
</div>

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3f57a4dd177f -->

# AtlasTsAccessor::returns

[Back to AtlasTsAccessor API Reference](../../atlas_ts_accessor_api_reference)

## Signature

```cpp
AtlasTsAccessor::returns(std::int64_t periods = 1) const -> Atlas
```

## What It Does

Computes the fractional period-over-period change of every bound feature in
**every frame of the stack**: `out[i] = x[i] / base[i] - 1`, where `base` is `x`
shifted by `periods` within that same frame — the same transform
[`DataFrameTsAccessor::returns`](../dataframe-ts/returns.md) applies to a single
frame, broadcast across the whole `Atlas` via
[`Atlas::map_frames()`](../atlas/map_frames.md). A cell is `NULL` wherever `x[i]` or
`base[i]` is missing, `base[i]` is `0`, or either value is non-finite.

## Parameters

**periods** (`std::int64_t`, default `1`)
    The shift distance used to build the comparison base, within each frame
    independently. Positive shifts compare each row against an earlier one (a lag);
    negative shifts compare against a later one (a lead). Must not be `0`, or the
    call throws.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's bound
    feature replaced by its period-over-period fractional return.

## Examples

Example 1. Compute the one-period return of a numeric feature stacked across two
frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"load", Slate(pluss::make_series({10.0, 20.0, 15.0, 30.0}))}});
auto q2 = DataFrame({{"load", Slate(pluss::make_series({12.0, 18.0, 14.0, 28.0}))}});
auto atlas = Atlas({q1, q2});

auto out = atlas.ts().returns();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
============
     | load 
------------
0    |  nan 
1    |  1   
2    | -0.25
3    |  1   
------------
[4 cs x 1 feat]
====================
     | load         
--------------------
0    |  nan         
1    |  0.5         
2    | -0.2222222222
3    |  1           
--------------------
[4 cs x 1 feat]
```

Each frame computes its own returns independently — `frame(1)`'s first return is
`18 / 12 - 1 = 0.5`, unrelated to `frame(0)`'s values.

## Notes

- `returns()` operates independently, feature by feature *and frame by frame* — no
  frame's computation ever crosses into another's data.
- A `base` value of exactly `0` always produces `NULL`, regardless of `x[i]`.
- `Atlas::ts()` auto-binds the numeric `Series` features on each frame, same as
  [`DataFrame::ts()`](../dataframe/ts.md) does for a single frame.

## See Also

- [`DataFrameTsAccessor::returns`](../dataframe-ts/returns.md): the single-frame
  operation this method broadcasts across the stack.
- [`AtlasTsAccessor::log_returns`](log_returns.md): the logarithmic counterpart.
- [`AtlasTsAccessor::diff`](diff.md): the absolute (non-fractional) period-over-period
  change.

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

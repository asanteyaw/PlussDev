<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3e630a4944b2 -->

# StreamTimeSeriesAccessor::ewm

[Back to StreamTimeSeriesAccessor API Reference](../../stream_ts_accessor_api_reference)

## Signature

```cpp
Stream StreamTimeSeriesAccessor::ewm(
    double alpha,
    pluss::table::WindowStatistic statistic = pluss::table::WindowStatistic::Mean,
    bool adjusted = true, bool bias_corrected = true,
    std::int64_t min_count = 1, bool ignore_nulls = false) const
```

## What It Does

Computes an **exponentially-weighted moving statistic**: each position's
output weighs every position seen so far, with more recent positions
weighted more heavily by a factor of `alpha` (`0 < alpha <= 1`). Only `Mean`,
`Var`, and `Std` are supported.

`adjusted` selects between two well-known EWM conventions: `true` (default)
normalizes by the sum of decay weights seen so far and supports missing
entries directly; `false` uses the classic recursive form and **requires the
source stream to carry no validity mask at all**.

```{important} StreamTimeSeriesAccessor requires the bound stream to be numeric Series-kind
Same precondition as every method on this accessor — see
[`Stream::ts`](../stream/ts.md)'s own important callout for the exact throw
message pattern.
```

## Parameters

**alpha** (`double`)
    The smoothing factor, in `(0, 1]`.

**statistic** (`pluss::table::WindowStatistic`, default `Mean`)
    `Mean`, `Var`, or `Std`.

**adjusted** (`bool`, default `true`)
    `true`: normalized form, supports missing entries. `false`: classic
    recursive form, requires a non-nullable source stream.

**bias_corrected** (`bool`, default `true`)
    For `Var`/`Std` only: whether to apply a bias correction.

**min_count** (`std::int64_t`, default `1`)
    The smallest number of real observations seen so far still considered
    valid.

**ignore_nulls** (`bool`, default `false`)
    Only relevant with `adjusted = true`: whether a gap consumes a decay
    step (`false`) or is skipped entirely for decay purposes (`true`).

## Returns

`Stream`
    A new stream, same length as the bound source, holding the
    exponentially-weighted statistic at each position.

## Examples

Example 1. An exponentially-weighted mean, `adjusted = true` (the default).

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));
auto out = stream.ts().ewm(0.5);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=no  mask=yes  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 10                                                                                                       
1            | 16.6666666667                                                                                            
2            | 24.2857142857                                                                                            
========================================================================================================================
```

Example 2 (edge case). The same data with `adjusted = false` — the classic
recursive formula gives a different (and simpler) result.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));
auto out = stream.ts().ewm(0.5, pluss::table::WindowStatistic::Mean, false);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=no  mask=yes  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 10                                                                                                       
1            | 15                                                                                                       
2            | 22.5                                                                                                     
========================================================================================================================
```

`y0 = x0 = 10`; `y1 = 0.5*20 + 0.5*10 = 15`; `y2 = 0.5*30 + 0.5*15 = 22.5` —
each step blends the new value with the *previous smoothed output*, rather
than re-normalizing against every raw value seen so far the way
`adjusted = true` does.

## Notes

- Only `Mean`, `Var`, and `Std` are supported.
- `alpha` must be in `(0, 1]`, or it throws.
- A position whose own value is missing is *not* `NULL` in the output — it
  carries forward the smoothed estimate from prior real observations.
- `adjusted = false` requires the source stream to carry no validity mask at
  all — pass `adjusted = true` (the default) if the source might carry a
  mask.
- See the important callout above.

## See Also

- [`StreamTimeSeriesAccessor::expanding`](expanding.md): an alternative
  "everything seen so far" statistic that weights every observation equally
  instead of decaying older ones.
- [`StreamTimeSeriesAccessor::rolling`](rolling.md): a fixed-width
  alternative, rather than a decaying weight over the whole stream.
- [`Stream::ts`](../stream/ts.md): the entry point this accessor is reached from.
- [`TimeSeriesAccessor::ewm`](../slate-ts/ewm.md): the underlying
  single-slate engine, with additional edge-case examples
  (`ignore_nulls`, `adjusted=false` throw).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diff.html">
    <span class="label">Previous</span>
    <span class="title">diff</span>
  </a>
  <a class="next-link" href="expanding.html">
    <span class="label">Next</span>
    <span class="title">expanding</span>
  </a>
</div>

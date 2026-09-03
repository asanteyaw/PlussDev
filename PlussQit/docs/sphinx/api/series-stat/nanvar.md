<!-- pluss-hand-authored -->

# SeriesStatAccessor::nanvar

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::nanvar() const
Series SeriesStatAccessor::nanvar(std::int64_t dim, bool keepdim = false) const
```

## What It Does

`nanvar()` treats IEEE NaNs as missing observations, then computes population
variance from the values left in each lane. It has no sample-estimator toggle.

The policy is precise: one observed value has variance zero; an all-NaN lane
has no defined variance and returns NaN.

## Examples

```cpp
double nan = std::numeric_limits<double>::quiet_NaN();
auto x = pluss::make_series({{1.0, nan, 3.0}, {nan, 5.0, 7.0}});
std::cout << x.stat.nanvar() << "\n";
std::cout << x.stat.nanvar(0) << "\n";
std::cout << x.stat().nanvar(1, true) << "\n";
```

### Output

```text
data=5
data=[0, 0, 4]
data=[[1],
  [1]]
```

Globally, the observed population is `[1,3,5,7]`. Column zero and column one
have only one observation apiece, explaining their zero variances.

## Choosing NaN Omission Deliberately

Use this method when NaN encodes absence, such as an unrecorded sensor sample.
Do not use it to conceal an invalid upstream computation; ordinary `var()`
will preserve that failure signal by propagating NaN.

## Gotchas And Failure Modes

- The denominator is the number of observed values, never the original lane length.
- All-NaN lanes return NaN rather than zero.
- Invalid dimensions fail explicitly.

## See Also

- [`Series::nanvar`](../series/nanvar.md): expanded missing-lane examples.
- [`nanstd`](nanstd.md): same omission policy in original units.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nanstd.html"><span class="label">Previous</span><span class="title">nanstd</span></a>
  <a class="next-link" href="normc.html"><span class="label">Next</span><span class="title">normc</span></a>
</div>

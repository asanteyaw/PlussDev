<!-- pluss-hand-authored -->

# Series::StatAccessor::nanvar

[Back to Series::StatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series Series::StatAccessor::nanvar() const
Series Series::StatAccessor::nanvar(std::int64_t dim, bool keepdim = false) const
Series SeriesStatProxy::nanvar() const
Series SeriesStatProxy::nanvar(std::int64_t dim, bool keepdim = false) const
```

## What It Does

`nanvar()` treats IEEE NaNs as missing observations, then computes population
variance from the values left in each lane. It has no sample-estimator toggle.

The policy is precise: one observed value has variance zero; an all-NaN lane
has no defined variance and returns NaN.

## Parameters And Returns

`dim` selects the independently reduced lane and accepts negative indexing.
`keepdim` retains that axis at size one. The return is a floating-point
population variance; unlike `var()`, there is no sample-estimator overload.

## Examples

Example 1. Global omission forms the observed population `[1,3,5,7]`.

```cpp
double nan = std::numeric_limits<double>::quiet_NaN();
auto x = pluss::make_series({{1.0, nan, 3.0}, {nan, 5.0, 7.0}});
std::cout << x.stat.nanvar() << "\n";
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5
```

Example 2. Column-wise omission gives the columns different observed counts.

```cpp
std::cout << x.stat.nanvar(0) << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 4]
```

The first two columns contain one observed value each, so their population
variance is zero. The final column contains `3` and `7`, whose variance is `4`.

Example 3. The proxy form and `keepdim=true` preserve row alignment.

```cpp
std::cout << x.stat().nanvar(1, true) << "\n";
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [1]]
```

Each row has two observed values separated by two, hence population variance
one.

Example 4. No observed values means the statistic remains undefined.

```cpp
auto none = pluss::make_series({nan, nan});
std::cout << none.stat.nanvar() << "\n";
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=nan
```

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

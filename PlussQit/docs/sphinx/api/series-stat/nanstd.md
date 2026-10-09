<!-- pluss-hand-authored -->

# Series::StatAccessor::nanstd

[Back to Series::StatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series Series::StatAccessor::nanstd() const
Series Series::StatAccessor::nanstd(std::int64_t dim, bool keepdim = false) const
Series SeriesStatProxy::nanstd() const
Series SeriesStatProxy::nanstd(std::int64_t dim, bool keepdim = false) const
```

## What It Does

`nanstd()` omits NaNs and returns the square root of population `nanvar()`.
It expresses spread in the input's original units while preserving the same
missing-data contract.

## Parameters And Returns

`dim` chooses the reduced lane and accepts negative indexing. `keepdim`
retains the reduced axis at size one. The result is the population spread of
observed values in the original units; there is no `unbiased` option.

## Examples

Example 1. Global `nanstd()` is the square root of global `nanvar()`.

```cpp
double nan = std::numeric_limits<double>::quiet_NaN();
auto x = pluss::make_series({{1.0, nan, 3.0}, {nan, 5.0, 7.0}});
std::cout << x.stat.nanstd() << "\n";
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=2.236068
```

Example 2. Column-wise output exposes the one-observation rule.

```cpp
std::cout << x.stat.nanstd(0) << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 2]
```

Example 3. `-1` selects rows and `keepdim=true` keeps a `[2,1]` result.

```cpp
std::cout << x.stat().nanstd(-1, true) << "\n";
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [1]]
```

The global value is `sqrt(5)`. The column result makes the singleton rule
visible: a lane with one observed value has zero population spread.

Example 4. Ordinary spread propagates missingness; `nanstd()` deliberately
omits it.

```cpp
std::cout << x.stat.std(1, false) << "\n";
std::cout << x.stat.nanstd(1) << "\n";
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[nan, nan]
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

## Interpretation

NaN omission changes each lane's effective sample count. Comparing two output
lanes therefore compares spread computed from potentially different amounts of
evidence. Carry a count mask alongside the result when that difference matters.

## Gotchas And Failure Modes

- `nanstd()` is population standard deviation; there is no `unbiased` argument.
- An all-NaN lane remains NaN.
- Omission is a data policy, not a generic numerical-error recovery mechanism.

## See Also

- [`Series::nanstd`](../series/nanstd.md): full root-surface examples.
- [`nanvar`](nanvar.md): the squared-unit form.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="movvar.html"><span class="label">Previous</span><span class="title">movvar</span></a>
  <a class="next-link" href="nanvar.html"><span class="label">Next</span><span class="title">nanvar</span></a>
</div>

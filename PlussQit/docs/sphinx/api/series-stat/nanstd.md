<!-- pluss-hand-authored -->

# SeriesStatAccessor::nanstd

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::nanstd() const
Series SeriesStatAccessor::nanstd(std::int64_t dim, bool keepdim = false) const
```

## What It Does

`nanstd()` omits NaNs and returns the square root of population `nanvar()`.
It expresses spread in the input's original units while preserving the same
missing-data contract.

## Examples

```cpp
double nan = std::numeric_limits<double>::quiet_NaN();
auto x = pluss::make_series({{1.0, nan, 3.0}, {nan, 5.0, 7.0}});
std::cout << x.stat.nanstd() << "\n";
std::cout << x.stat.nanstd(0) << "\n";
std::cout << x.stat().nanstd(-1, true) << "\n";
```

### Output

```text
data=2.236068
data=[0, 0, 2]
data=[[1],
  [1]]
```

The global value is `sqrt(5)`. The column result makes the singleton rule
visible: a lane with one observed value has zero population spread.

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

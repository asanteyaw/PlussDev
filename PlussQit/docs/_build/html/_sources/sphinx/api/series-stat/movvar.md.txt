<!-- pluss-hand-authored -->

# SeriesStatAccessor::movvar

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::movvar() const
Series SeriesStatAccessor::movvar(std::int64_t window) const
Series SeriesStatAccessor::movvar(std::int64_t window, std::int64_t dim) const
```

## What It Does

`movvar()` computes population variance inside every trailing window while
preserving shape. The default window is three on the last axis.

Population is important here: each window divides squared deviations by its
actual number of values, not by `N-1`. The first startup window contains one
value and therefore always has variance zero.

## Examples

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
std::cout << x.stat.movvar() << "\n";
std::cout << x.stat().movvar(2) << "\n";
```

### Output

```text
data=[0, 0.25, 0.66666667, 0.66666667]
data=[0, 0.25, 0.25, 0.25]
```

For window three, the startup pair `[1,2]` has mean `1.5` and population
variance `(0.25+0.25)/2 = 0.25`. Full windows `[1,2,3]` and `[2,3,4]` both have
population variance `2/3`.

## Gradient

Within a window of actual length `N`, the derivative for member `x_i` is
`2(x_i-window_mean)/N`. Reverse mode accumulates that term from every trailing
window containing the input. Constant windows send zero sensitivity because
all centered deviations are zero.

## Choosing Moving Spread

Use `movvar()` when squared units are useful in later algebra, such as variance
addition or volatility objectives. Use `movstd()` when readers need spread in
the original units. Neither method uses the sample estimator.

## Gotchas And Failure Modes

- Partial prefixes use their actual length.
- There is no `unbiased` argument.
- `window` must be positive; omitted `dim` selects the last axis.

## See Also

- [`movstd`](movstd.md): square root of the same trailing population variance.
- [`var`](var.md): reduce a lane to one variance.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="movsum.html"><span class="label">Previous</span><span class="title">movsum</span></a>
  <a class="next-link" href="nanstd.html"><span class="label">Next</span><span class="title">nanstd</span></a>
</div>

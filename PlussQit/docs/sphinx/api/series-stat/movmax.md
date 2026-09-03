<!-- pluss-hand-authored -->

# SeriesStatAccessor::movmax

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::movmax() const
Series SeriesStatAccessor::movmax(std::int64_t window) const
Series SeriesStatAccessor::movmax(std::int64_t window, std::int64_t dim) const
```

## What It Does

`movmax()` emits the largest value in each trailing window. Shape is preserved,
the current position is included, startup windows use the available prefix,
and the default is window three on the last axis.

This operation is often used as a rolling high-water mark or as one-dimensional
morphological dilation. It does not report the index of the winning value.

## Examples

```cpp
auto rising = pluss::make_series({1.0, 2.0, 3.0, 4.0});
auto mixed = pluss::make_series({3.0, 1.0, 4.0, 2.0, 0.0});
std::cout << rising.stat.movmax() << "\n";
std::cout << mixed.stat().movmax(3) << "\n";
```

### Output

```text
data=[1, 2, 3, 4]
data=[3, 3, 4, 4, 4]
```

The final output remains `4` because the trailing windows at positions three
and four still contain the value at position two.

## Gradient And Ties

Reverse mode sends an output's sensitivity to its maximizing input. If several
values tie for the maximum in the same window, sensitivity is split evenly
among those ties. An input can accumulate contributions from several outputs.

That tie split is a deliberate subgradient convention: the mathematical
maximum is nondifferentiable where winners tie.

## Gotchas And Failure Modes

- `movmax()` is causal/trailing; future values never affect earlier outputs.
- Tied maxima share gradients rather than choosing the first occurrence.
- `window` must be positive.

## See Also

- [`movmin`](movmin.md): trailing low-water mark.
- [`movmean`](movmean.md): smooth every value rather than select a winner.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mean.html"><span class="label">Previous</span><span class="title">mean</span></a>
  <a class="next-link" href="movmean.html"><span class="label">Next</span><span class="title">movmean</span></a>
</div>

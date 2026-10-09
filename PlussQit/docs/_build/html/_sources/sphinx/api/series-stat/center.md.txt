<!-- pluss-hand-authored -->

# Series::StatAccessor::center

[Back to Series::StatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series Series::StatAccessor::center() const
Series Series::StatAccessor::center(std::int64_t dim) const
Series SeriesStatProxy::center() const
Series SeriesStatProxy::center(std::int64_t dim) const
```

## What It Does

`center()` subtracts a mean without changing shape. With no axis it subtracts
the global mean from every element. With `dim`, each lane gets its own mean.
The transformed lanes have mean zero, apart from floating-point rounding.

Centering changes location but not scale: pairwise differences, variance, and
standard deviation within each centered lane remain unchanged.

## Choosing The Axis

For a matrix, `center(0)` removes each column mean and is common when columns
are features. `center(1)` removes each row mean and is useful when each row is
an independently offset signal. Global `center()` treats every element as one
population; it is not interchangeable with either axis-wise form.

## Parameters And Returns

`dim` selects the lanes whose means are removed and accepts negative indexing.
There is no `keepdim` option because centering broadcasts the means back and
always returns the owner's original shape. The operation is out-of-place.

## Examples

Example 1. Global centering subtracts the single mean `3.5` everywhere.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
std::cout << x.stat.center() << "\n";
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-2.5, -1.5, -0.5],
  [0.5, 1.5, 2.5]]
```

Example 2. Column centering subtracts means `2.5`, `3.5`, and `4.5`.

```cpp
std::cout << x.stat.center(0) << "\n";
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1.5, -1.5, -1.5],
  [1.5, 1.5, 1.5]]
```

Example 3. Row centering subtracts `2` from the first row and `5` from the
second. The explicit proxy reaches the same transform.

```cpp
std::cout << x.stat().center(1) << "\n";
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1, 0, 1],
  [-1, 0, 1]]
```

In the final output, adding a different constant to either input row would not
change that row's centered values.

## Gradient

Reverse mode removes the mean component from each upstream lane. For a lane of
length `N`, `dx_i = g_i - mean(g)`. Consequently, summing a centered lane gives
zero and sends zero sensitivity to its input.

## Gotchas And Failure Modes

- `center()` does not divide by spread; use `standardize()` for that.
- Axis selection defines which offsets are removed.
- Negative dimensions count from the back.

## See Also

- [`Series::center`](../series/center.md): expanded root-surface examples.
- [`standardize`](standardize.md): zero mean plus unit population variance.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cdist.html"><span class="label">Previous</span><span class="title">cdist</span></a>
  <a class="next-link" href="kurtosis.html"><span class="label">Next</span><span class="title">kurtosis</span></a>
</div>

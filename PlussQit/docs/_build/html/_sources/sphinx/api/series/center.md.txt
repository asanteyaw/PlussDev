<!-- pluss-hand-authored -->

# Series::center

[Back to Series::StatAccessor API Reference](../../series_stat_accessor_api_reference)

## Signature

```cpp
Series::center() const -> Series
Series::center(std::int64_t dim) const -> Series
Series::stat.center(...) -> Series
Series::stat().center(...) -> Series
```

## What It Does

`center()` subtracts a mean and preserves the original shape.

That one sentence is the heart of the method:

- it removes location
- it does **not** remove scale
- it does **not** collapse dimensions

That makes it a transformation method rather than a summary statistic.

Compare the roles:

- [`mean`](mean.md) tells you the offset
- `center()` removes the offset

So the real question answered by `center()` is:

> What do these values look like once their average has been removed?

This is a good first normalization step whenever absolute level is less
important than deviation around a local center.

## Overload Map

**`center()`**

Subtract one global mean from the entire `Series`.

**`center(dim)`**

Subtract one mean per lane along the chosen axis, then broadcast that lane mean
back over the original positions.

**`stat.center(...)` / `stat().center(...)`**

Equivalent accessor spellings. The accessor does not change the semantics; it
just groups related statistical operations on a shared public surface.

## Parameters

**dim** (`std::int64_t`)

The axis whose lane means should be removed.

Important details:

- `dim = 0` means "work down the rows / along the first axis"
- `dim = 1` means "work across the columns / along the second axis"
- negative dims count from the back, so `-1` means the last axis
- centering happens independently inside each lane selected by that axis

There is no `keepdim` flag because `center()` is not a reduction. The output
already keeps the original shape.

## Returns

`Series`
    A new `Series` with the same shape as the input, but with the chosen mean
    removed.

## Gradient

Centering is a linear transform, so it fits naturally inside ordinary
differentiable numeric pipelines.

## Examples

The examples below are intentionally progressive. They start with the global
form, move to axis-aware centering, then show accessor spellings and edge
behavior.

Unless stated otherwise, they use:

```cpp
auto values = pluss::make_series({1, 2, 3, 4, 5, 6}, {2, 3});
```

Example 1. Global centering removes one mean from the whole series.

```cpp
std::cout << values.center();
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-2.5, -1.5, -0.5],
  [0.5, 1.5, 2.5]]
```

The global mean is `3.5`, so every element is shifted by that same amount.

Example 2. Row-wise centering removes one mean per row.

```cpp
std::cout << values.stat.center(1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1, 0, 1],
  [-1, 0, 1]]
```

Each row is centered independently:

- row `0` subtracts `2`
- row `1` subtracts `5`

This is often the most useful form in practice because each row becomes locally
mean-zero. The direct `values.center(1)` spelling follows the same semantics;
this example uses the verified accessor call.

Example 3. The accessor spelling `stat().center(dim)` reaches the same
operation.

```cpp
std::cout << values.stat().center(1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1, 0, 1],
  [-1, 0, 1]]
```

This matters mostly for discoverability and style consistency. It is the same
transformation as Example 2.

Example 4. Column-wise centering produces a different pattern because the lanes
changed.

```cpp
std::cout << values.center(0);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1.5, -1.5, -1.5],
  [1.5, 1.5, 1.5]]
```

Now each column is centered independently:

- column `0` subtracts `2.5`
- column `1` subtracts `3.5`
- column `2` subtracts `4.5`

Example 5. Negative dimensions count from the back, so `-1` means the last
axis.

```cpp
std::cout << values.center(-1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1, 0, 1],
  [-1, 0, 1]]
```

On this shape, the last axis is the row axis, so the result matches Example 2.

Example 6. A scalar `Series` centers to zero.

```cpp
std::cout << pluss::make_series(42.0).center();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0
```

That is a useful sanity check: if a `Series` has only one value, its mean is
the value itself, so centering removes everything and leaves zero.

## Gotchas And Failure Modes

- `center()` always preserves shape. If you want a smaller result, you want a
  reduction such as [`mean`](mean.md), not `center()`.
- Global centering and axis-wise centering can produce very different outputs.
  Choosing the wrong `dim` is one of the easiest mistakes to make.
- The centered values along the chosen lanes sum to zero up to ordinary floating
  arithmetic effects. That is one of the quickest ways to sanity-check the
  result.
- A `dim` outside the valid axis range fails.
- `center()` removes offset only. If you also want unit-scale output, use
  [`standardize`](standardize.md).

## Summary

Use `center()` when you want to preserve shape but remove average level:

- `center()` removes one global mean
- `center(dim)` removes one mean per lane
- the accessor spellings are the same operation, just reached through `stat`

## See Also

- [`Series::standardize`](standardize.md): centering plus scale normalization.
- [`Series::mean`](mean.md): the reduction that computes the mean itself.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nanstd.html"><span class="label">Previous</span><span class="title">nanstd</span></a>
  <a class="next-link" href="standardize.html"><span class="label">Next</span><span class="title">standardize</span></a>
</div>

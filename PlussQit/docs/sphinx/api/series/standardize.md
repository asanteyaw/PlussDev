<!-- pluss-hand-authored -->

# Series::standardize

[Back to Series::StatAccessor API Reference](../../series_stat_accessor_api_reference)

## Signature

```cpp
Series Series::standardize() const
Series Series::standardize(std::int64_t dim) const
Series Series::stat.standardize(...)
Series Series::stat().standardize(...)
```

## What It Does

`standardize()` centers the data and divides by its spread while preserving the
original shape.

This is the higher-level normalization counterpart to [`center`](center.md):

- `center()` removes only location
- `standardize()` removes location and rescales dispersion

In other words, standardization asks for more than "shift the values around
zero." It asks for "shift them around zero and put them on a comparable spread."

That makes `standardize()` a stronger next step than `center()`:

- `center()` tells you deviations from the mean
- `standardize()` tells you deviations from the mean in spread-scaled units

## Overload Map

**`standardize()`**

Apply one global mean and one global spread to the entire `Series`.

**`standardize(dim)`**

Apply one mean and one spread per lane along the chosen axis, then broadcast
that normalization back across the original shape.

**`stat.standardize(...)` / `stat().standardize(...)`**

Equivalent accessor spellings.

There is no `keepdim` flag because the transform preserves shape.

## Parameters

**dim** (`std::int64_t`)

The axis whose lanes should be normalized independently.

Important details:

- `dim = 0` standardizes column-wise on a 2D `Series`
- `dim = 1` standardizes row-wise on a 2D `Series`
- negative dims count from the back, so `-1` means the last axis
- each lane gets its own mean and its own spread

## Returns

`Series`
    A new `Series` with the same shape as the input, but centered and rescaled
    along the chosen scope.

## Gradient

This is a floating normalization transform intended to participate in ordinary
numeric pipelines.

## Examples

The examples below are arranged to introduce the two big choices:

- global versus lane-wise normalization
- which lane direction `dim` is actually selecting

Unless stated otherwise, they use:

```cpp
auto values = pluss::make_series({1, 2, 3, 4, 5, 6}, {2, 3});
```

Example 1. Global standardization uses one mean and one spread for the entire
series.

```cpp
std::cout << values.standardize();
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1.4638501, -0.87831007, -0.29277002],
  [0.29277002, 0.87831007, 1.4638501]]
```

Everything is measured relative to one global center and one global spread, so
the two rows do **not** end up looking identical.

Example 2. Row-wise standardization normalizes each row independently.

```cpp
std::cout << values.stat.standardize(1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1.2247449, 0, 1.2247449],
  [-1.2247449, 0, 1.2247449]]
```

Each row is normalized by its own mean and spread, so the rows now share the
same normalized shape. The direct `values.standardize(1)` spelling follows the
same semantics; this example uses the verified accessor call.

Example 3. The accessor spelling `stat().standardize(dim)` reaches the same
transformation.

```cpp
std::cout << values.stat().standardize(1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1.2247449, 0, 1.2247449],
  [-1.2247449, 0, 1.2247449]]
```

This is the same result as Example 2. The accessor is organizational, not a
different algorithm.

Example 4. Column-wise standardization gives a different answer because the
lanes changed.

```cpp
std::cout << values.standardize(0);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1, -1, -1],
  [1, 1, 1]]
```

Each column has only two values here, so each column becomes a simple `[-1, 1]`
pattern after standardization.

Example 5. Negative dimensions count from the back, so `-1` means the last
axis.

```cpp
std::cout << values.standardize(-1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1.2247449, 0, 1.2247449],
  [-1.2247449, 0, 1.2247449]]
```

On this shape, the last axis is the row axis, so the result matches Example 2.

Example 6. Constant lanes are handled stably rather than exploding.

```cpp
auto constant_rows = pluss::make_series({2, 2, 2, 5, 5, 5}, {2, 3});
std::cout << constant_rows.standardize(1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0, 0, 0],
  [0, 0, 0]]
```

This is an important verified behavior:

- constant lanes do not turn into NaNs
- they collapse cleanly to zeros

## Reading The Numbers

Look again at the row-wise output from Example 2:

```text
data=[[-1.2247449, 0, 1.2247449],
  [-1.2247449, 0, 1.2247449]]
```

For the row `[1, 2, 3]`, that magnitude corresponds to dividing by
`sqrt(2/3)`, not by sample standard deviation `1`.

That matters if you are checking results by hand: the current verified behavior
is population-style normalization on this surface.

## Gotchas And Failure Modes

- `standardize()` preserves shape. If you expected fewer dimensions, you wanted
  a reduction, not a transform.
- Choosing the wrong `dim` changes the meaning substantially. Row-wise and
  column-wise standardization are often both "reasonable" but answer different
  questions.
- Constant lanes normalize to zero rather than failing or producing NaNs. This
  is convenient, but it also means "all zeros" can mean "no spread was present."
- A `dim` outside the valid axis range fails.
- The current verified behavior is population-style normalization.

## Summary

Use `standardize()` when both offset and spread should be removed while shape is
preserved:

- `standardize()` uses one global mean and spread
- `standardize(dim)` uses one mean and spread per lane
- constant lanes collapse to zero on the current public surface

## See Also

- [`Series::center`](center.md): mean removal without scale normalization.
- [`Series::std`](std.md): report the spread instead of applying it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="center.html"><span class="label">Previous</span><span class="title">center</span></a>
  <a class="next-link" href="skewness.html"><span class="label">Next</span><span class="title">skewness</span></a>
</div>

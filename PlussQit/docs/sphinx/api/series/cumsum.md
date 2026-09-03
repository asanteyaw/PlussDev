<!-- pluss-hand-authored -->

# Series::cumsum

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::cumsum(std::int64_t dim) const -> Series
Series::cumsum(std::int64_t dim, bool keepdim) const -> Series
Series& Series::cumsum_(std::int64_t dim)
Series& Series::cumsum_(std::int64_t dim, bool keepdim)
```

## What It Does

Computes a cumulative sum along one axis.

Unlike [`sum()`](sum.md), `cumsum()` does **not** reduce away an axis. It keeps
the full original shape and replaces each position with the running total up to
that position along `dim`.

For a row `[1, 2, 3]`, the rowwise cumulative sum is `[1, 3, 6]`.

That prefix idea is the important mental model:

- position `0` sees only the first value
- position `1` sees the first two values
- position `2` sees the first three values

Unlike a reduction, every intermediate running total survives in the output.

## Parameters

**dim** (`std::int64_t`)

The axis along which the running total is accumulated.

Important details:

- `dim = 0` scans down the first axis
- `dim = 1` scans across the second axis
- negative dims count from the back
- the result keeps the same shape because a scan does not remove axes

**keepdim** (`bool`)

Accepted for API parity, but in the current implementation it does not affect
the result shape because a cumulative scan already preserves shape.

## Returns

`Series` or `Series&`
    The prefix-summed result, either as a fresh value or as the mutated current
    series for the underscore overloads.

## Gradient

The out-of-place `cumsum()` is differentiable.

Its backward is a reverse cumulative sum along the same axis. Intuitively:

- an early input affects many later outputs
- a late input affects fewer outputs

That is why, when you reduce the scanned result to a scalar loss, the
sensitivity is largest at the front of each lane and decreases toward the end.

## Examples

The examples on this page build incrementally.

Example 1. `dim = 1` computes rowwise running totals.

```cpp
auto mat = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << mat.cumsum(1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 3, 6],
  [4, 9, 15]]
```

Each row is scanned independently.

Example 2. Changing the axis changes the accumulation direction completely.

```cpp
std::cout << mat.cumsum(0);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2, 3],
  [5, 7, 9]]
```

Now the running total is vertical rather than horizontal.

Example 3. Negative dimensions count from the back, so `-1` means the last
axis.

```cpp
std::cout << mat.cumsum(-1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 3, 6],
  [4, 9, 15]]
```

On this shape, that matches `cumsum(1)` because the last axis is the row-local
column axis.

Example 4. `keepdim` is currently accepted but shape-neutral.

```cpp
std::cout << (mat.cumsum(1, false).sizes() == mat.cumsum(1, true).sizes() ? "true" : "false");
```

### Output

```text
true
```

That is not a coincidence. A scan already preserves rank and extent, so there
is no axis to reinsert.

Example 5. The in-place overload mutates the current `Series` but keeps the same
scan semantics.

```cpp
auto inplace = mat.clone();
inplace.cumsum_(1);

std::cout << inplace;
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 3, 6],
  [4, 9, 15]]
```

Use the underscore form when you explicitly want to overwrite the source
storage.

Example 6. The backward pass is a reverse scan of the upstream sensitivity.

```cpp
auto tracked_cumsum = mat.clone();
tracked_cumsum.tracks_slope_(true);

auto scanned_sum = tracked_cumsum.cumsum(1);
scanned_sum.sum().reverse();

std::cout << "y=" << scanned_sum << "\n";
std::cout << "dx=" << tracked_cumsum.sensitivity();
```

### Output

```text
y=pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 3, 6],
  [4, 9, 15]]
dx=pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[3, 2, 1],
  [3, 2, 1]]
```

The first element in each row contributes to all three scanned outputs, so it
receives derivative `3`. The last element contributes only to the last output,
so it receives derivative `1`.

## Gotchas And Failure Modes

- `cumsum()` is a scan, not a reduction.
- `keepdim` is currently shape-neutral.
- The underscore overloads mutate storage but preserve the same scan semantics.
- Choosing the wrong axis changes the meaning completely. Horizontal and
  vertical scans answer different questions.
- A `dim` outside the valid axis range fails.

## Summary

Use `cumsum()` when every running total matters, not just the final one:

- `cumsum(dim)` preserves shape and stores all prefixes
- `keepdim` is currently accepted but does not change the shape
- `cumsum_` mutates the current `Series`
- backward sensitivity runs in the reverse prefix direction

## See Also

- [`Series::sum`](sum.md): collapses an axis instead of preserving every prefix.
- [`Series::cumprod`](cumprod.md): multiplicative prefix scan.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cumprod.html">
    <span class="label">Previous</span>
    <span class="title">cumprod</span>
  </a>
  <a class="next-link" href="deg2rad.html">
    <span class="label">Next</span>
    <span class="title">deg2rad</span>
  </a>
</div>

<!-- pluss-hand-authored -->

# Series::triu

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::triu() const
Series Series::triu(std::int64_t diagonal) const
```

## What It Does

`triu()` keeps the upper-triangular region of the trailing matrix dimensions
and zeroes everything below the chosen diagonal.

It is the upper-side companion to [`tril()`](tril.md), and it follows the same
shape rule:

- rank 2 means one matrix
- rank 3+ means a batch over the last two axes
- rank 0 or 1 is invalid

## Overload Map

**`triu()`**

Equivalent to `triu(0)`, which keeps the main diagonal and everything above it.

**`triu(std::int64_t diagonal)`**

Shifts the keep boundary.

- positive values remove more of the lower-left region
- negative values keep one or more diagonals below the main diagonal too

## Parameters

**diagonal** (`std::int64_t`)

The lowest diagonal that remains non-zero.

## Returns

`Series`
    A new `Series` with the same shape and dtype as the input, with the
    forbidden lower-triangular positions replaced by zero.

## Gradient

`triu()` shares the same verified triangular masking rule as [`tril()`](tril.md),
but mirrored across the diagonal.

So reverse-mode keeps sensitivity in the upper-triangular region that survived
the forward mask and drops it in the zeroed region below.

## Examples

Example 1. The default form keeps the main diagonal and everything above it.

```cpp
auto square = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9},
}, pluss::ScalarType::Int64);

std::cout << square.triu();
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=int64, platform=cpu, numel=9, contiguous=true)
data=[[1, 2, 3],
  [0, 5, 6],
  [0, 0, 9]]
```

Example 2. A negative offset keeps one diagonal below the main diagonal too.

```cpp
std::cout << square.triu(-1);
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=int64, platform=cpu, numel=9, contiguous=true)
data=[[1, 2, 3],
  [4, 5, 6],
  [0, 8, 9]]
```

Example 3. Rank-1 input fails because the method needs a matrix-shaped tail.

```cpp
try {
  std::cout << pluss::make_series({1, 2, 3}, pluss::ScalarType::Int64).triu() << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
tril/triu: expected tensor with at least 2 dimensions
```

## Gotchas And Failure Modes

- `triu()` operates on trailing matrix axes, not necessarily on the first two
  axes of a higher-rank object.
- Negative offsets keep more values below the main diagonal.
- Rank-0 and rank-1 inputs are invalid.
- Reverse-mode uses the same upper-triangular keep-mask as the forward pass.

## Summary

Use `triu()` for upper-triangular masking with copy-returning semantics. If the
result should replace the current value in place, use [`triu_()`](triu_.md)
instead.

## See Also

- [`Series::tril`](tril.md): lower-triangular companion.
- [`Series::triu_`](triu_.md): in-place upper-triangular masking.
- [`Series::diag`](diag.md): diagonal-only extract-or-embed surface.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tril_.html">
    <span class="label">Previous</span>
    <span class="title">tril_</span>
  </a>
  <a class="next-link" href="triu_.html">
    <span class="label">Next</span>
    <span class="title">triu_</span>
  </a>
</div>

<!-- pluss-hand-authored -->

# Series::unsqueeze

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::unsqueeze(std::int64_t dim) const -> Series
```

## What It Does

Inserts a size-`1` axis at `dim` and returns the result as a view.

`unsqueeze()` does not change the values in the `Series` and does not change
`numel()`. It only changes rank and indexing structure. In practice this makes
it one of the main broadcasting-preparation tools: if another operation expects
an axis that your current `Series` does not explicitly have, `unsqueeze()` is
how you add it without copying data.

## Parameters

**dim** (`std::int64_t`)

Where the new axis should be inserted.

Valid positions are in `[0, self.dim() + 1]` after negative-dimension
normalization. So for a 2-D series:

- `0` inserts a new leading axis
- `1` inserts between the two existing axes
- `2` or `-1` inserts at the end

## Returns

`Series`
    A view with one additional axis of length `1`.

## Gradient

`unsqueeze()` is differentiable. Backward simply reshapes the outgoing
sensitivity back to the original shape, because no values were duplicated or
dropped in the forward pass.

This is why `unsqueeze()` is safe to use freely in broadcast preparation: it is
pure shape bookkeeping, not a numerical transformation.

## Examples

The examples on this page build incrementally.

Example 1. Insert a leading axis.

```cpp
auto mat = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});
std::cout << mat.unsqueeze(0);
```

### Output

```text
pluss::Series(shape=[1,2,3], strides=[6,3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[[1, 2, 3],
  [4, 5, 6]]]
```

Nothing numerical changed. The same `2 x 3` payload is now wrapped in an outer
axis of length `1`.

Example 2. Inserting in the middle is often the most useful form for
broadcast-alignment code.

```cpp
std::cout << mat.unsqueeze(1);
```

### Output

```text
pluss::Series(shape=[2,1,3], strides=[3,3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[[1, 2, 3]],
  [[4, 5, 6]]]
```

This turns each row into a length-`1` group containing a `3`-element lane.

Example 3. Negative indexing can be used to append the axis at the end.

```cpp
std::cout << mat.unsqueeze(-1);
```

### Output

```text
pluss::Series(shape=[2,3,1], strides=[3,1,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[[1],
  [2],
  [3]],
  [[4],
  [5],
  [6]]]
```

This is the common "turn each scalar into a length-1 feature axis" form.

## Notes

- `unsqueeze()` always inserts exactly one axis.
- The new axis always has length `1`.
- The result is a view, not a copy.

## See Also

- [`Series::squeeze`](squeeze.md): remove size-`1` axes again.
- [`Series::permute`](permute.md): reorder axes after rank alignment.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="permute.html">
    <span class="label">Previous</span>
    <span class="title">permute</span>
  </a>
  <a class="next-link" href="squeeze.html">
    <span class="label">Next</span>
    <span class="title">squeeze</span>
  </a>
</div>

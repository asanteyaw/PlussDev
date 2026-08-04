<!-- pluss-hand-authored -->

# Series::unsqueeze

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::unsqueeze(std::int64_t dim) const -> Series
```

## What It Does

Inserts a size-`1` axis at `dim` and returns the result as a view.

`unsqueeze()` does not change which values exist or how many values there are.
It changes only rank and indexing structure. This is the standard shape-alignment
tool for broadcasting: if one operand is missing an axis that another operand
expects, `unsqueeze()` is how you make that axis explicit without copying data.

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

Example 1. Insert a leading axis.

```cpp
auto mat = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3});
std::cout << mat.unsqueeze(0);
```

### Output

```text
pluss::Series(shape=[1,2,3], strides=[6,3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[[1, 2, 3],
  [4, 5, 6]]]
```

Nothing numerical changed; the same `2 x 3` payload is now wrapped in an outer
batch-like axis of length `1`.

Example 2. Negative indexing can be used to append the axis at the end.

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

<!-- pluss-hand-authored -->

# Series::view

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::view(pluss::msharp::RangeView::Int new_sizes) const -> Series
```

## What It Does

Reinterprets the same underlying storage under a new shape without copying.

`view()` is the strict shape-reinterpretation surface. You use it when the
no-copy property is part of the contract, not just a performance preference.
If the requested shape cannot be expressed as a direct alias, `view()` fails
instead of quietly allocating a normalized copy.

In the current implementation that strictness is intentionally conservative:

- the source must be contiguous
- the requested shape must preserve total `numel()`

If you want "give me this shape, copy if needed," the right method is
[`reshape()`](reshape.md), not `view()`.

## Parameters

**new_sizes** (`pluss::msharp::RangeView::Int`)

The desired output shape.

In ordinary use you can pass brace literals such as `{3, 2}` directly, or reuse
another `Series`’s [`sizes()`](sizes.md) result.

## Returns

`Series`
    A new `Series` handle over the same storage, with the requested logical
    shape.

## Autograd Behavior

`view()` preserves reverse-mode connectivity. Gradients flowing into the view are
reshaped back to the original shape on the way to the source.

That is the right mental model: `view()` changes only how the same values are
indexed, so the backward path only has to restore the original shape metadata.

## Examples

The examples on this page build incrementally.

Example 1. Reinterpret a contiguous `2 x 3` series as `3 x 2`.

```cpp
auto mat = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << mat.view({3, 2});
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2],
  [3, 4],
  [5, 6]]
```

This is not a transpose. The flat storage order stayed `[1, 2, 3, 4, 5, 6]`;
you simply asked to reread that same sequence under a different shape.

Example 2. `new_sizes` can be borrowed from another tensor's shape metadata, not
just spelled out manually.

```cpp
auto shape_template = pluss::make_series({
    {10.0, 20.0},
    {30.0, 40.0},
    {50.0, 60.0},
});

std::cout << mat.view(shape_template.sizes());
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2],
  [3, 4],
  [5, 6]]
```

What matters is not that the shape came from another tensor. What matters is
that `[3, 2]` still has the same `numel()` as `mat`.

Example 3. A non-contiguous input is rejected instead of being copied.

```cpp
try {
  auto transposed = mat.transpose(0, 1);
  std::cout << transposed.view({3, 2});
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
view: requires contiguous in v0
```

That failure is deliberate. Once the source stops being directly viewable,
`view()` forces you to make an explicit choice to use [`reshape()`](reshape.md)
instead.

Example 4. A `numel()` mismatch also fails.

```cpp
try {
  std::cout << mat.view({4, 2});
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
view: requires contiguous in v0
```

This is a current implementation quirk worth knowing: contiguity failure and
shape incompatibility collapse to the same top-line error text because both are
checked through the same view-eligibility gate.

## Notes

- `view()` is the no-copy contract, not the forgiving one.
- Shape compatibility still requires equal total `numel()`.
- In the current implementation, contiguity is mandatory even if a more general
  stride reinterpretation could theoretically exist.

## See Also

- [`Series::reshape`](reshape.md): same target shape, but copy-capable.
- [`Series::sizes`](sizes.md): a convenient way to feed shape metadata into
  `view()`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sizes.html">
    <span class="label">Previous</span>
    <span class="title">sizes</span>
  </a>
  <a class="next-link" href="reshape.html">
    <span class="label">Next</span>
    <span class="title">reshape</span>
  </a>
</div>

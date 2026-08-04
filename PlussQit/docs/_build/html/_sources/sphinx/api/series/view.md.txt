<!-- pluss-hand-authored -->

# Series::view

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::view(pluss::msharp::RangeView::Int new_sizes) const -> Series
```

## What It Does

Reinterprets the same underlying storage under a new shape **without copying**.

`view()` is the strict shape-reinterpretation surface. It is for the case where
you do not want "reshape semantics" in the abstract; you want a genuine no-copy
alias over the same storage, and you want the call to fail if that is not
possible.

In the current implementation, that strictness is intentionally conservative:

- the source must be contiguous
- the requested shape must have the same total `numel()`

If either condition fails, `view()` throws instead of silently materializing a
contiguous buffer. That boundary is the entire point of the method. If you want
"same logical result, copy if needed," use [`reshape()`](reshape.md) instead.

## Parameters

**new_sizes** (`pluss::msharp::RangeView::Int`)

The desired output shape.

This parameter is shape-like rather than container-specific. In practice that
means you can feed it from a `std::span<const std::int64_t>`, a `std::array`, a
`std::vector`, or another `Series`'s [`sizes()`](sizes.md) view, as long as the
borrowed range stays valid for the call.

## Returns

`Series`
    A new `Series` handle over the same storage, with `new_sizes` and
    contiguous strides computed for that new logical shape.

## Autograd Behavior

`view()` preserves reverse-mode connectivity. Gradients flowing into the view are
reshaped back to the original shape on the way to the source.

That is the right mental model: `view()` changes only how the same values are
indexed, so the backward path only has to restore the original shape metadata.

## Examples

Example 1. Reinterpret a contiguous `2 x 3` series as `3 x 2`.

```cpp
auto mat = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3});
const std::array<std::int64_t, 2> shape_3x2{{3, 2}};

std::cout << mat.view(
    std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size()));
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2],
  [3, 4],
  [5, 6]]
```

This is not a transpose. The flat storage order stayed `[1, 2, 3, 4, 5, 6]`;
you simply asked to read that same sequence as `3 x 2` instead of `2 x 3`.

Example 2. `new_sizes` can be borrowed from another tensor's shape metadata, not
just spelled out manually.

```cpp
auto shape_template =
    pluss::make_series(std::vector<double>{10, 20, 30, 40, 50, 60}, {3, 2});

std::cout << mat.view(shape_template.sizes());
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2],
  [3, 4],
  [5, 6]]
```

The important contract is not "borrowed from another tensor"; it is "same
number of elements." `shape_template.sizes()` works here because `[3, 2]` still
has `6` elements.

Example 3. A non-contiguous input is rejected instead of being copied.

```cpp
try {
  auto transposed = mat.transpose(0, 1);
  std::cout << transposed.view(
      std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size()));
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
`view()` stays strict and forces you to choose [`reshape()`](reshape.md) if you
actually want fallback materialization.

## Notes

- `view()` is the no-copy contract, not the forgiving one.
- Shape compatibility still requires equal total `numel()`.
- In the current implementation, contiguity is mandatory even if a more general
  stride reinterpretation might theoretically exist.

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

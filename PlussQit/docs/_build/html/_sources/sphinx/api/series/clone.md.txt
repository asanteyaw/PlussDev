<!-- pluss-hand-authored -->

# Series::clone

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::clone() const -> Series
```

## What It Does

`clone()` makes a new `Series` with the same visible values, shape, dtype, and
platform as the receiver.

The important public guarantee is storage independence. Unlike no-op
`to(...)` calls or `contiguous()` on an already contiguous series, `clone()`
does not return a same-buffer alias. It materializes a separate result.

That makes `clone()` the right choice when you want to:

- preserve the current value before an in-place write
- break buffer sharing between two variables
- materialize a view as an ordinary standalone series

## Parameters

None.

This is a copy-producing operation on `self`.

## Returns

`Series`
    A new `Series` with the same visible content as `self`.

If `self` is a view, the clone keeps the view's visible order and shape. It
does not revert to the source storage's original layout.

## Gradient

`clone()` preserves reverse-mode connectivity on the current verified surface.

That means cloning a tracked series is safe in autodiff pipelines: later
reverse passes still route sensitivity back to the original source.

## Examples

Example 1. Mutating a clone does not mutate the original.

```cpp
auto clone_base = pluss::make_series({1.0, 2.0, 3.0});
auto clone_copy = clone_base.clone();
clone_copy.add_(100.0);

std::cout << "base=" << clone_base << "\n";
std::cout << "copy=" << clone_copy << "\n";
```

### Output

```text
base=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
copy=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[101, 102, 103]
```

Example 2. Cloning a non-contiguous view materializes a contiguous standalone
result while preserving the view's visible order.

```cpp
auto clone_view = pluss::make_series({
    {1.0, 2.0},
    {3.0, 4.0},
    {5.0, 6.0},
}).transpose(0, 1);

auto clone_materialized = clone_view.clone();

std::cout << "before=" << (clone_view.is_contiguous() ? "true" : "false") << "\n";
std::cout << "after=" << (clone_materialized.is_contiguous() ? "true" : "false") << "\n";
std::cout << clone_materialized << "\n";
```

### Output

```text
before=false
after=true
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 3, 5],
  [2, 4, 6]]
```

Example 3. Reverse-mode sensitivity passes through `clone()` to the original
source.

```cpp
auto clone_grad = pluss::make_series({
    {1.0, 2.0},
    {3.0, 4.0},
});
clone_grad.tracks_slope_(true);
clone_grad.clone().sum().reverse();

pluss::series::print(std::cout, clone_grad.sensitivity());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 1.0000 1.0000 ⎤
⎣ 1.0000 1.0000 ⎦
```

## Gotchas And Failure Modes

- `clone()` copies the visible series, not the raw source-storage arrangement
  behind a view.
- The result is materialized and independent even when `self` was already
  contiguous.
- `clone()` is about storage independence, not dtype or platform conversion.
  Use [`to()`](to.md) for representation changes.

## Summary

Use `clone()` when you need the same values in a distinct buffer. It preserves
the current visible shape/order, materializes standalone storage, and now keeps
reverse-mode connected on the current verified surface.

## See Also

- [`Series::contiguous`](contiguous.md): materialize a view specifically to
  normalize layout.
- [`Series::to`](to.md): change dtype, platform, or both.
- [`Series::copy_`](copy_.md): mutate an existing destination instead of
  producing a new result.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clear_sensitivity.html">
    <span class="label">Previous</span>
    <span class="title">clear_sensitivity</span>
  </a>
  <a class="next-link" href="comp.html">
    <span class="label">Next</span>
    <span class="title">comp</span>
  </a>
</div>

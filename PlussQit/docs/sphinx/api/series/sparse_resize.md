<!-- pluss-hand-authored -->

# Series::sparse_resize

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::sparse_resize_() const
Series& Series::sparse_resize_and_clear_() const
```

## What It Does

Pluss's `Series` has no real sparse storage format, so both of these are torch-parity stubs rather than working sparse-tensor operations.

`sparse_resize_()` is a pure no-op: it returns a reference to `self` unmodified.

`sparse_resize_and_clear_()` is **not** a no-op, despite the similar name: it zeros out `self`'s values in place (same shape, same dtype) — the "and_clear" half of the name is the part that actually does something.

Despite being declared `const` at the call site, `sparse_resize_and_clear_()` genuinely mutates `self`'s values.

## Parameters

None on this surface.

## Returns

`Series&` — a reference to `self`. Unmodified after `sparse_resize_()`; zeroed after `sparse_resize_and_clear_()`.

## Examples

Example 1. `sparse_resize_()` is a verified no-op.

```cpp
auto x = pluss::make_series({1.0, 2.0});

auto& ref = x.sparse_resize_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 2]
same object: true
```

Example 2. `sparse_resize_and_clear_()` zeros the values, unlike its plain sibling.

```cpp
auto x = pluss::make_series({1.0, 2.0});

x.sparse_resize_and_clear_();
std::cout << x;
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 0]
```

## Gotchas And Failure Modes

- Neither method performs any real sparse-storage resizing — there is no sparse backend behind them.
- `sparse_resize_and_clear_()` zeros values despite `sparse_resize_()` alone being a no-op — read the name carefully.

## Summary

Both are torch-parity stubs kept for API compatibility: `sparse_resize_()` does nothing; `sparse_resize_and_clear_()` zeros `self`'s values.

## See Also

- [`Series::zero`](zero.md): the dedicated, non-stub way to zero a `Series` in place.
- [`Series::resize_as`](resize_as.md): also covers the `resize_as_sparse_` stub overloads.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="split_with_sizes.html">
    <span class="label">Previous</span>
    <span class="title">split_with_sizes</span>
  </a>
  <a class="next-link" href="sqrt.html">
    <span class="label">Next</span>
    <span class="title">sqrt</span>
  </a>
</div>

<!-- pluss-hand-authored -->

# Series::resize_as

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::resize_as_(const Series& other) const
Series& Series::resize_as_sparse_() const
Series& Series::resize_as_sparse_(const Series& other) const
```

## What It Does

`resize_as_(other)` rebinds `self` to a freshly allocated, **uninitialized** buffer matching `other`'s shape — the same discard-and-reallocate contract as [`Series::resize_`](resize.md), just with the target shape read from another `Series` instead of spelled out directly.

`resize_as_sparse_()` is a torch-parity stub: Pluss's `Series` has no real sparse storage format, so both the zero-argument and `other`-argument overloads are pure no-ops — they return `self` unmodified.

Despite being declared `const` at the call site, `resize_as_(other)` genuinely mutates `self`; the `resize_as_sparse_` overloads do not, since they do nothing at all.

## Parameters

**other** (`const Series&`) — for `resize_as_`, the `Series` whose shape to adopt; for `resize_as_sparse_(other)`, ignored (no-op).

## Returns

`Series&` — a reference to `self`. After `resize_as_(other)`, backed by a new, uninitialized buffer shaped like `other`. After either `resize_as_sparse_` overload, unchanged.

## Examples

Example 1. `resize_as_(other)`: only the new shape is guaranteed, so this example checks shape, not data.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
auto other = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});

x.resize_as_(other);
std::cout << "shape=[" << x.sizes()[0] << "," << x.sizes()[1] << "] numel=" << x.numel();
```

### Output

```text
shape=[2,2] numel=4
```

Example 2. `resize_as_sparse_()` is a verified no-op.

```cpp
auto x = pluss::make_series({1.0, 2.0});

auto& ref = x.resize_as_sparse_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 2]
same object: true
```

## Gotchas And Failure Modes

- `resize_as_(other)`'s resulting buffer is uninitialized, exactly like `resize_()`.
- `resize_as_sparse_()` never actually resizes anything — it's a stub, not a working sparse feature.

## Summary

Use `resize_as_(other)` to adopt another `Series`'s shape into a fresh, uninitialized buffer. `resize_as_sparse_()` is a no-op stub kept for torch-style API compatibility.

## See Also

- [`Series::resize`](resize.md): the direct-shape form this delegates its contract to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="resize.html">
    <span class="label">Previous</span>
    <span class="title">resize</span>
  </a>
  <a class="next-link" href="retain_grad.html">
    <span class="label">Next</span>
    <span class="title">retain_grad</span>
  </a>
</div>

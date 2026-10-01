<!-- pluss-hand-authored -->

# Series::resize

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::resize_(pluss::msharp::RangeView::Int sizes) const
```

## What It Does

`resize_()` rebinds `self` to a **freshly allocated, uninitialized** buffer of the given `sizes`. It does not preserve, reshape, or reinterpret the old data — the old values are simply discarded, and the new buffer's contents are unspecified until you write to them.

This is fundamentally different from [`Series::reshape`](reshape.md) (which reinterprets existing data without copying or discarding anything) or [`Series::fill_`](fill.md) (which overwrites existing storage with a known value). `resize_()` is the low-level, torch-parity "just give me a buffer of this shape" primitive.

Despite being declared `const` at the call site, `resize_()` genuinely mutates `self`.

## Parameters

**sizes** (`RangeView::Int`) — the new shape. Accepts a braced list or a `std::array`/`std::vector` of `std::int64_t`.

## Returns

`Series&` — a reference to `self`, now backed by a new, uninitialized buffer of `sizes`.

## Examples

Example 1. The values after `resize_()` are unspecified — only the new shape is guaranteed, so this example checks shape, not data.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
const std::array<std::int64_t, 1> sizes{5};

x.resize_(pluss::msharp::RangeView::Int(sizes.data(), sizes.size()));
std::cout << "shape=[" << x.sizes()[0] << "] numel=" << x.numel();
```

### Output

```text
shape=[5] numel=5
```

## Gotchas And Failure Modes

- The resulting buffer is **uninitialized** — reading it before writing to it observes unspecified values.
- Old data is discarded outright, not preserved or reinterpreted; use `reshape()` if you want to keep the data.

## Summary

Use `resize_()` only when you specifically want a fresh, uninitialized buffer of a new shape and intend to write every position yourself before reading it back.

## See Also

- [`Series::resize_as`](resize_as.md): resizes to match another `Series`'s shape.
- [`Series::reshape`](reshape.md): reinterprets existing data instead of discarding it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reshape.html">
    <span class="label">Previous</span>
    <span class="title">reshape</span>
  </a>
  <a class="next-link" href="resize_as.html">
    <span class="label">Next</span>
    <span class="title">resize_as</span>
  </a>
</div>

<!-- pluss-hand-authored -->

# Series::zero

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::zero_() const
```

## What It Does

`zero_()` overwrites every element of `self` with `0`, in place, keeping `self`'s existing shape and dtype. It is exactly the `fill_(0.0)` special case, given its own dedicated dispatch rather than routing through `fill_`.

Despite being declared `const` at the call site, `zero_()` genuinely mutates the series it's called on, including any other handle sharing the same storage.

## Parameters

None. This is a zero-argument transform on `self`.

## Returns

`Series&` — a reference to `self`, after every element has been overwritten with `0`.

## Examples

Example 1. Every element becomes zero, in place; shape and dtype are unchanged.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.zero_();
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 0]
```

## Gotchas And Failure Modes

- `zero_()` mutates `self`; use `Series::zeros(sizes, dtype)` when you want a fresh, independent zero-filled result instead.

## Summary

Use `zero_()` to reset an existing buffer to all zeros without allocating a new `Series`.

## See Also

- [`Series::fill`](fill.md): the general form, `zero_()` is `fill_(0.0)`.
- `Series::zeros`: builds a new zero-filled `Series` from a shape.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="xlogy.html">
    <span class="label">Previous</span>
    <span class="title">xlogy</span>
  </a>
  <a class="next-link" href="zeta.html">
    <span class="label">Next</span>
    <span class="title">zeta</span>
  </a>
</div>

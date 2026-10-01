<!-- pluss-hand-authored -->

# Series::fill

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::fill_(double value) const
```

## What It Does

`fill_()` overwrites every element of `self` with the same scalar `value`, in place. There is no out-of-place counterpart on this surface — the closest non-mutating equivalent is `Series::full(sizes, value, dtype)`, which builds a brand-new, fully-`value`-filled `Series` from a shape rather than overwriting an existing one.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `fill_()` genuinely mutates the series it's called on, including any other handle sharing the same storage.

## Parameters

**value** (`double`) — the scalar written into every position.

## Returns

`Series&` — a reference to `self`, after every element has been overwritten with `value`.

## Examples

Example 1. Every element becomes the same value, in place.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.fill_(9.0);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[9, 9, 9]
```

## Gotchas And Failure Modes

- `fill_()` mutates `self`; use `Series::full(sizes, value)` when you want a fresh, independent result instead.

## Summary

Use `fill_()` to overwrite an existing buffer with a constant, e.g. resetting a working accumulator, without allocating a new `Series`.

## See Also

- `Series::full`: builds a new value-filled `Series` from a shape.
- [`Series::zero`](zero.md): the `fill_(0.0)` special case, with its own dedicated dispatch.
- [`Series::fill_diagonal`](fill_diagonal.md): fills only the diagonal.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fftshift.html">
    <span class="label">Previous</span>
    <span class="title">fftshift</span>
  </a>
  <a class="next-link" href="fill_diagonal.html">
    <span class="label">Next</span>
    <span class="title">fill_diagonal</span>
  </a>
</div>

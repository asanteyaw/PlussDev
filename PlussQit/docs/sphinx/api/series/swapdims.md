<!-- pluss-hand-authored -->

# Series::swapdims

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::swapdims(std::int64_t axis0, std::int64_t axis1) const
Series& Series::swapdims_(std::int64_t dim0, std::int64_t dim1) const
```

## What It Does

`swapdims()` is an exact alias of [`swapaxes()`](swapaxes.md) (and therefore of
[`transpose()`](transpose.md)). It exchanges `axis0` and `axis1`, leaving every
other axis in place, and returns a stride view. See
[`swapaxes()`](swapaxes.md) for the full description, the swap-versus-move
distinction, examples, and gradient behavior.

## Parameters

**axis0** (`std::int64_t`)

One axis to exchange. Negative counts from the end.

**axis1** (`std::int64_t`)

The other axis. Negative counts from the end.

## Returns

`Series`
    A view with the two axes exchanged. Identical to
    `swapaxes(axis0, axis1)`.

## Gradient

Differentiable, identical to [`swapaxes()`](swapaxes.md): the backward swaps the
gradient's axes back.

## In-Place Form

`swapdims_()` is the in-place sibling of [`swapdims()`](swapdims.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `swapdims_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
x.swapdims_(0, 1);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3,2], strides=[1,3], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[1, 4],
  [2, 5],
  [3, 6]]
```

## Examples

Example 1. `swapdims` matches `swapaxes`.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});

std::cout << m.swapdims(0, 1);
```

### Output

```text
pluss::Series(shape=[3,2], strides=[1,3], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[1, 4],
  [2, 5],
  [3, 6]]
```

## Summary

`swapdims` is a spelling of [`swapaxes()`](swapaxes.md) / [`transpose()`](transpose.md)
— same semantics, same view result, same gradient.

## See Also

- [`Series::swapaxes`](swapaxes.md): the primary page for this operation.
- [`Series::transpose`](transpose.md): the canonical two-axis swap.
- [`Series::movedim`](movedim.md) / [`Series::moveaxis`](moveaxis.md): relocate
  one axis instead of swapping two.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="swapaxes.html">
    <span class="label">Previous</span>
    <span class="title">swapaxes</span>
  </a>
  <a class="next-link" href="t.html">
    <span class="label">Next</span>
    <span class="title">t</span>
  </a>
</div>

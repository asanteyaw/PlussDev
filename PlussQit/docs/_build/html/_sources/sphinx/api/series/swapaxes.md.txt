<!-- pluss-hand-authored -->

# Series::swapaxes

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::swapaxes(std::int64_t axis0, std::int64_t axis1) const -> Series
Series& Series::swapaxes_(std::int64_t dim0, std::int64_t dim1) const
```

[`swapdims()`](swapdims.md) is an exact alias. Both are spellings of
[`transpose(axis0, axis1)`](transpose.md).

## What It Does

`swapaxes()` exchanges two axes and leaves every other axis exactly where it is.
It is the general-rank version of [`t()`](t.md): where `t()` only handles a 2-D
matrix (and silently ignores anything else), `swapaxes()` works on any rank and
lets you name the pair.

Contrast with [`moveaxis()`](moveaxis.md): a swap touches only the two named
axes, while a move shifts a whole run of axes over. On a rank-3 series
`swapaxes(0, 2)` produces axis order `(2, 1, 0)`; `moveaxis(0, 2)` produces
`(1, 2, 0)`.

Either index may be negative. The result is a stride view — no copy.

## Parameters

**axis0** (`std::int64_t`)

One of the two axes to exchange. Negative counts from the end.

**axis1** (`std::int64_t`)

The other axis. Negative counts from the end.

## Returns

`Series`
    A view with `axis0` and `axis1` exchanged. Same dtype and platform.
    Non-contiguous in general.

## Gradient

`swapaxes()` is differentiable. A swap is its own inverse, so the backward
applies the same swap to the upstream gradient. Values pass through unchanged.

## In-Place Form

`swapaxes_()` is the in-place sibling of [`swapaxes()`](swapaxes.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `swapaxes_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
x.swapaxes_(0, 1);
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

Example 1. Swapping the first and last axes of a `2 x 3 x 4` series.

```cpp
std::vector<double> d(24);
for (int i = 0; i < 24; ++i) d[i] = i;
auto t3 = pluss::make_series(d, {2, 3, 4});

auto r = t3.swapaxes(0, 2);
std::cout << "shape=[";
for (auto x : r.sizes()) std::cout << x << ",";
std::cout << "]";
```

### Output

```text
shape=[4,3,2,]
```

Only axes `0` and `2` traded places; axis `1` (size `3`) stayed in the middle.

Example 2. On a 2-D matrix, `swapaxes(0, 1)` is the plain transpose.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});

std::cout << m.swapaxes(0, 1);
```

### Output

```text
pluss::Series(shape=[3,2], strides=[1,3], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[1, 4],
  [2, 5],
  [3, 6]]
```

Example 3. Swap versus move on the same input.

```cpp
std::vector<double> d(24);
for (int i = 0; i < 24; ++i) d[i] = i;
auto t3 = pluss::make_series(d, {2, 3, 4});

auto swapped = t3.swapaxes(0, 2);
auto moved   = t3.moveaxis(0, 2);
std::cout << "swapaxes(0,2)=[";
for (auto x : swapped.sizes()) std::cout << x << ",";
std::cout << "]  moveaxis(0,2)=[";
for (auto x : moved.sizes()) std::cout << x << ",";
std::cout << "]";
```

### Output

```text
swapaxes(0,2)=[4,3,2,]  moveaxis(0,2)=[3,4,2,]
```

Example 4. Differentiable: the gradient is swapped back.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
m.tracks_slope_(true);

m.swapaxes(0, 1).sum().reverse();

pluss::series::print(std::cout, m.sensitivity());
```

### Output

```text
Series(shape=[2,3], strides=[1,2], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=
⎡ 1.0000 1.0000 1.0000 ⎤
⎣ 1.0000 1.0000 1.0000 ⎦
```

## Gotchas And Failure Modes

- It swaps exactly two axes. To relocate one axis and shift the rest, use
  [`moveaxis()`](moveaxis.md); to reorder everything, [`permute()`](permute.md).
- Out-of-range indices raise.
- The result is a non-contiguous view; materialize with
  [`contiguous()`](contiguous.md) when a dense buffer is needed.
- `swapaxes` / `swapdims` / `transpose` are three names for the same call.

## Summary

`swapaxes(axis0, axis1)` exchanges two axes of any-rank series:

- every other axis stays put — it is a swap, not a shift
- the same operation as [`transpose()`](transpose.md); [`swapdims()`](swapdims.md)
  is an alias
- a zero-copy view; differentiable

## See Also

- [`Series::swapdims`](swapdims.md): exact alias of this method.
- [`Series::transpose`](transpose.md): the primary name for a two-axis swap.
- [`Series::t`](t.md): the 2-D-only shorthand.
- [`Series::moveaxis`](moveaxis.md): relocate one axis, sliding the others.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sum.html">
    <span class="label">Previous</span>
    <span class="title">sum</span>
  </a>
  <a class="next-link" href="swapdims.html">
    <span class="label">Next</span>
    <span class="title">swapdims</span>
  </a>
</div>

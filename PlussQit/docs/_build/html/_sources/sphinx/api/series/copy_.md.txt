<!-- pluss-hand-authored -->

# Series::copy_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::copy_(const Series& src) const
Series& Series::copy_(const Series& src, bool non_blocking) const
```

## What It Does

Overwrites `self`'s storage in place with `src`'s values, broadcasting `src`
against `self`'s shape if needed, and returns a reference to `self` — not a new
`Series`. `self` and `src` may differ in dtype or platform; `copy_` converts and
transfers as needed.

`copy_` is what every module's `reset_parameters()` uses to fill a freshly
allocated, already-registered parameter with real values (see the **Gradient**
section) — it is deliberately a storage write, not a value-producing expression
like most of the rest of the surface.

## Parameters

**src** (`const Series&`)

The source of the values to write. Broadcast against `self`'s shape.

**non_blocking** (`bool`, second overload)

Requests an asynchronous transfer when `self` and `src` are on different
platforms and the backend supports it. Has no observable effect on the result —
only on whether the call blocks until the transfer completes.

## Returns

`Series&`
    A reference to `self`, now holding `src`'s values. This is the same object,
    not a copy — see Example 1.

## Gradient

`copy_` is differentiable when `self` and/or `src` track slope, but *what* the
gradient means depends on what `self` was before the call:

```{important} A freshly allocated, tracked leaf keeps behaving like a leaf
The common pattern across this engine's modules — `Series::empty(shape, ...)`,
then `tracks_slope_(true)`, then `copy_(some_untracked_init_values)` inside
`reset_parameters()` — does **not** poison the parameter's later gradient. Even
though `copy_` technically gives `self` a source link back to "how it was
initialized," that link only ever redirects gradient to a real destination
(existing graph history, or a tracked `src`). A one-time `copy_` from an untracked
initializer has neither, so `self`'s own future sensitivity — accumulated normally
from whatever forward computations use it later — is left untouched instead of
being masked away. See Example 2.
```

```{note} Writing into a tracked view is a different case, and splits the gradient
When `self` is a *view* into a larger tracked tensor (e.g. `root.slice(...)`), the
overwritten region's gradient is redirected to `src` instead of staying on `root` —
`root` only keeps the gradient for the part `copy_` never touched. See Example 3.
```

## Examples

Example 1. `copy_` mutates and returns the same object; it does not allocate a new
one.

```cpp
auto dst = pluss::make_series(std::vector<double>{0, 0, 0}, {3});
auto src = pluss::make_series(std::vector<double>{1, 2, 3}, {3});
Series& ret = dst.copy_(src);
std::cout << "dst=" << dst << "\n";
std::cout << "same_object=" << (&ret == &dst ? "true" : "false") << "\n";
```

### Output

```text
dst=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
same_object=true
```

Example 2. A parameter allocated empty, marked as tracking slope, then filled via
`copy_` from an untracked initializer — the exact `Series::empty()` +
`tracks_slope_(true)` + `copy_()` sequence every module's `reset_parameters()`
uses. Its gradient from later use is not lost.

```cpp
Series w = Series::empty({5, 3}, ScalarType::Float64, pluss::pCPU);
w.tracks_slope_(true);
w.copy_(pluss::make_series(
    std::vector<double>{1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5}, {5, 3}));
w.clear_sensitivity();

auto idx = pluss::make_series(std::vector<std::int64_t>{0, 2, 4}, {3});
auto out = w.index_select(0, idx);
out.sum().reverse();
std::cout << "w.sensitivity()=" << w.sensitivity();
```

### Output

```text
w.sensitivity()=pluss::Series(shape=[5,3], strides=[3,1], dtype=float64, platform=cpu, numel=15, contiguous=true)
data=[[1, 1, 1],
  [0, 0, 0],
  [1, 1, 1],
  [0, 0, 0],
  [1, 1, 1]]
```

Rows 0, 2, and 4 (looked up by [`index_select`](index_select.md)) correctly show
gradient `1`; rows 1 and 3 (never touched downstream) correctly show `0`. The
one-time `copy_` that initialized `w` did not zero any of this out.

Example 3. `copy_` into a *view* of a larger tracked tensor splits the gradient
between the untouched region (stays on the root) and the overwritten region
(redirected to `src`).

```cpp
Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4});
x.tracks_slope_(true);
Series src = pluss::make_series(std::vector<double>{10.0, 20.0}, {2});
src.tracks_slope_(true);

Series v = x.slice(0, 1, 3, 1);  // x[1:3], a view sharing x's storage
v.copy_(src);

Series loss = x.sum();
loss.reverse();
std::cout << "x.sensitivity()=" << x.sensitivity() << "\n";
std::cout << "src.sensitivity()=" << src.sensitivity() << "\n";
```

### Output

```text
x.sensitivity()=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 0, 0, 1]
src.sensitivity()=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

`x`'s untouched positions (0 and 3) keep gradient `1` from `x.sum()`. Positions 1
and 2 — the ones `v.copy_(src)` overwrote — show `0` on `x` and `1` on `src`
instead: `x.sum()`'s naive per-element gradient at those two positions has been
redirected to whichever tensor actually produced their current values (`src`), not
left sitting on `x` as if `x` had produced them itself.

## Notes

- `copy_` always writes into `self`'s existing storage; it never reallocates. This
  is exactly what makes it safe to call from `reset_parameters()` on an
  already-registered parameter — see Example 2 above.
- The distinction between Example 2 (a fresh leaf) and Example 3 (a view into an
  existing tracked tensor) is not incidental — it is *why* the leaf case is safe:
  a fresh leaf has no prior graph history for `copy_` to redirect gradient toward,
  so nothing is masked; a view into existing history does have somewhere for the
  overwritten region's gradient to go, so it gets redirected there instead.
- `non_blocking=true` only affects transfer timing, never the numerical result.

## See Also

- [`Series::index_select`](index_select.md), [`Series::gather`](gather.md): typical
  downstream consumers whose gradient flows back into a `copy_`-initialized
  parameter, as in Example 2.
- [`Series::slice`](slice.md): how to build the kind of view used in Example 3.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clamp_min.html">
    <span class="label">Previous</span>
    <span class="title">clamp_min</span>
  </a>
  <a class="next-link" href="gather.html">
    <span class="label">Next</span>
    <span class="title">gather</span>
  </a>
</div>

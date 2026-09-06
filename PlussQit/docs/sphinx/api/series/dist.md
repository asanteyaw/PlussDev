<!-- pluss-hand-authored -->

# Series::dist

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::dist(const Series& other) const
Series Series::dist(const Series& other, double p) const
```

## What It Does

Returns a single number: the **`p`-norm of the difference** `self - other`.

It is the norm-based notion of "how far apart are these two tensors". The two
inputs are subtracted elementwise, the whole difference is treated as one flat
bag of values, and its `p`-norm is taken. The result is always a 0-D scalar, no
matter the shape of the inputs.

Different `p` give different notions of distance:

- **`p = 2`** (the default) — ordinary Euclidean / straight-line distance:
  `sqrt(sum((a - b)^2))`. For a 2-D input this is the Frobenius distance.
- **`p = 1`** — Manhattan / taxicab distance: `sum(|a - b|)`.
- **`p = inf`** — Chebyshev distance: `max(|a - b|)`, the single largest
  coordinate gap.
- **general `p`** — `(sum(|a - b|^p))^(1/p)`.

The one-argument form `dist(other)` is exactly `dist(other, 2.0)`.

The difference is a normal broadcast subtraction, so `self` and `other` do not
have to be the same shape as long as they broadcast together.

## Parameters

**other** (`const Series&`): the tensor to measure the distance to. Must
broadcast against `self`.

**p** (`double`, default `2.0`): which norm of the difference to take. `1`,
`2`, any positive real, or `inf` (`std::numeric_limits<double>::infinity()`).

## Returns

`Series`
    A 0-D scalar — the `p`-norm of `self - other`. Real dtype; integer inputs
    are promoted.

## Gradient

Differentiable in both operands. For `p = 2`,
`d/da dist(a, b) = (a - b) / dist(a, b)` — the unit vector pointing from `b`
to `a`. It is undefined when `a == b` (the `0/0` at the cone tip); a reverse
pass there produces `nan`/`0` rather than raising, the same as
[`norm`](norm.md).

## Examples

Example 1. The default is the Euclidean distance, handed back as a scalar.

```cpp
auto a = pluss::make_series({1.0, 2.0, 3.0});
auto b = pluss::make_series({4.0, 6.0, 3.0});
std::cout << a.dist(b);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5
```

The difference is `[-3, -4, 0]`; `sqrt(9 + 16 + 0) = 5`.

Example 2. The `p` argument selects the norm.

```cpp
auto a = pluss::make_series({1.0, 2.0, 3.0});
auto b = pluss::make_series({4.0, 6.0, 3.0});
std::cout << "p=1  : " << a.dist(b, 1.0);
std::cout << "p=2  : " << a.dist(b, 2.0);
std::cout << "p=3  : " << a.dist(b, 3.0);
std::cout << "p=inf: " << a.dist(b, std::numeric_limits<double>::infinity());
```

### Output

```text
p=1  : pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=7p=2  : pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5p=3  : pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=4.4979414p=inf: pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=4
```

`p = 1` sums the gaps (`3 + 4 + 0 = 7`); `p = inf` takes the largest (`4`);
`p = 3` sits between `p = 2` and `p = inf`, always closer to the max as `p`
grows.

Example 3. A 2-D input is flattened first — this is the Frobenius distance.

```cpp
auto a = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto b = pluss::make_series({{1.0, 2.0}, {3.0, 8.0}});
std::cout << a.dist(b);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=4
```

Only one entry differs, by `4`, so every `p`-norm of the difference is `4`.
Shape plays no role beyond defining which entries line up.

Example 4. The distance from a tensor to itself is `0`.

```cpp
auto a = pluss::make_series({1.0, 2.0, 3.0});
std::cout << a.dist(a);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0
```

This is also the point where the `p = 2` gradient is undefined — see the
Gradient section.

Example 5. `dist` is `sub` followed by `norm`: `a.dist(b, p)` equals
`(a - b).norm(p)`.

```cpp
auto a = pluss::make_series({1.0, 2.0, 3.0});
auto b = pluss::make_series({4.0, 6.0, 3.0});
std::cout << "a.dist(b, 3) = " << a.dist(b, 3.0);
std::cout << "(a-b).norm(3) = " << a.sub(b).norm(3.0);
```

### Output

```text
a.dist(b, 3) = pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=4.4979414(a-b).norm(3) = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[4.4979414]
```

Same value. `dist` always normalizes the result to a 0-D scalar;
[`norm`](norm.md) with a non-standard `p` can hand back a `[1]` vector, which
is the one shape difference to be aware of.

Example 6. The difference broadcasts, so the two operands need not match shape.

```cpp
auto a = pluss::make_series({3.0, 4.0, 12.0});
std::cout << a.dist(pluss::make_series(0.0));
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=13
```

`a - 0` is just `a`; `sqrt(9 + 16 + 144) = 13`. This is the length of `a`
itself — `x.dist(zeros_like(x))` is `x.norm()`.

Example 7. Gradient: for `p = 2` it is the unit vector from `b` toward `a`.

```cpp
auto a = pluss::make_series({1.0, 2.0, 3.0});
a.tracks_slope_(true);
auto b = pluss::make_series({4.0, 6.0, 3.0});
a.dist(b).reverse();
std::cout << a.sensitivity();
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.6, -0.8, 0]
```

`(a - b) / dist(a, b) = [-3, -4, 0] / 5`. The vector has length `1`.

## Gotchas And Failure Modes

- The result is always a **0-D scalar**, even for `p` values where a bare
  [`norm(p)`](norm.md) would return a `[1]` vector.
- The two operands are combined with a broadcast **subtraction**, not a
  distance-matrix construction. For all pairwise distances between the rows of
  two matrices, reach for `stat`'s `cdist` instead.
- `p` must be positive. `p = 0` is not a norm and is rejected.
- The `p = 2` gradient is undefined at `a == b`. If your objective can drive the
  two together, add an epsilon or guard the step.
- `dist` promotes to a real dtype; the answer is never integral.

## Summary

`dist(other, p = 2)` is the `p`-norm of `self - other`, as a scalar:

- `p = 2` Euclidean (default), `p = 1` Manhattan, `p = inf` Chebyshev
- inputs broadcast; the difference is flattened before the norm
- always a 0-D scalar result
- differentiable; the `p = 2` gradient is the unit vector between the operands,
  undefined where they coincide

## See Also

- [`Series::norm`](norm.md): the `p`-norm of a single tensor — `dist` is
  `(a - b).norm(p)` pinned to a scalar.
- [`Series::sub`](sub.md): the elementwise difference `dist` measures.
- [`Series::renorm`](renorm.md): the other norm-constraint op — rescale
  sub-tensors so their norm stays under a cap.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dim.html">
    <span class="label">Previous</span>
    <span class="title">dim</span>
  </a>
  <a class="next-link" href="div.html">
    <span class="label">Next</span>
    <span class="title">div</span>
  </a>
</div>

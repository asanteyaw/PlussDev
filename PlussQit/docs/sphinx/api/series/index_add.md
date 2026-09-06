<!-- pluss-hand-authored -->

# Series::index_add

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::index_add(std::int64_t dim, const Series& index, const Series& source) const
Series Series::index_add(std::int64_t dim, const Series& index, const Series& source, double alpha) const
Series& Series::index_add_(std::int64_t dim, const Series& index, const Series& source) const
Series& Series::index_add_(std::int64_t dim, const Series& index, const Series& source, double alpha) const
```

## What It Does

Adds whole slices of `source` into whole slices of `self`, at positions named
by a plain list of integers.

Pick an axis `dim`. `index` is a 1-D list of destination positions along that
axis; `source` has one slice per entry of `index`. `index_add` walks `index`
and, for each entry `i`, adds `source`'s `i`-th slice into `self`'s
`index[i]`-th slice — every coordinate *outside* `dim` moves along for the
ride unchanged:

```text
result = self.clone()
for i in range(index.numel()):
    result.select(dim, index[i]) += alpha * source.select(dim, i)
```

This is the write-side sibling of [`index_select`](index_select.md): both take
one flat, 1-D list of positions along a single axis and reuse it uniformly
across every other axis. `index_select` reads those slices out;
`index_add` adds slices in. It is also a friendlier, restricted special case
of [`scatter_add`](scatter_add.md) — where `scatter_add`'s `index` must be a
full-rank tensor naming a destination for every individual element,
`index_add`'s `index` only ever names *which slice*, which is all you need
when whole rows, columns, or channels move together (the common case: updating
selected rows of an embedding table, accumulating gradients into selected time
steps, merging duplicate label buckets).

Destinations may repeat. If `index` contains the same position twice, **both**
source slices are added there — this accumulate-on-repeat rule is the one
thing that separates `index_add` from [`index_copy`](index_copy.md), which
overwrites and has no accumulation semantics at all.

## Parameters

**dim** (`std::int64_t`): the axis along which slices are routed. Negative
values count from the end, exactly like everywhere else in `Series`.

**index** (`const Series&`): a scalar or 1-D `int32`/`int64` series of
destination positions along `dim`. Its length must equal
`source.size(dim)`. Values must lie in `[0, self.size(dim))` — unlike `dim`,
values inside `index` are **not** wrap-normalized for negatives.

**source** (`const Series&`): the slices to add. Same rank as `self`, same
size as `self` on every axis except `dim`, and the **same dtype** as `self`
(no implicit promotion).

**alpha** (`double`, default `1.0`): a scale applied to `source` before
adding. `alpha = -1` turns `index_add` into an indexed subtraction;
`alpha = 0.5` adds half of each routed slice.

## Returns

`Series`
    A new tensor with `self`'s shape, dtype, and platform. `self` is
    untouched. `index_add_` mutates `self` in place (preserving its storage)
    and returns a reference to it.

## Gradient

Differentiable in both `self` and `source`. Every position of `self`
contributes additively to the result, so its sensitivity passes straight
through unchanged. Each slice of `source` receives the upstream sensitivity
from the destination it was routed to, scaled by `alpha`:

```text
dL/dself                     = dL/dresult                       (unchanged)
dL/dsource.select(dim, i)    = alpha * dL/dresult.select(dim, index[i])
```

When a destination repeats, every source slice routed there independently
receives that destination's *entire* upstream sensitivity — the mirror image
of how, forward, every one of them independently contributed there.

## Examples

Example 1. The simplest shape: a 1-D receiver, three destinations, no
repeats.

```cpp
auto base = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});
auto index = pluss::make_series({0, 2, 4});
auto source = pluss::make_series({10.0, 20.0, 30.0});
pluss::series::print(std::cout, base.index_add(0, index, source));
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[11.0000, 2.0000, 23.0000, 4.0000, 35.0000]
```

Positions `0`, `2`, and `4` each gained their routed value; `1` and `3` were
never named and are untouched.

Example 2. `dim = 0` on a matrix: each `index` entry names a whole receiver
row.

```cpp
auto base = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}});
auto index = pluss::make_series({0, 2});
auto source = pluss::make_series({{100.0, 200.0}, {300.0, 400.0}});
pluss::series::print(std::cout, base.index_add(0, index, source));
```

### Output

```text
Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 101.0000 202.0000 ⎤
⎢   3.0000   4.0000 ⎥
⎣ 305.0000 406.0000 ⎦
```

Rows `0` and `2` each absorbed a whole source row; row `1`, never named in
`index`, is exactly the original `[3, 4]`.

Example 3. The same data with `dim = 1` instead: now `index` names whole
columns.

```cpp
auto base = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
auto index = pluss::make_series({0, 2});
auto source = pluss::make_series({{10.0, 30.0}, {40.0, 60.0}});
pluss::series::print(std::cout, base.index_add(1, index, source));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 11.0000 2.0000 33.0000 ⎤
⎣ 44.0000 5.0000 66.0000 ⎦
```

`source` is still shaped `[2, 2]` — one column per entry of `index` — it is
just interpreted as *columns of the receiver* now instead of *rows*. Changing
only `dim` changes which axis "one slice" means, not anything about `source`
or `index` themselves.

Example 4. A repeated destination accumulates every routed slice, not just
the last one.

```cpp
auto base = pluss::make_series({{1.0, 2.0, 3.0},
                                {4.0, 5.0, 6.0},
                                {7.0, 8.0, 9.0}});
auto index = pluss::make_series({2, 0, 2});
auto source = pluss::make_series({{10.0, 20.0, 30.0},
                                  {40.0, 50.0, 60.0},
                                  {1.0, 2.0, 3.0}});
pluss::series::print(std::cout, base.index_add(0, index, source));
```

### Output

```text
Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=
⎡ 41.0000 52.0000 63.0000 ⎤
⎢  4.0000  5.0000  6.0000 ⎥
⎣ 18.0000 30.0000 42.0000 ⎦
```

`index = [2, 0, 2]` routes source row `0` to receiver row `2`, source row `1`
to row `0`, and source row `2` *also* to row `2`. Row `2` therefore becomes
`[7,8,9] + [10,20,30] + [1,2,3] = [18,30,42]`. Row `1`, never named, is
untouched.

Example 5. `alpha` scales only what gets added — the receiver's own values are
never touched by it.

```cpp
// same base / index / source as Example 4
pluss::series::print(std::cout, base.index_add(0, index, source, 0.5));
```

### Output

```text
Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=
⎡ 21.0000 27.0000 33.0000 ⎤
⎢  4.0000  5.0000  6.0000 ⎥
⎣ 12.5000 19.0000 25.5000 ⎦
```

Every routed contribution is halved (`row 2 = [7,8,9] + 0.5*([10,20,30] +
[1,2,3])`); the base values `[7,8,9]`, `[1,2,3]`, `[4,5,6]` themselves are
full-strength.

Example 6. Negative `dim` wraps exactly like it does everywhere else in
`Series`.

```cpp
auto base = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto index = pluss::make_series({0});
auto source = pluss::make_series({5.0, 5.0}).reshape({2, 1});
pluss::series::print(std::cout, base.index_add(-1, index, source));
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 6.0000 2.0000 ⎤
⎣ 8.0000 4.0000 ⎦
```

On a 2-D input `dim = -1` is `dim = 1` (columns); `index = [0]` names column
`0`, which receives `source`'s single column `[5, 5]`.

Example 7. Gradient, on both operands at once, with a repeated destination in
the mix.

```cpp
auto base = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}});
auto source = pluss::make_series({{10.0, 20.0}, {30.0, 40.0}});
base.tracks_slope_(true);
source.tracks_slope_(true);
auto result = base.index_add(0, pluss::make_series({2, 0}), source, 2.0);
auto weights = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}});
result.mul(weights).sum().reverse();
std::cout << "dbase = "; pluss::series::print(std::cout, base.sensitivity());
std::cout << "dsrc  = "; pluss::series::print(std::cout, source.sensitivity());
```

### Output

```text
dbase = Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 1.0000 2.0000 ⎤
⎢ 3.0000 4.0000 ⎥
⎣ 5.0000 6.0000 ⎦dsrc  = Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 10.0000 12.0000 ⎤
⎣  2.0000  4.0000 ⎦
```

`base`'s sensitivity is exactly `weights`, unmodified — every receiver
position contributes additively, so nothing about `index_add` changes its
gradient. Source row `0` was routed to receiver row `2` (weights `[5,6]`);
scaled by `alpha = 2` that gives `dsrc[0] = [10, 12]`. Source row `1` was
routed to row `0` (weights `[1,2]`), giving `dsrc[1] = 2*[1,2] = [2,4]`.

Example 8. A destination outside `[0, self.size(dim))` raises immediately —
nothing is written.

```cpp
try {
  auto base = pluss::make_series({1.0, 2.0});
  auto index = pluss::make_series({5});
  auto source = pluss::make_series({1.0});
  pluss::series::print(std::cout, base.index_add(0, index, source));
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
index_add: index value out of range
```

Example 9. `index_add_` mutates the receiver in place and hands back a
reference to it.

```cpp
auto base = pluss::make_series({1.0, 2.0, 3.0});
base.index_add_(0, pluss::make_series({0, 1}), pluss::make_series({100.0, 200.0}));
pluss::series::print(std::cout, base);
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[101.0000, 202.0000, 3.0000]
```

## Gotchas And Failure Modes

- `index` is always 1-D (or scalar), even when `self` and `source` are
  matrices or higher rank. It is *not* a full-rank index tensor — that is
  what [`scatter_add`](scatter_add.md) is for.
- Repeated destinations **accumulate**. If you actually want "last write
  wins" with no summing, reach for [`index_copy`](index_copy.md) instead.
- `self` and `source` must share a dtype exactly — there is no implicit
  promotion here, unlike [`scatter_add`](scatter_add.md), which casts.
  Convert `source` first if the dtypes differ.
- Values inside `index` are never negative-wrapped. `dim = -1` is fine;
  an entry of `-1` *inside* `index` is out of range and raises.
- Every non-`dim` axis of `source` must match `self` exactly — there is no
  broadcasting to fill in a missing axis.
- `self` must have rank `>= 1`; a scalar receiver has no axis to route along.

## Summary

`index_add(dim, index, source, alpha = 1)` adds whole `source` slices into
`self` at the positions named by `index`:

- `index` is a flat, 1-D list of destination positions along `dim`
- repeated destinations accumulate every routed slice
- `alpha` scales the added contribution only, not `self`
- `self` and `source` must share rank, dtype, and every non-`dim` size
- differentiable in both operands; `self`'s gradient passes straight through,
  each `source` slice receives its destination's upstream sensitivity times
  `alpha`

## See Also

- [`Series::index_copy`](index_copy.md): overwrite instead of accumulating.
- [`Series::index_fill`](index_fill.md): write one repeated scalar instead of
  a full `source` tensor.
- [`Series::index_select`](index_select.md): the read-side sibling — the same
  1-D index, but reading slices out instead of adding them in.
- [`Series::scatter_add`](scatter_add.md): the general, full-rank-index form
  of accumulation.
- [`Series::index_reduce`](index_reduce.md): combine repeated destinations
  with a reduction other than addition (`prod`, `mean`, `amax`, `amin`).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="index.html">
    <span class="label">Previous</span>
    <span class="title">index</span>
  </a>
  <a class="next-link" href="index_copy.html">
    <span class="label">Next</span>
    <span class="title">index_copy</span>
  </a>
</div>

<!-- pluss-hand-authored -->

# Series::index_copy

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::index_copy(std::int64_t dim, const Series& index, const Series& source) const
Series& Series::index_copy_(std::int64_t dim, const Series& index, const Series& source) const
```

## What It Does

Overwrites whole slices of `self` with whole slices of `source`, at positions
named by a plain list of integers.

Pick an axis `dim`. `index` is a 1-D list of destination positions along that
axis; `source` has one slice per entry of `index`. For each entry `i`,
`source`'s `i`-th slice **replaces** `self`'s `index[i]`-th slice — every
coordinate outside `dim` moves along unchanged, and every destination not
named in `index` keeps its original value from `self`:

```text
result = self.clone()
for i in range(index.numel()):
    result.select(dim, index[i]) = source.select(dim, i)
```

`index_copy` is the overwrite-only counterpart of [`index_add`](index_add.md)
(which accumulates) — reach for it whenever a small set of rows, columns, or
channels needs to be patched or replaced wholesale without building a
full-size replacement tensor. It is also a friendlier, restricted special case
of [`scatter`](scatter.md), the same way `index_add` is a restricted special
case of `scatter_add`: `index_copy`'s `index` only ever names *which slice*,
not a destination for every individual element.

The mapping direction is the part that is easy to get backwards: `index[i]`
is read **source-slice-first** — "source slice `i` goes to destination
`index[i]`" — not "destination `i` reads from source position `index[i]`".
With `index = [2, 0]`, source slice `0` lands at destination `2`, and source
slice `1` lands at destination `0`.

## Parameters

**dim** (`std::int64_t`): the axis along which slices are replaced. Negative
values count from the end.

**index** (`const Series&`): a scalar or 1-D `int32`/`int64` series of
destination positions along `dim`. Its length must equal `source.size(dim)`.
Values must lie in `[0, self.size(dim))` — negative values inside `index` are
not wrap-normalized.

**source** (`const Series&`): the replacement slices, in the order they are
written. Same rank as `self`, same size as `self` on every axis except `dim`,
and the **same dtype** as `self` (no implicit promotion).

## Returns

`Series`
    A new tensor with `self`'s shape, dtype, and platform. `self` is
    untouched. `index_copy_` mutates `self` in place (preserving its storage)
    and returns a reference to it.

## Gradient

Differentiable in both `self` and `source`. A destination that was overwritten
no longer depends on `self`'s original value there, so `self`'s sensitivity
is zero at every named position and passes straight through everywhere else.
Each slice of `source` receives the upstream sensitivity from the destination
it was written to:

```text
dL/dself.select(dim, index[i])   = 0
dL/dself                         = dL/dresult  (everywhere else)
dL/dsource.select(dim, i)        = dL/dresult.select(dim, index[i])
```

This clean rule assumes destinations are unique. When `index` repeats a
position, the forward result keeps only the last write to it, but the
backward pass still routes that destination's upstream sensitivity to
*every* source slice that named it — not just the one that survived forward.
Avoid repeated destinations whenever the gradient of a discarded write
matters.

## Examples

Example 1. The simplest shape: a 1-D receiver, three destinations, no
repeats.

```cpp
auto base = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});
auto index = pluss::make_series({0, 2, 4});
auto source = pluss::make_series({10.0, 20.0, 30.0});
pluss::series::print(std::cout, base.index_copy(0, index, source));
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[10.0000, 2.0000, 20.0000, 4.0000, 30.0000]
```

Positions `0`, `2`, and `4` are replaced outright; `1` and `3` keep their
original values from `self`.

Example 2. The mapping direction, made concrete: `index` says where each
*source* slice goes, not which *destination* to read from.

```cpp
auto base = pluss::make_series({{1, 2, 3}, {4, 5, 6}});
auto index = pluss::make_series({2, 0});
auto source = pluss::make_series({{30, 10}, {60, 40}});
pluss::series::print(std::cout, base.index_copy(1, index, source));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 10 2 30 ⎤
⎣ 40 5 60 ⎦
```

Source column `0` (`[30, 60]`) is written to destination column `2`; source
column `1` (`[10, 40]`) is written to column `0`. Column `1` (`[2, 5]`) was
never a destination and survives from `self`.

Example 3. `dim = 0` on a matrix replaces whole rows.

```cpp
auto base = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}});
auto index = pluss::make_series({0, 2});
auto source = pluss::make_series({{-1.0, -2.0}, {-5.0, -6.0}});
pluss::series::print(std::cout, base.index_copy(0, index, source));
```

### Output

```text
Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ -1.0000 -2.0000 ⎤
⎢  3.0000  4.0000 ⎥
⎣ -5.0000 -6.0000 ⎦
```

Rows `0` and `2` are fully replaced; row `1`, never named, is exactly the
original `[3, 4]` — the same "untouched elsewhere" contract as
[`index_add`](index_add.md), just with overwriting instead of accumulation.

Example 4. A repeated destination keeps only the **last** write — there is no
accumulation rule to fall back on.

```cpp
auto base = pluss::make_series({0.0, 0.0, 0.0});
auto index = pluss::make_series({1, 1});
auto source = pluss::make_series({7.0, 9.0});
pluss::series::print(std::cout, base.index_copy(0, index, source));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, 9.0000, 0.0000]
```

Both source entries target position `1`; the second one (`9`) wins. Contrast
this with [`index_add`](index_add.md), where the same `index` would leave
`7 + 9 = 16` at that position instead.

Example 5. `self` and `source` must share a dtype exactly.

```cpp
try {
  auto base = pluss::make_series({1.0, 2.0});
  auto index = pluss::make_series({0});
  auto source = pluss::make_series({1});   // int64, base is float64
  pluss::series::print(std::cout, base.index_copy(0, index, source));
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
index_copy: self and source must have the same dtype
```

There is no implicit promotion here — cast `source` yourself
(`source.to(base.dtype())`) if the dtypes genuinely differ.

Example 6. Gradient: overwritten positions in `self` see zero; everything
else passes through.

```cpp
auto base = pluss::make_series({1.0, 2.0, 3.0});
base.tracks_slope_(true);
auto source = pluss::make_series({100.0});
source.tracks_slope_(true);
base.index_copy(0, pluss::make_series({1}), source).sum().reverse();
std::cout << "dbase = "; pluss::series::print(std::cout, base.sensitivity());
std::cout << "dsrc  = "; pluss::series::print(std::cout, source.sensitivity());
```

### Output

```text
dbase = Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.0000, 0.0000, 1.0000]dsrc  = Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1.0000]
```

Position `1` was overwritten by `source`, so `base`'s sensitivity there is
`0` — the original `2.0` no longer has any influence on the result. Positions
`0` and `2` are unaffected by the copy and pass their sensitivity through as
`1`.

Example 7. The in-place form preserves the receiver's storage identity —
existing aliases observe the update.

```cpp
auto values = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
const void* storage = values.raw_data();
values.index_copy_(0, pluss::make_series({1}), pluss::make_series({{30.0, 40.0}}));
pluss::series::print(std::cout, values);
std::cout << "same storage: " << std::boolalpha << (storage == values.raw_data());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡  1.0000  2.0000 ⎤
⎣ 30.0000 40.0000 ⎦same storage: true
```

`index_copy_` writes into the existing allocation rather than rebinding
`values` to a new one.

Example 8. A destination outside `[0, self.size(dim))` raises.

```cpp
try {
  auto base = pluss::make_series({1.0, 2.0});
  auto index = pluss::make_series({9});
  auto source = pluss::make_series({1.0});
  pluss::series::print(std::cout, base.index_copy(0, index, source));
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
index_copy: index value out of range
```

## Gotchas And Failure Modes

- The index maps **source order to destination order**, not the reverse. Read
  `index[i]` as "source slice `i` goes here", never as "destination `i` reads
  from here".
- Repeated destinations keep only the last write. If you want repeats to sum
  instead, use [`index_add`](index_add.md).
- `self` and `source` must match in dtype, platform, rank, and every
  non-`dim` size. There is no broadcasting and no implicit cast.
- Values inside `index` are never negative-wrapped, even though `dim` is.
- The backward pass routes a repeated destination's gradient to every source
  slice that named it, including ones whose forward contribution was
  discarded. Keep destinations unique whenever gradients through discarded
  writes would be misleading.
- `self` must have rank `>= 1`.

## Summary

`index_copy(dim, index, source)` overwrites whole `self` slices at the
positions named by `index`:

- `index` is a flat, 1-D list mapping *source* order to *destination* order
- repeated destinations keep only the last write — no accumulation
- `self` and `source` must share dtype, rank, and every non-`dim` size exactly
- differentiable: `self` gets zero gradient at overwritten positions, `source`
  gets the destination's upstream sensitivity

## See Also

- [`Series::index_add`](index_add.md): accumulate instead of overwriting when
  destinations may repeat.
- [`Series::index_fill`](index_fill.md): write one repeated scalar rather
  than a distinct slice per destination.
- [`Series::scatter`](scatter.md): the general, full-rank-index form of
  overwriting.
- [`Series::select_scatter`](select_scatter.md): the single-slice special case
  of the same idea.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="index_add.html">
    <span class="label">Previous</span>
    <span class="title">index_add</span>
  </a>
  <a class="next-link" href="index_fill.html">
    <span class="label">Next</span>
    <span class="title">index_fill</span>
  </a>
</div>

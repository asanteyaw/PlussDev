<!-- pluss-hand-authored -->

# Series::index_fill

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::index_fill(std::int64_t dim, const Series& index, double value) const
Series& Series::index_fill_(std::int64_t dim, const Series& index, double value) const
Series& Series::index_fill_(std::int64_t dim, const Series& index, double value) const
```

## What It Does

Overwrites whole slices of `self` with one repeated scalar, at positions
named by a plain list of integers.

Pick an axis `dim`. `index` is a 1-D list of positions along that axis; every
one of those slices — every coordinate on every other axis — is set to
`value`. Selecting columns `0` and `2` of a matrix fills both *entire*
columns, not two individual cells:

```text
result = self.clone()
for destination in index:
    result.select(dim, destination) = value
```

`index_fill` is [`index_copy`](index_copy.md) for the common special case
where every replacement slice is the *same* constant — no need to build and
shape a `source` tensor just to broadcast one number. It shares
`index_copy`'s and [`index_add`](index_add.md)'s flat, 1-D `index` — the
write-side counterpart of [`index_select`](index_select.md) — but only ever
needs a scalar on the other side.

## Parameters

**dim** (`std::int64_t`): the axis containing the selected slices. Negative
values count from the end; `-1` is the last axis, `-2` the one before it.

**index** (`const Series&`): a scalar or 1-D `int32`/`int64` series of
positions along `dim` to fill. Values must lie in `[0, self.size(dim))` and
are not negative-wrapped. Repeated positions are harmless — writing the same
scalar to a slice twice has the same effect as writing it once.

**value** (`double`): the fill scalar, converted to `self`'s dtype before
assignment. Pick a value representable by that dtype when `self` is integral.

## Returns

`Series`
    A new tensor with `self`'s shape, dtype, and platform. `self` is
    untouched. `index_fill_` mutates `self` in place (preserving its storage)
    and returns a reference to it.

## Gradient

Differentiable in `self`. Every filled slice becomes a constant with respect
to `self`, so its sensitivity there is zero; every other position passes
sensitivity through unchanged:

```text
dL/dself.select(dim, destination) = 0     for every destination in index
dL/dself                          = dL/dresult   (everywhere else)
```

`value` is a plain C++ `double` on this overload, not a tracked `Series`, so
it has no sensitivity of its own to receive. The integer `index` is likewise
non-differentiable.

## In-Place Form

`index_fill_()` is the in-place sibling of [`index_fill()`](index_fill.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `index_fill_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0}, {3});
auto idx = pluss::series::arange(0, 3, 2, pluss::ScalarType::Int64);

x.index_fill_(0, idx, 9.0);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[9, 2, 9]
```

## Examples

Example 1. The simplest shape: fill two positions of a 1-D series with one
constant.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});
pluss::series::print(std::cout, values.index_fill(0, pluss::make_series({1, 3}), -1.0));
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, -1.0000, 3.0000, -1.0000, 5.0000]
```

Positions `1` and `3` become `-1`; the rest are untouched.

Example 2. Negative `dim` fills whole columns.

```cpp
auto values = pluss::make_series({{1, 2, 3}, {4, 5, 6}});
pluss::series::print(std::cout, values.index_fill(-1, pluss::make_series({0, 2}), -1.0));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=
⎡ -1 2 -1 ⎤
⎣ -1 5 -1 ⎦
```

`dim = -1` is the column axis on a 2-D input, so index `0` fills the entire
first column and index `2` fills the entire last column. The middle column is
untouched. Notice `value = -1.0` (a `double`) lands as `-1` in this `int64`
series — it is converted to `self`'s dtype.

Example 3. `dim = 0` fills whole rows instead.

```cpp
auto values = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}});
pluss::series::print(std::cout, values.index_fill(0, pluss::make_series({1}), 0.0));
```

### Output

```text
Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 1.0000 2.0000 ⎤
⎢ 0.0000 0.0000 ⎥
⎣ 5.0000 6.0000 ⎦
```

Changing only `dim` changes the unit of replacement from columns to rows —
the same lesson as [`index_add`](index_add.md) Examples 2–3, here with a
constant instead of a routed tensor.

Example 4. A repeated index is harmless: writing the same scalar twice has no
extra effect.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0});
pluss::series::print(std::cout, values.index_fill(0, pluss::make_series({1, 1}), 7.0));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.0000, 7.0000, 3.0000]
```

Contrast this with [`index_add`](index_add.md), where a repeated destination
changes the *result* (values accumulate) — here it changes nothing, because
`value` is a fixed point, not something that stacks.

Example 5. Gradient: filled positions become constants (zero sensitivity);
everything else passes through.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0});
values.tracks_slope_(true);
values.index_fill(0, pluss::make_series({1, 2}), 0.0).sum().reverse();
pluss::series::print(std::cout, values.sensitivity());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000, 0.0000, 0.0000, 1.0000]
```

Positions `1` and `2` were overwritten by the constant `0.0`, so their
original values no longer influence the objective — sensitivity `0`.
Positions `0` and `3` are untouched and pass their sensitivity through as `1`.

Example 6. An empty `index` selects nothing — a no-op.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0});
auto index = pluss::make_series(std::vector<std::int64_t>{}).to(pluss::ScalarType::Int64);
pluss::series::print(std::cout, values.index_fill(0, index, -9.0));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.0000, 2.0000, 3.0000]
```

Example 7. The index must be integral — a floating-point `index` is
rejected.

```cpp
try {
  auto values = pluss::make_series({1.0, 2.0});
  auto index = pluss::make_series({0.0, 1.0});
  pluss::series::print(std::cout, values.index_fill(0, index, -1.0));
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
index_fill: index dtype must be int32 or int64
```

Example 8. `index_fill_` mutates the receiver in place and hands back a
reference to it.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0});
values.index_fill_(0, pluss::make_series({0, 2}), 8.0);
pluss::series::print(std::cout, values);
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[8.0000, 2.0000, 8.0000]
```

## Gotchas And Failure Modes

- `index_fill` selects **complete slices** along one axis — it cannot express
  an arbitrary elementwise pattern. For that, use
  [`masked_fill`](masked_fill.md) with a boolean mask instead.
- `value` is a single scalar for the whole call. To fill different positions
  with different values, use [`index_copy`](index_copy.md) with a `source`
  tensor.
- Repeated indices are harmless (idempotent), unlike in
  [`index_add`](index_add.md), where repeats change the result.
- Values inside `index` are never negative-wrapped, even though `dim` is.
- `value` is a plain `double`, not a tracked `Series` — it never receives a
  gradient of its own, regardless of `self`'s `tracks_slope_` state.
- `self` must have rank `>= 1`.

## Summary

`index_fill(dim, index, value)` overwrites whole `self` slices with one
repeated scalar:

- `index` is a flat, 1-D list of positions along `dim`; every named slice —
  every coordinate on every other axis — becomes `value`
- repeated indices are harmless (idempotent)
- `value` converts to `self`'s dtype and carries no gradient of its own
- differentiable in `self`: filled positions get zero sensitivity, the rest
  pass through

## See Also

- [`Series::masked_fill`](masked_fill.md): fill positions selected by an
  arbitrary boolean mask instead of whole slices along one axis.
- [`Series::index_copy`](index_copy.md): fill each selected slice with its
  own value instead of one shared scalar.
- [`Series::index_add`](index_add.md): accumulate a routed value instead of
  overwriting with a constant.
- [`Series::slice_scatter`](slice_scatter.md): replace a regularly strided
  run of slices in one call.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="index_copy.html">
    <span class="label">Previous</span>
    <span class="title">index_copy</span>
  </a>
  <a class="next-link" href="index_put.html">
    <span class="label">Next</span>
    <span class="title">index_put</span>
  </a>
</div>

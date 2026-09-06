<!-- pluss-hand-authored -->

# Series::select_scatter

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::select_scatter(const Series& src, std::int64_t dim, std::int64_t index) const
```

## What It Does

Returns a **copy** of the receiver with exactly **one** slice — the one at
position `index` along `dim` — replaced by `src`. The receiver is never
modified.

It is the write-side inverse of [`select`](select.md):

```text
result = base.select_scatter(src, dim, index)
result.select(dim, index) == src
```

The defining feature: [`select`](select.md) **removes** the axis it indexes
into, so `src` has **one fewer dimension** than the receiver. Replacing
column `2` of a `[2, 3]` matrix takes a `src` of shape `[2]` — *not*
`[2, 1]`. That rank reduction is the whole difference from
[`slice_scatter`](slice_scatter.md), whose `src` keeps the sliced axis.

Every position outside the selected slice keeps its value from the receiver.

## Parameters

**src** (`const Series&`): the replacement, with **exactly** the shape of
`base.select(dim, index)` — the receiver's shape with `dim` removed. Same
dtype and platform as the receiver. No broadcasting, no automatic singleton
axis.

**dim** (`std::int64_t`): the axis removed by the selection. Negative values
count from the end.

**index** (`std::int64_t`): the position on `dim`. Negative values count from
the end (`-1` is the last slice). Out-of-range values raise.

## Returns

`Series`
    A new tensor with the receiver's shape, dtype, and platform. There is no
    public `select_scatter_`.

## Gradient

Differentiable in both operands. The selected output slice depends on `src`,
not on the receiver's old slice, so the receiver's sensitivity is zero there
and passes straight through everywhere else. `src`'s sensitivity is
`out_sensitivity.select(dim, index)`.

## Examples

Example 1. A 1-D receiver: selecting removes the only axis, so `src` is a
**0-D scalar**.

```cpp
auto base = pluss::make_series({10.0, 20.0, 30.0, 40.0});
auto src = pluss::make_series(99.0);
pluss::series::print(std::cout, base.select_scatter(src, 0, 2));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10.0000, 20.0000, 99.0000, 40.0000]
```

Position `2` becomes `99`; the rest are unchanged.

Example 2. Replace the final column of a matrix with a negative index.

```cpp
auto base = pluss::make_series({{1, 2, 3}, {4, 5, 6}});
auto src = pluss::make_series({30, 60});
pluss::series::print(std::cout, base.select_scatter(src, 1, -1));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 1 2 30 ⎤
⎣ 4 5 60 ⎦
```

Selecting a matrix column removes the column axis → shape `[2]`, which is
exactly `src`. `index = -1` resolves to column `2`.

Example 3. A `[2, 1]` singleton is **not** the selected shape `[2]`.

```cpp
auto base = pluss::make_series({{1, 2, 3}, {4, 5, 6}});
try {
  base.select_scatter(pluss::make_series({30, 60}).reshape({2, 1}), 1, 0);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n'));
}
```

### Output

```text
select_scatter: src shape must match self.select(dim, index)
```

`[2, 1]` holds the right number of values but is rank `2`; the selected
column has shape `[2]`, and the contract rejects the extra singleton rather
than silently squeezing it.

Example 4. It round-trips with [`select`](select.md).

```cpp
auto base = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
auto src = pluss::make_series({7.0, 8.0, 9.0});   // base.select(0, 1) has shape [3]
auto out = base.select_scatter(src, 0, 1);
std::cout << "select back == src: " << std::boolalpha
          << out.select(0, 1).equal(src) << "\n";
```

### Output

```text
select back == src: true
```

Example 5. Gradient: the reverse pass hands the selected slice to the
source.

```cpp
auto base = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
auto src = pluss::make_series({30.0, 60.0});
base.tracks_slope_(true);
src.tracks_slope_(true);
base.select_scatter(src, 1, 1).sum().reverse();
std::cout << "dbase = "; pluss::series::print(std::cout, base.sensitivity());
std::cout << "dsrc  = "; pluss::series::print(std::cout, src.sensitivity());
```

### Output

```text
dbase = Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 1.0000 0.0000 1.0000 ⎤
⎣ 1.0000 0.0000 1.0000 ⎦dsrc  = Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.0000, 1.0000]
```

The old middle column no longer affects the result, so its receiver
sensitivity is zero; both `src` values appear once in the sum.

Example 6. An out-of-range `index` and a dtype mismatch are both rejected.

```cpp
auto base = pluss::make_series({{1, 2, 3}, {4, 5, 6}});
try {
  base.select_scatter(pluss::make_series({1, 2}), 1, 9);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
try {
  base.select_scatter(pluss::make_series({1.0, 2.0}), 1, 0);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
```

### Output

```text
select_scatter: index out of range
select_scatter: self and src must have the same dtype
```

## Gotchas And Failure Modes

- `src` has **one fewer axis** than the receiver. Do not keep `dim` as a
  singleton — that shape is rejected, not squeezed.
- On a 1-D receiver `src` must be a 0-D scalar.
- Negative `index` wraps from the end — a *scalar* argument convention.
  (This differs from the values *inside* an index tensor for
  [`index_copy`](index_copy.md), which are never negative-wrapped.)
- Receiver and `src` must share dtype and platform.
- Functional only — there is no `select_scatter_`. Use
  [`index_copy_`](index_copy.md) for a genuine in-place single-slice update.

## Summary

`select_scatter(src, dim, index)` returns the receiver with one selected
slice replaced:

- `src` matches `base.select(dim, index)` — the receiver's shape with `dim`
  **removed** (a 0-D scalar for a 1-D receiver)
- negative `index` wraps; out-of-range raises
- round-trips with [`select`](select.md); differentiable, with the receiver
  getting zero sensitivity at the replaced slice
- functional — never mutates the receiver

## See Also

- [`Series::select`](select.md): read the one rank-reduced slice this
  writes.
- [`Series::slice_scatter`](slice_scatter.md): replace a range while keeping
  the axis.
- [`Series::index_copy`](index_copy.md): replace a list of slices named by an
  index tensor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="select.html">
    <span class="label">Previous</span>
    <span class="title">select</span>
  </a>
  <a class="next-link" href="sensitivity.html">
    <span class="label">Next</span>
    <span class="title">sensitivity</span>
  </a>
</div>

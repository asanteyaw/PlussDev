<!-- pluss-hand-authored -->

# Series::index_reduce

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::index_reduce(std::int64_t dim, const Series& index, const Series& source, const std::string& reduce) const
Series Series::index_reduce(std::int64_t dim, const Series& index, const Series& source, const std::string& reduce, bool include_self) const
Series& Series::index_reduce_(std::int64_t dim, const Series& index, const Series& source, const std::string& reduce) const
Series& Series::index_reduce_(std::int64_t dim, const Series& index, const Series& source, const std::string& reduce, bool include_self) const
Series& Series::index_reduce_(std::int64_t dim, const Series& index, const Series& source, const std::string& reduce) const
```

## What It Does

Routes `source` slices to destinations exactly like
[`index_add`](index_add.md), but combines repeated destinations with a
reduction other than addition: product, mean, maximum, or minimum.

For each source position `i`, `index[i]` names a destination slice along
`dim`. Every source slice sent to the same destination forms a **group**.
`reduce` says how to combine a group; `include_self` says whether the
receiver's own slice at that destination joins the group as one more
member:

```text
result = self.clone()
for destination j named by index:
    group = { self.select(dim, j) } ∪ { source.select(dim, i) : index[i] == j }    -- if include_self
    group =                             { source.select(dim, i) : index[i] == j }    -- otherwise
    result.select(dim, j) = reduce(group)
```

A destination `self` never names in `index` is left completely alone in
either mode — `include_self = false` only removes the receiver from groups
that are *actually touched*, it does not reset the whole tensor to a
reduction identity first.

`"sum"` deliberately has no place here — that accumulation belongs to
[`index_add`](index_add.md). `index_reduce` is the compact way to express
"group these routed observations by destination and combine them", the same
job [`grouped_mean`](grouped_mean.md) and its siblings do with an explicit
group-id tensor instead of a `(dim, index)` pair.

## Reduction Modes

- **`"prod"`** multiplies the group's members.
- **`"mean"`** averages them. With `include_self = true` the receiver counts
  as one more contributor at every touched destination. An integral `"mean"`
  floors; use a floating dtype when fractional averages matter.
- **`"amax"`** keeps the group's greatest member.
- **`"amin"`** keeps the group's smallest member.

## Parameters

**dim** (`std::int64_t`): the axis along which slices are routed. Negative
values count from the end.

**index** (`const Series&`): a scalar or 1-D `int32`/`int64` series of
destination positions along `dim`, one per `source` slice. Repeated values
form multi-member groups. Values must lie in `[0, self.size(dim))` and are
not negative-wrapped.

**source** (`const Series&`): the slices to reduce. Same rank as `self`, same
size as `self` on every axis except `dim`, and the same dtype as `self`.

**reduce** (`const std::string&`): one of `"prod"`, `"mean"`, `"amax"`, or
`"amin"`.

**include_self** (`bool`, default `true`): whether the receiver's own slice
joins each touched group as one more member.

## Returns

`Series`
    A new tensor with `self`'s shape, dtype, and platform. `self` is
    untouched. `index_reduce_` mutates `self` in place (preserving its
    storage) and returns a reference to it.

## Gradient

Differentiable in both `self` and `source`, with a rule specific to each
`reduce`:

- **`mean`**: the upstream sensitivity at a destination is split evenly
  across every member of that destination's group (the receiver included, if
  `include_self`).
- **`amax` / `amin`**: sensitivity goes only to the group member(s) that
  equal the winning value; an exact tie splits it evenly among the tied
  winners.
- **`prod`**: each nonzero member receives the upstream sensitivity times the
  product of every *other* member of its group. A single zero member routes
  the product of the remaining (nonzero) members to it and zero to everyone
  else; two or more zero members route zero to every member — a true
  derivative-of-a-product-through-a-zero case, handled without dividing by
  zero.

The integer `index` and the string `reduce` are non-differentiable.

## In-Place Form

`index_reduce_()` is the in-place sibling of [`index_reduce()`](index_reduce.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `index_reduce_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({1.0, 1.0, 1.0}, {3});
auto idx = pluss::series::arange(0, 3, 2, pluss::ScalarType::Int64);
auto src = pluss::make_series({5.0, 7.0}, {2});

x.index_reduce_(0, idx, src, "prod");
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[5, 1, 7]
```

## Examples

Example 1. `prod` with and without the receiver in the group.

```cpp
auto base = pluss::make_series({10.0, 20.0, 30.0, 40.0});
auto index = pluss::make_series({0, 0, 2});
auto source = pluss::make_series({2.0, 3.0, 5.0});
pluss::series::print(std::cout, base.index_reduce(0, index, source, "prod", true));
std::cout << "\n";
pluss::series::print(std::cout, base.index_reduce(0, index, source, "prod", false));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[60.0000, 20.0000, 150.0000, 40.0000]

Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[6.0000, 20.0000, 5.0000, 40.0000]
```

`index = [0, 0, 2]` routes source `2` and `3` to destination `0`, and source
`5` to destination `2`. With the receiver included, destination `0` becomes
`10*2*3=60`; without it, just `2*3=6`. Positions `1` and `3` are never named
and keep `20` and `40` in both cases — `include_self` only affects touched
groups.

Example 2. `mean` on the same routing: `include_self` changes both the
numerator and the count.

```cpp
pluss::series::print(std::cout, base.index_reduce(0, index, source, "mean", true));
std::cout << "\n";
pluss::series::print(std::cout, base.index_reduce(0, index, source, "mean", false));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[5.0000, 20.0000, 17.5000, 40.0000]

Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[2.5000, 20.0000, 5.0000, 40.0000]
```

The inclusive mean at destination `0` is `(10+2+3)/3 = 5`; the exclusive mean
is `(2+3)/2 = 2.5`. Destination `2` moves from `(30+5)/2 = 17.5` to
`5/1 = 5`.

Example 3. `amax` and `amin` walk the same groups.

```cpp
pluss::series::print(std::cout, base.index_reduce(0, index, source, "amax", false));
std::cout << "\n";
pluss::series::print(std::cout, base.index_reduce(0, index, source, "amin", true));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[3.0000, 20.0000, 5.0000, 40.0000]

Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[2.0000, 20.0000, 5.0000, 40.0000]
```

The exclusive maximum at destination `0` compares only `2` and `3`, giving
`3`. The inclusive minimum compares `10`, `2`, and `3` too, giving `2`.

Example 4. `dim = 0` on a matrix: whole rows form a group, exactly like
[`index_add`](index_add.md)'s row-routing, but combined by `amax` here
instead of summed.

```cpp
auto rows = pluss::make_series({{1.0, 1.0}, {100.0, 100.0}, {1.0, 1.0}});
auto idx = pluss::make_series({0, 0});
auto src = pluss::make_series({{3.0, 5.0}, {7.0, 9.0}});
pluss::series::print(std::cout, rows.index_reduce(0, idx, src, "amax", true));
```

### Output

```text
Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡   7.0000   9.0000 ⎤
⎢ 100.0000 100.0000 ⎥
⎣   1.0000   1.0000 ⎦
```

Both source rows route to receiver row `0`; per column, the group is
`{1, 3, 7}` and `{1, 5, 9}`, so row `0` becomes `[7, 9]`. Rows `1` and `2`
were never named and are untouched.

Example 5. Negative `dim` routes columns instead, using the same rule.

```cpp
auto cols = pluss::make_series({{1.0, 100.0}, {1.0, 100.0}});
auto idx = pluss::make_series({0, 0});
auto src = pluss::make_series({{3.0, 7.0}, {9.0, 2.0}});
pluss::series::print(std::cout, cols.index_reduce(-1, idx, src, "amax", true));
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 7.0000 100.0000 ⎤
⎣ 9.0000 100.0000 ⎦
```

On a 2-D input `dim = -1` is `dim = 1` (columns). Both source columns route
to receiver column `0`; column `1` is untouched.

Example 6. Gradient: `prod`'s reverse multiplies every other member of the
group.

```cpp
auto base = pluss::make_series({2.0, 3.0});
auto source = pluss::make_series({4.0, 5.0});
base.tracks_slope_(true);
source.tracks_slope_(true);
base.index_reduce(0, pluss::make_series({0, 0}), source, "prod").sum().reverse();
std::cout << "dbase = "; pluss::series::print(std::cout, base.sensitivity());
std::cout << "dsrc  = "; pluss::series::print(std::cout, source.sensitivity());
```

### Output

```text
dbase = Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[20.0000, 1.0000]
dsrc  = Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[10.0000, 8.0000]
```

Destination `0`'s product is `2*4*5=40`. The receiver's local derivative is
the product of the *other* members, `4*5=20`; each source member's is the
product of the others (`2*5=10` and `2*4=8`). Position `1`, never named,
passes the objective's sensitivity through untouched as `1`.

Example 7. An invalid `reduce` name fails immediately.

```cpp
try {
  base.index_reduce(0, pluss::make_series({0, 0}), source, "sum");
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
index_reduce: reduce must be one of 'prod', 'mean', 'amax', or 'amin'
```

Example 8. A destination outside `[0, self.size(dim))` raises.

```cpp
try {
  auto base = pluss::make_series({1.0, 2.0});
  auto index = pluss::make_series({7});
  auto source = pluss::make_series({1.0});
  auto y = base.index_reduce(0, index, source, "amax");
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
index_reduce: index value out of range
```

Example 9. `index_reduce_` mutates the receiver in place.

```cpp
auto base = pluss::make_series({10.0, 20.0, 30.0});
base.index_reduce_(0, pluss::make_series({0, 0}), pluss::make_series({2.0, 3.0}), "prod");
pluss::series::print(std::cout, base);
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[60.0000, 20.0000, 30.0000]
```

`index_reduce_`'s default `include_self = true` folds the receiver's own
`10` into the product: `10*2*3 = 60`.

## Gotchas And Failure Modes

- `"sum"` belongs to [`index_add`](index_add.md) and is rejected here.
- `include_self = false` changes touched groups only; a destination never
  named in `index` is untouched in either mode — it is not reset to a
  reduction identity.
- Integral `"mean"` floors. Use floating `self`/`source` for fractional
  averages.
- `"prod"` can overflow fixed-width integer dtypes just like any other
  product.
- `self` and `source` must match dtype and every non-`dim` size exactly —
  the same strict contract as [`index_add`](index_add.md) and
  [`index_copy`](index_copy.md).
- Values inside `index` are never negative-wrapped, even though `dim` is.

## Summary

`index_reduce(dim, index, source, reduce, include_self = true)` groups
`source` slices by destination and combines each group:

- routing is identical to [`index_add`](index_add.md); the difference is how
  repeated destinations combine
- `reduce` is `"prod"`, `"mean"`, `"amax"`, or `"amin"` — never `"sum"`
- `include_self` decides whether the receiver's own slice joins a touched
  group; untouched destinations are unaffected either way
- differentiable, with a reduction-specific backward rule for each mode

## See Also

- [`Series::index_add`](index_add.md): the additive sibling — repeated
  destinations sum instead of reducing.
- [`Series::scatter_reduce`](scatter_reduce.md): the general, full-rank-index
  form of the same grouped reduction.
- [`Series::grouped_mean`](grouped_mean.md): group-oriented reduction driven
  by an explicit group-id tensor rather than a `(dim, index)` pair.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="index_put_.html">
    <span class="label">Previous</span>
    <span class="title">index_put_</span>
  </a>
  <a class="next-link" href="index_select.html">
    <span class="label">Next</span>
    <span class="title">index_select</span>
  </a>
</div>

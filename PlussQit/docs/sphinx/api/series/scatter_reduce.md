<!-- pluss-hand-authored -->

# Series::scatter_reduce

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::scatter_reduce(
    std::int64_t dim,
    const Series& index,
    const Series& src,
    const std::string& reduce) const -> Series

Series::scatter_reduce(
    std::int64_t dim,
    const Series& index,
    const Series& src,
    const std::string& reduce,
    bool include_self) const -> Series

Series::scatter_reduce(
    std::int64_t dim,
    const Series& index,
    const Series& src,
    const std::string& reduce,
    bool include_self,
    bool has_initial,
    double initial) const -> Series
```

## What It Does

`scatter_reduce()` is the generalized indexed-write reduction on `Series`.

Like [`scatter()`](scatter.md), it uses `index` to decide where values land
along one chosen axis. Unlike `scatter()`, it does not stop at overwrite
semantics. When several values target the same destination position,
`scatter_reduce()` combines them with a named reduction:

- `"sum"` adds contributors
- `"mean"` averages contributors
- `"amax"` keeps the largest contributor
- `"amin"` keeps the smallest contributor

The `include_self` toggle decides whether the destination's original value in
`self` participates in that reduction.

This one switch changes the mathematics in an important way:

- `include_self = true` means the existing lane in `self` is one of the
  reduction inputs
- `include_self = false` means the reduction is formed only from the scattered
  source contributions, or from the explicit `initial` seed when one is given

The final overload adds `has_initial` and `initial`. When enabled, every output
lane starts from `initial` before the reduction runs. That is most useful when
`include_self = false` and you want an explicit neutral or baseline value.

## Overload Map

- `scatter_reduce(dim, index, src, reduce)`:
  same as `include_self = true`
- `scatter_reduce(dim, index, src, reduce, include_self)`:
  choose whether the original destination value participates
- `scatter_reduce(dim, index, src, reduce, include_self, has_initial, initial)`:
  also seed every output lane with `initial`

If you already know the reduction at compile time, the named wrappers
[`scatter_reduce_sum()`](scatter_reduce_sum.md),
[`scatter_reduce_amax()`](scatter_reduce_amax.md), and
[`scatter_reduce_amin()`](scatter_reduce_amin.md) avoid the string argument.

## Parameters

**dim** (`std::int64_t`)

The axis whose positions are addressed by `index`.

For a matrix:

- `dim = 1` means reduce into columns inside each row
- `dim = 0` means reduce into rows inside each column lane

Negative dims normalize from the back.

**index** (`const Series&`)

Integral destination positions along `dim`.

The shape of `index` describes the write pattern itself. Repeated positions are
not only allowed here; they are the reason the method exists.

**src** (`const Series&`)

The values to reduce into the indexed destinations.

`src` may broadcast to the `index` shape. That makes this method more flexible
than plain [`scatter()`](scatter.md), whose source must match the index shape
exactly.

**reduce** (`const std::string&`)

The reduction to apply at repeated destinations.

Accepted values are:

- `"sum"`
- `"mean"`
- `"amax"`
- `"amin"`

**include_self** (`bool`)

Whether each destination lane in `self` participates in the reduction.

This toggle is often the difference between "update an existing value" and
"compute a grouped result from scratch."

For one destination lane `j`, if the incoming scattered values are
`v₁, v₂, ..., vₖ`, then:

- `"sum"` with `include_self = true` computes
  `self[j] + v₁ + ... + vₖ`
- `"sum"` with `include_self = false` computes
  `v₁ + ... + vₖ`
- `"mean"` with `include_self = true` computes
  `(self[j] + v₁ + ... + vₖ) / (k + 1)`
- `"mean"` with `include_self = false` computes
  `(v₁ + ... + vₖ) / k`

The same inclusion rule applies to `"amax"` and `"amin"`: the existing base
lane either competes in the max/min or it does not.

**has_initial** (`bool`)

Whether the explicit `initial` seed is active.

When this is `true`, every output lane begins from `initial` before the
reduction consumes `self` and scattered source contributions.

**initial** (`double`)

The explicit seed value used when `has_initial = true`.

This is a real participant in the reduction, not a comment or display hint.
That is why untouched positions become the seed value when
`include_self = false`.

## Returns

`Series`
    A new reduced result. `self` is not mutated.

## Gradient

`scatter_reduce()` participates in reverse-mode for the public reductions on
this surface: `"sum"`, `"mean"`, `"amax"`, and `"amin"`.

The gradient rule follows the forward reduction:

- `"sum"` routes the full upstream sensitivity to every contributing term
- `"mean"` divides that upstream sensitivity across the number of contributing
  terms in each destination lane
- `"amax"` and `"amin"` route sensitivity only through winning terms
- ties in `"amax"` and `"amin"` split sensitivity across equal winners

The examples below focus on the non-obvious winner and tie cases, because that
is where users usually need the strongest intuition.

## Examples

Example 1. `"sum"` with `include_self = true` keeps the original destination
value in the reduction.

```cpp
auto base = pluss::make_series({10.0, 20.0, 30.0, 40.0});
auto index = pluss::make_series({1LL, 1LL});
auto src = pluss::make_series({1.0, 3.0});

std::cout << base.scatter_reduce(0, index, src, "sum", true);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10, 24, 30, 40]
```

Destination `1` starts from `20`, then receives `1` and `3`, so the final lane
is `20 + 1 + 3 = 24`.

Example 2. `"sum"` with `include_self = false` reduces only the scattered
values.

```cpp
std::cout << base.scatter_reduce(0, index, src, "sum", false);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10, 4, 30, 40]
```

Now destination `1` no longer includes the old base value, so the reduced lane
is just `1 + 3 = 4`.

Example 3. `"mean"` averages exactly the contributors that the reduction sees.

```cpp
std::cout << base.scatter_reduce(0, index, src, "mean", true);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10, 8, 30, 40]
```

Destination `1` averages three contributors:

```text
(20 + 1 + 3) / 3 = 8
```

If `include_self` were `false`, the average would be taken only over the
scattered values.

Example 4. An explicit seed initializes every destination lane before the
reduction runs.

```cpp
auto seeded_index = pluss::make_series({0LL, 0LL, 1LL, 1LL});
auto seeded_src = pluss::make_series({10.0, 10.0, 10.0, 10.0});

std::cout << base.scatter_reduce(
    0, seeded_index, seeded_src, "sum", false, true, 5.0);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[25, 25, 5, 5]
```

Every lane starts from `5`. Destinations `0` and `1` then receive two extra
`10`s each, while untouched positions remain at the seed value.

Example 5. Negative dims work the same way as their normalized axis.

```cpp
auto matrix = pluss::make_series({
    {10.0, 20.0, 30.0},
    {40.0, 50.0, 60.0},
});
auto matrix_index = pluss::make_series({
    {2LL, 0LL},
    {1LL, 1LL},
});
auto matrix_src = pluss::make_series({
    {1.0, 2.0},
    {3.0, 4.0},
});

std::cout << matrix.scatter_reduce(-1, matrix_index, matrix_src, "sum", true);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[12, 20, 31],
  [40, 57, 60]]
```

On a rank-2 `Series`, `-1` means the last axis, so the reduction happens
across columns inside each row.

Example 6. `src` may broadcast to the index shape before the reduction.

```cpp
auto zeroes = pluss::make_series({
    {0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0},
});
auto broadcast_index = pluss::make_series({
    {0LL, 1LL, 2LL, 3LL},
    {0LL, 1LL, 2LL, 3LL},
});
auto row_src = pluss::make_series({{1.0, 2.0, 3.0, 4.0}});

std::cout << zeroes.scatter_reduce(1, broadcast_index, row_src, "sum", false);
```

### Output

```text
pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[1, 2, 3, 4],
  [1, 2, 3, 4]]
```

One row-shaped source broadcasts across both rows because it is compatible with
the index shape.

Example 7. `"amax"` splits sensitivity across equal winners.

```cpp
auto amax_base = pluss::make_series({2.0, 5.0, 5.0, 1.0});
auto amax_src = pluss::make_series({5.0, 4.0});
auto amax_index = pluss::make_series({1LL, 3LL});

amax_base.tracks_slope_(true);
amax_src.tracks_slope_(true);

auto out = amax_base.scatter_reduce(0, amax_index, amax_src, "amax", true);
out.sum().reverse();

std::cout << "out=" << out << "\n";
std::cout << "dbase=" << amax_base.sensitivity() << "\n";
std::cout << "dsrc=" << amax_src.sensitivity() << "\n";
```

### Output

```text
out=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[2, 5, 5, 4]
dbase=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 0.5, 1, 0]
dsrc=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.5, 1]
```

At destination `1`, the base value `5` and the source value `5` tie for the
maximum. The upstream sensitivity of that lane is therefore shared equally:
`0.5` to the base path and `0.5` to the source path.

Example 8. `"amin"` with `include_self = false` routes sensitivity only through
the winning scattered values.

```cpp
auto amin_base = pluss::make_series({1.0, 2.0});
auto amin_src = pluss::make_series({-3.0, 0.5});
auto amin_index = pluss::make_series({0LL, 1LL});

amin_base.tracks_slope_(true);
amin_src.tracks_slope_(true);

auto out = amin_base.scatter_reduce(0, amin_index, amin_src, "amin", false);
out.sum().reverse();

std::cout << "out=" << out << "\n";
std::cout << "dbase=" << amin_base.sensitivity() << "\n";
std::cout << "dsrc=" << amin_src.sensitivity() << "\n";
```

### Output

```text
out=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-3, 0.5]
dbase=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 0]
dsrc=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

Because `include_self = false`, the base lanes do not compete in the minimum
at all. Each output lane depends only on the scattered source value that won
that destination.

Example 9. The reduction name must be one of the supported strings.

```cpp
try {
  std::cout << base.scatter_reduce(0, index, src, "prod", true);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
scatter_reduce: bad reduce
```

Example 10. The explicit `initial` seed is not supported for `"mean"`.

```cpp
try {
  std::cout << base.scatter_reduce(0, index, src, "mean", false, true, 5.0);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
scatter_reduce_: mean with explicit initial is unsupported
```

## Gotchas And Failure Modes

- `reduce` must be one of `"sum"`, `"mean"`, `"amax"`, or `"amin"`.
- `include_self` changes the actual mathematics, not just an implementation
  detail.
- `initial` seeds every output lane, including untouched ones, when
  `has_initial = true`.
- `"mean"` with an explicit `initial` currently fails.
- Repeated destinations are the normal case here; they are not a misuse.
- If you only want additive accumulation, [`scatter_add()`](scatter_add.md) is
  the shorter and clearer spelling.

## Summary

Use `scatter_reduce()` when indexed writes should merge by a named reduction
instead of simple overwrite.

- `dim` chooses the destination axis
- `reduce` chooses the merge rule
- `include_self` decides whether the old destination value participates
- `has_initial` and `initial` provide an explicit seed
- `src` may broadcast to the index shape

## See Also

- [`Series::scatter`](scatter.md): indexed overwrite.
- [`Series::scatter_add`](scatter_add.md): additive accumulation shorthand.
- [`Series::scatter_reduce_`](scatter_reduce_.md): the in-place sibling.
- [`Series::scatter_reduce_sum`](scatter_reduce_sum.md): typed sum wrapper.
- [`Series::scatter_reduce_amax`](scatter_reduce_amax.md): typed max wrapper.
- [`Series::scatter_reduce_amin`](scatter_reduce_amin.md): typed min wrapper.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="scatter_add_.html">
    <span class="label">Previous</span>
    <span class="title">scatter_add_</span>
  </a>
  <a class="next-link" href="scatter_reduce_.html">
    <span class="label">Next</span>
    <span class="title">scatter_reduce_</span>
  </a>
</div>

<!-- pluss-hand-authored -->

# Series::scatter_add

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::scatter_add(std::int64_t dim, const Series& index, const Series& src) const -> Series
```

## What It Does

`scatter_add()` adds source values into indexed destination positions along one
chosen axis and returns the accumulated result.

This is the additive sibling of [`scatter()`](scatter.md). The two methods look
similar at a glance, but the semantic difference is large:

- `scatter()` means assignment into indexed positions
- `scatter_add()` means addition into indexed positions

That difference matters most when several source positions target the same
destination slot. With `scatter_add()`, those contributions sum. This is why
the method is so useful for accumulation patterns, histogram-like writes,
grouped totals, and the backward of gather-like reads.

There is another important public difference from `scatter()`: the source side
can broadcast to the `index` shape.

## Parameters

**dim** (`std::int64_t`)

The axis along which destination positions are chosen.

As with `scatter()`, this parameter changes the geometric meaning of the same
index values:

- `dim = 1` means add into columns inside each row
- `dim = 0` means add into rows inside each column lane

Negative dims normalize from the back.

**index** (`const Series&`)

Integral destination positions along `dim`.

The index must already line up positionally with the write pattern. In
particular:

- `index` must be integral
- `index` must have the same rank as `self`
- on non-`dim` axes, it must already align with `self`
- repeated positions are allowed and are often intentional

**src** (`const Series&`)

The values to add into the indexed positions.

Unlike `scatter()`, `src` can broadcast to the `index` shape. That means you
can write one row into many rows, or one scalar lane into many indexed writes,
as long as the source is broadcast-compatible with the index shape.

## Returns

`Series`
    A new `Series` containing the accumulated result.

## Gradient

`scatter_add()` is differentiable in both `self` and `src`.

For a scalar objective `L` built from the scattered result:

- every base lane contributes directly to the output lane at the same position,
  so base sensitivity follows the incoming sensitivity
- each source lane receives the sum of the destination sensitivities of the
  places it contributed to

When one source lane broadcasts across several indexed writes, its sensitivity
adds those repeated contributions together. That is why a repeated scalar
source can end up with a sensitivity larger than `1`.

## Examples

Example 1. Repeated targets add together instead of overwriting each other.

```cpp
auto base = pluss::make_series({0.0, 0.0, 0.0, 0.0});
auto index = pluss::make_series({0LL, 2LL, 2LL, 3LL});
auto src = pluss::make_series({0.1, 0.2, 0.3, 0.4});

std::cout << base.scatter_add(0, index, src);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.1, 0, 0.5, 0.4]
```

Destination `2` receives both `0.2` and `0.3`, so it ends at `0.5`.

Example 2. Existing destination values remain part of the final result because
this method adds rather than replaces.

```cpp
auto keep_base = pluss::make_series({10.0, 20.0, 30.0, 40.0});
auto keep_index = pluss::make_series({1LL, 3LL});
auto keep_src = pluss::make_series({7.0, 8.0});

std::cout << keep_base.scatter_add(0, keep_index, keep_src);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10, 27, 30, 48]
```

The destination starts from the base values and then adds the contributions.

Example 3. The index/src write pattern may be smaller than `self` along the
scatter axis.

```cpp
auto small_base = pluss::make_series({
    {0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0},
});
auto small_index = pluss::make_series({
    {0LL, 1LL, 2LL},
    {2LL, 1LL, 0LL},
});
auto small_src = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << small_base.scatter_add(1, small_index, small_src);
```

### Output

```text
pluss::Series(shape=[2,5], strides=[5,1], dtype=float64, platform=cpu, numel=10, contiguous=true)
data=[[1, 2, 3, 0, 0],
  [6, 5, 4, 0, 0]]
```

This is useful when you only want to update a subset of the destination axis
without constructing a full same-width source.

Example 4. `src` may broadcast to the index shape here.

```cpp
auto broadcast_base = pluss::make_series({
    {0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0},
});
auto broadcast_index = pluss::make_series({
    {0LL, 1LL, 2LL, 3LL},
    {0LL, 1LL, 2LL, 3LL},
});
auto row_src = pluss::make_series({{1.0, 2.0, 3.0, 4.0}});

std::cout << broadcast_base.scatter_add(1, broadcast_index, row_src);
```

### Output

```text
pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[1, 2, 3, 4],
  [1, 2, 3, 4]]
```

One row-shaped source broadcasts across both destination rows because it is
broadcast-compatible with the index shape.

Example 5. A repeated scalar-like source accumulates sensitivity according to
how many indexed writes reused it.

```cpp
auto grad_base = pluss::make_series({0.0, 0.0, 0.0, 0.0});
auto grad_src = pluss::make_series({2.0});
auto grad_index = pluss::make_series({0LL, 1LL, 1LL, 3LL});

grad_base.tracks_slope_(true);
grad_src.tracks_slope_(true);

auto out = grad_base.scatter_add(0, grad_index, grad_src);
out.sum().reverse();

std::cout << "out=" << out << "\n";
std::cout << "dbase=" << grad_base.sensitivity() << "\n";
std::cout << "dsrc=" << grad_src.sensitivity() << "\n";
```

### Output

```text
out=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[2, 4, 0, 2]
dbase=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 1, 1, 1]
dsrc=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=4
```

The scalar objective is the sum of four output lanes, and the single source
value participated in four indexed additions, so its total sensitivity is `4`.

## Gotchas And Failure Modes

- `scatter_add()` adds; it does not replace existing destination values.
- Repeated targets are usually intentional here, because they sum.
- `src` can broadcast to the `index` shape, which is more permissive than
  [`scatter()`](scatter.md).
- If overwrite semantics are what you mean, use `scatter()` instead.

## Summary

Use `scatter_add()` when indexed destinations should accumulate contributions.

- `dim` chooses the destination axis
- repeated targets sum
- existing base values remain part of the result
- `src` can broadcast to the index shape

## See Also

- [`Series::scatter`](scatter.md): overwrite semantics instead of addition.
- [`Series::scatter_add_`](scatter_add_.md): the in-place sibling.
- [`Series::scatter_reduce`](scatter_reduce.md): generalized indexed
  reductions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="scatter_.html">
    <span class="label">Previous</span>
    <span class="title">scatter_</span>
  </a>
  <a class="next-link" href="scatter_add_.html">
    <span class="label">Next</span>
    <span class="title">scatter_add_</span>
  </a>
</div>

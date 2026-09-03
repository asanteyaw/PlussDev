<!-- pluss-hand-authored -->

# Series::take_along_dim

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::take_along_dim(const Series& index) const -> Series
Series::take_along_dim(const Series& index, std::int64_t dim) const -> Series
```

## What It Does

`take_along_dim()` selects values along one axis using a matching array of
integer positions.

- **`take_along_dim(index, dim)`** reads, for each position of `index`, the
  element of `self` at that coordinate along `dim` and the same coordinates on
  every other axis. `index` must have the same rank as `self`; along `dim` it
  can be any length (that becomes the output length along `dim`), and along the
  other axes it must match `self` or be `1` to broadcast. The result has
  `index`'s shape.
- **`take_along_dim(index)`** with no axis flattens `self` first and treats
  `index` as flat positions into that 1-D sequence.

It is the axis-aware sibling of [`take()`](take.md) (flat linear positions) and
[`gather()`](gather.md) (which it shares an implementation with — error
messages say "gather"). The classic use is applying an
[`argsort()`](argsort.md) or [`argmax`](argmax.md) result: `x.take_along_dim(x.argsort(dim), dim)`
reorders `x` by its own sort order.

## Parameters

**index** (`const Series&`)

An `int64` array of positions along `dim`. Same rank as `self`; broadcasts on
the non-`dim` axes. Values must be in range for `dim`.

**dim** (`std::int64_t`, optional)

The axis to index along. Omitted: `self` is flattened and `index` holds flat
positions.

## Returns

`Series`
    A `Series` with `index`'s shape (after broadcasting), gathering from `self`
    along `dim`.

## Gradient

`take_along_dim()` is differentiable with respect to `self` (not `index`). The
backward scatters each output's sensitivity to the source position it came
from; when `index` names the same position more than once, those
contributions **accumulate**.

## Examples

Example 1. Picking a different column per row.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0, 4.0}, {5.0, 6.0, 7.0, 8.0}});
auto index = pluss::make_series({{2LL, 0LL}, {1LL, 3LL}});

std::cout << m.take_along_dim(index, 1);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[3, 1],
  [6, 8]]
```

Row `0` reads columns `2` and `0`; row `1` reads columns `1` and `3`.

Example 2. With no `dim`, `self` is flattened and `index` holds flat positions.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});

pluss::series::print(std::cout, m.take_along_dim(pluss::make_series({5LL, 0LL, 3LL})));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[6.0000, 1.0000, 4.0000]
```

Example 3. The backward scatters sensitivity back, accumulating on repeated
indices.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0, 4.0}, {5.0, 6.0, 7.0, 8.0}});
m.tracks_slope_(true);
auto index = pluss::make_series({{0LL, 0LL, 1LL, 1LL}, {1LL, 1LL, 0LL, 0LL}});

m.take_along_dim(index, 1).sum().reverse();

pluss::series::print(std::cout, m.sensitivity());
```

### Output

```text
Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=
⎡ 2.0000 2.0000 0.0000 0.0000 ⎤
⎣ 2.0000 2.0000 0.0000 0.0000 ⎦
```

Each row's index names two positions twice each, so those positions accumulate
`2` and the rest get `0`.

Example 4. `index` must have the same rank as `self`, and its values must be in
range.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});

try {
  m.take_along_dim(pluss::make_series({{9LL, 0LL, 1LL}, {0LL, 0LL, 0LL}}), 1);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
gather: index out of range
```

## Gotchas And Failure Modes

- `index` must be `int64` and the same rank as `self`. A 1-D `index` against a
  2-D `self` is a rank error.
- Out-of-range positions raise `gather: index out of range`.
- The no-`dim` form flattens, so a 2-D `self` comes back 1-D.
- Only `self` receives a gradient; `index` never does.

## Summary

`take_along_dim(index [, dim])` gathers along an axis by integer position:

- `index` shares `self`'s rank, broadcasts on the non-`dim` axes, sets the
  output shape
- no `dim` -> flatten `self` and index flat positions
- differentiable in `self`; the backward scatters and accumulates on repeats

## See Also

- [`Series::gather`](gather.md): the same operation; `take_along_dim` is the
  broadcasting-friendly spelling.
- [`Series::take`](take.md): flat linear indexing, no axis.
- [`Series::argsort`](argsort.md): the usual source of the `index` argument.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take.html">
    <span class="label">Previous</span>
    <span class="title">take</span>
  </a>
  <a class="next-link" href="tan.html">
    <span class="label">Next</span>
    <span class="title">tan</span>
  </a>
</div>

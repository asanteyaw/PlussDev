<!-- pluss-hand-authored -->

# Series::dim

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
std::int64_t Series::dim() const
```

## What It Does

Returns the **rank** of the `Series`: how many coordinates are needed to
address one value.

This is a purely structural question, unrelated to how many values exist or
how big any one axis is. A scalar has rank `0` — no coordinates needed, there
is only ever one value. A vector has rank `1`. A matrix has rank `2`. Every
extra pair of brackets in a nested initializer, every [`unsqueeze`](unsqueeze.md),
adds one more axis and therefore one more to `dim()`.

`dim()` is the number every negative-axis argument elsewhere in `Series`
normalizes against: `sum(-1)`, `cat(-1)`, `unsqueeze(-1)` all resolve `-1` to
`dim() - 1` internally. Knowing `dim()` is knowing which axis "the last one"
actually is for a given tensor.

The call is a plain native-integer accessor: no allocation, no data
movement, and (being a scalar `std::int64_t`, not a `Series`) nothing to
differentiate.

## Parameters

None.

## Returns

`std::int64_t`
    The number of axes in the current logical shape. Always `>= 0`.

## Gradient

Not applicable — `dim()` returns a plain C++ integer, not a `Series`, so
there is no sensitivity to track.

## Examples

Example 1. Rank climbs with each level of nesting.

```cpp
auto s = pluss::make_series(7.0);
auto v = pluss::make_series({1.0, 2.0, 3.0});
auto m = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto t = m.unsqueeze(0);
std::cout << "scalar dim=" << s.dim() << "\n";
std::cout << "vector dim=" << v.dim() << "\n";
std::cout << "matrix dim=" << m.dim() << "\n";
std::cout << "3-D    dim=" << t.dim() << "\n";
```

### Output

```text
scalar dim=0
vector dim=1
matrix dim=2
3-D    dim=3
```

Example 2. An axis with extent `0` still counts toward rank — `dim()` counts
axes, not the values along them.

```cpp
auto e = pluss::series::zeros({2, 0, 4});
std::cout << "shape=[2,0,4] dim=" << e.dim() << ", numel=" << e.numel() << "\n";
```

### Output

```text
shape=[2,0,4] dim=3, numel=0
```

Three axes, zero values — the two counts answer entirely different
questions. See [`numel`](numel.md) for the value-count side of this
distinction.

Example 3. `dim()` and `numel()` vary independently: a "wide" rank-1 tensor
can hold more values than a "deep" high-rank one.

```cpp
auto wide = pluss::series::zeros({1000});
auto tiny_deep = pluss::series::zeros({1, 1, 1, 1, 1});
std::cout << "wide:      dim=" << wide.dim() << ", numel=" << wide.numel() << "\n";
std::cout << "tiny_deep: dim=" << tiny_deep.dim() << ", numel=" << tiny_deep.numel() << "\n";
```

### Output

```text
wide:      dim=1, numel=1000
tiny_deep: dim=5, numel=1
```

Example 4. Shape-changing views alter `dim()` without moving a single value.

```cpp
auto m = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto flat = m.reshape({4});
auto boxed = m.unsqueeze(0).unsqueeze(0);
std::cout << "m     dim=" << m.dim() << "\n";
std::cout << "flat  dim=" << flat.dim() << "\n";
std::cout << "boxed dim=" << boxed.dim() << "\n";
```

### Output

```text
m     dim=2
flat  dim=1
boxed dim=4
```

`reshape` collapses two axes into one; each `unsqueeze` inserts a new
length-`1` axis. All three share the same four underlying values.

Example 5. `dim()` is exactly what a negative axis argument resolves
against, everywhere in `Series`.

```cpp
auto t = pluss::series::zeros({2, 3, 4});
std::cout << "sum(-1) reduces axis " << (t.dim() - 1) << "\n";
auto r = t.sum(-1);
std::cout << "result sizes=[" << r.size(0) << "," << r.size(1) << "]\n";
```

### Output

```text
sum(-1) reduces axis 2
result sizes=[2,3]
```

`-1` on a rank-`3` tensor means axis `2` — one less than `dim()` — which is
why the result drops that axis and keeps `[2, 3]`.

## Gotchas And Failure Modes

- `dim() == 0` means "no axes" (a scalar), **not** "empty". Test emptiness
  with [`numel`](numel.md)` == 0` instead.
- `dim() == 1` means "one axis of any extent", not "exactly one value" — a
  length-`1000` vector still has rank `1`.
- An axis of extent `0` still counts toward rank; see Example 2.
- `dim()` reflects the *current* view. Reshaping, squeezing, or unsqueezing
  changes it immediately and without copying data.

## Summary

`dim()` returns the `Series`' rank — the number of axes, independent of how
many values exist or how big any one axis is:

- scalar `0`, vector `1`, matrix `2`, and so on
- independent of [`numel`](numel.md); a wide rank-1 tensor can outnumber a
  deep high-rank one
- what every negative-axis argument elsewhere resolves against
- no allocation, no gradient — a plain integer accessor

## See Also

- [`Series::numel`](numel.md): the value count — the complementary structural
  question to rank.
- [`Series::size`](size.md) / [`Series::sizes`](sizes.md): the extent of one
  axis, or all of them.
- [`Series::stride`](stride.md): how coordinates on each axis map to storage.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="digamma.html">
    <span class="label">Previous</span>
    <span class="title">digamma</span>
  </a>
  <a class="next-link" href="dist.html">
    <span class="label">Next</span>
    <span class="title">dist</span>
  </a>
</div>

<!-- pluss-hand-authored -->

# Series::topk

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::topk(std::int64_t k) const
Series Series::topk(std::int64_t k, std::int64_t dim) const
Series Series::topk(std::int64_t k, std::int64_t dim, bool largest) const
Series Series::topk(std::int64_t k, std::int64_t dim, bool largest, bool sorted) const
```

## What It Does

`topk()` selects a partial ordering. It retains exactly `k` values from each
lane rather than fully sorting every value. The selected axis remains in the
result, but its size becomes `k`.

The default means "the largest `k` values on the final axis, in descending
order." `largest=false` selects the smallest side instead. `sorted=false`
keeps the correct membership but removes the promise that selected values will
be presented in rank order. Use [`topk_pair()`](topk_pair.md) when local source
indices must accompany the selected values.

## Parameters

**k** (`std::int64_t`): number of values to retain from each lane. It must lie
between `0` and `size(dim)`, inclusive.

**dim** (`std::int64_t`): selected axis; negative values count from the end.

**largest** (`bool`): choose the largest (`true`) or smallest (`false`) side of
the order.

**sorted** (`bool`): request rank ordering of the selected values. It changes
presentation guarantees, not which values are selected.

## Returns

`topk()` returns selected values.

## Gradient

`topk()` passes the selected values through unchanged, so its **value channel
is differentiable**: the backward routes each output's gradient to the input
position that value came from, and every position outside the selected set
receives `0`. The selection boundary itself is discrete — which positions make
the cut is held fixed during the backward pass, so no gradient flows through a
rank exchange or a tie at the cutoff.

## Examples

Example 1. The default retains the three largest values.

```cpp
auto scores = pluss::make_series({3.0, 1.0, 4.0, 1.0, 5.0});
std::cout << scores.topk(3);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[5, 4, 3]
```

Example 2. `largest=false` takes the low end instead.

```cpp
std::cout << scores.topk(3, 0, false, true);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 3]
```

Example 3. The selected axis changes length to `k`; every other axis stays.

```cpp
auto table = pluss::make_series({{4.0, 1.0, 3.0}, {2.0, 5.0, 1.0}});
std::cout << table.topk(1, 0);
```

### Output

```text
pluss::Series(shape=[1,3], strides=[1,1], dtype=float64, platform=cpu, numel=3, contiguous=false)
data=[[4, 5, 3]]
```

Example 4. `sorted=false` does not weaken membership, only its display order.

```cpp
std::cout << scores.topk(3, 0, true, false);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[5, 4, 3]
```

This run happens to be ordered. The `sorted=false` contract permits another
valid ordering of the same three winners, so presentation code must not rely
on this appearance.

Example 5. `k=0` is valid and keeps an empty selected axis.

```cpp
std::cout << scores.topk(0);
```

### Output

```text
pluss::Series(shape=[0], strides=[1], dtype=float64, platform=cpu, numel=0, contiguous=true)
[]
```

Example 6. Asking for more winners than a lane contains fails.

```cpp
try { std::cout << scores.topk(6); }
catch (const std::exception& error) { std::cout << error.what(); }
```

### Output

```text
topk: k must be <= size(dim)
```

Example 7. The value channel is differentiable; unselected positions get `0`.

```cpp
auto x = pluss::make_series({3.0, 1.0, 4.0, 1.0, 5.0});
x.tracks_slope_(true);

auto weights = pluss::make_series({100.0, 10.0, 1.0});
(x.topk(3, 0, true, true) * weights).sum().reverse();

pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 0.0000, 10.0000, 0.0000, 100.0000]
```

The three winners `[5, 4, 3]` came from positions `[4, 2, 0]`, so they collect
weights `100`, `10`, `1`; the two unselected `1`s at positions `1` and `3` get
nothing.

## Gotchas And Failure Modes

- `topk()` is not a full sort; only the selected subset is guaranteed to be
  present.
- `sorted=false` does not mean random and does not imply stable ordering.
- Ties at the cutoff have no stable winner contract.
- The value channel is differentiable; the selection boundary is not.
- Complex-valued inputs are rejected.

## Summary

Use `topk()` when only a small winning subset matters. Use `sort()` when every
value must be ordered, and keep `topk_pair()` results when later code needs to
recover the winning source positions.

## See Also

- [`Series::sort`](sort.md)
- [`Series::argsort`](argsort.md)
- [`Series::kthvalue`](kthvalue.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to.html">
    <span class="label">Previous</span>
    <span class="title">to</span>
  </a>
  <a class="next-link" href="topk_pair.html">
    <span class="label">Next</span>
    <span class="title">topk_pair</span>
  </a>
</div>

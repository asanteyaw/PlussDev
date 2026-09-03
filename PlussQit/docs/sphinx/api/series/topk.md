<!-- pluss-hand-authored -->

# Series::topk

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::topk(std::int64_t k) const -> Series
Series::topk(std::int64_t k, std::int64_t dim) const -> Series
Series::topk(std::int64_t k, std::int64_t dim, bool largest) const -> Series
Series::topk(std::int64_t k, std::int64_t dim, bool largest, bool sorted) const -> Series
Series::topk_pair(std::int64_t k, std::int64_t dim = -1,
                  bool largest = true, bool sorted = true) const
    -> pluss::StackArray<Series>
```

## What It Does

`topk()` selects a partial ordering. It retains exactly `k` values from each
lane rather than fully sorting every value. The selected axis remains in the
result, but its size becomes `k`.

The default means "the largest `k` values on the final axis, in descending
order." `largest=false` selects the smallest side instead. `sorted=false`
keeps the correct membership but removes the promise that selected values will
be presented in rank order. `topk_pair()` adds the local source indices that
identify where the winners came from.

## Parameters

**k** (`std::int64_t`): number of values to retain from each lane. It must lie
between `0` and `size(dim)`, inclusive.

**dim** (`std::int64_t`): selected axis; negative values count from the end.

**largest** (`bool`): choose the largest (`true`) or smallest (`false`) side of
the order.

**sorted** (`bool`): request rank ordering of the selected values. It changes
presentation guarantees, not which values are selected.

## Returns

`topk()` returns selected values. `topk_pair()` returns values at element `0`
and aligned local `int64` positions at element `1`.

## Gradient

`topk()` is a discrete selection boundary. Do not rely on a smooth derivative
when scores exchange rank or when a tie occurs at the selection boundary.

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

Example 5. The paired form preserves each winner's source position.

```cpp
auto winners = scores.topk_pair(3, 0, true, true);
std::cout << "values=" << winners[0] << "\n";
std::cout << "indices=" << winners[1];
```

### Output

```text
values=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[5, 4, 3]
indices=pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[4, 2, 0]
```

Example 6. `k=0` is valid and keeps an empty selected axis.

```cpp
std::cout << scores.topk(0);
```

### Output

```text
pluss::Series(shape=[0], strides=[1], dtype=float64, platform=cpu, numel=0, contiguous=true)
[]
```

Example 7. Asking for more winners than a lane contains fails.

```cpp
try { std::cout << scores.topk(6); }
catch (const std::exception& error) { std::cout << error.what(); }
```

### Output

```text
topk: k must be <= size(dim)
```

## Gotchas And Failure Modes

- `topk()` is not a full sort; only the selected subset is guaranteed to be
  present.
- `sorted=false` does not mean random and does not imply stable ordering.
- Ties at the cutoff have no stable winner contract.
- Complex-valued inputs are rejected.

## Summary

Use `topk()` when only a small winning subset matters. Use `sort()` when every
value must be ordered, and keep `topk_pair()` results when later code needs to
recover the winning source positions.

## See Also

- [`Series::sort`](sort.md)
- [`Series::argsort`](argsort.md)
- [`Series::kthvalue`](kthvalue.md)


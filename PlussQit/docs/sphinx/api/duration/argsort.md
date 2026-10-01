<!-- pluss-hand-authored -->

# Duration::argsort

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Series Duration::argsort() const;
```

## Return The Ordering, Not The Values

`argsort()` returns the zero-based source positions that would arrange a
one-dimensional `Duration` in ascending elapsed-time order. The returned
object is an `Int64` `Series`, because its lanes are positions rather than
elapsed values.

Ordering is stable. Equal finite values retain their source order, all `NaT`
lanes follow finite values, and multiple `NaT` lanes also retain source order.
These guarantees make the result suitable for applying the same permutation
to associated arrays.

The receiver must be rank one. A scalar or N-D receiver is rejected rather
than implicitly flattened, because flattening would erase the structural
meaning of the returned positions. Empty rank-one input is valid and returns
an empty rank-one `Int64` Series.

## Examples

Example 1. Read the stable permutation, including duplicate and `NaT`
positions.

```cpp
auto values = pluss::duration::nanos(
    {3, pluss::Duration::kNaT, 1, 3, pluss::Duration::kNaT, 2});
std::cout << pluss::series::print(values.argsort());
```

### Output

```text
Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[2, 5, 0, 3, 1, 4]
```

Positions `0` and `3` both contain three, so stability keeps `0` first. The
two absent lanes similarly remain in source order at the end.

Example 2. Apply the positions at an explicit host boundary.

```cpp
auto order = values.argsort().to_vec<std::int64_t>();
auto source = values.host();
std::vector<std::int64_t> arranged;
arranged.reserve(order.size());
for (auto i : order) arranged.push_back(source[static_cast<std::size_t>(i)]);

std::cout << pluss::duration::print(pluss::duration::nanos(arranged));
```

### Output

```text
Duration(shape=[6], dim=1, numel=6)
[ 0 days 00:00:00.000000001, 0 days 00:00:00.000000002, 0 days 00:00:00.000000003, 0 days 00:00:00.000000003, NaT, NaT ]
```

Example 3. Empty is valid; rank two is not.

```cpp
auto empty = pluss::duration::nanos(std::vector<std::int64_t>{});
std::cout << pluss::series::print(empty.argsort()); // shape [0]

pluss::duration::nanos({1,2,3,4}, {2,2}).argsort();
// throws: Duration::argsort requires rank-one input
```

## See Also

- [`sort()`](sort.md): return the arranged durations directly.
- [`unique()`](unique.md): retain first occurrences without sorting.

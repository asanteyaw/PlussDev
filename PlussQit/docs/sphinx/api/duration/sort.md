<!-- pluss-hand-authored -->

# Duration::sort

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Duration Duration::sort() const;
```

## Stable Ascending Order

`sort()` returns a newly allocated one-dimensional `Duration` whose finite
lanes are in ascending nanosecond order and whose `NaT` lanes are at the end.
It is stable: duplicate finite values and duplicate `NaT` values preserve
their relative source order.

Only rank-one receivers are accepted. The method does not flatten matrices or
cubes because doing so would silently discard structure. Empty rank-one input
is valid and remains shape `[0]`. The receiver is never mutated.

The implementation extracts values in logical order before sorting, so a
strided one-dimensional view is ordered by the values it exposes, not by the
physical order of its backing allocation.

## Examples

Example 1. Sort finite values while placing absence last.

```cpp
auto values = pluss::duration::nanos(
    {3, pluss::Duration::kNaT, 1, 3, pluss::Duration::kNaT, 2});
std::cout << pluss::duration::print(values.sort());
```

### Output

```text
Duration(shape=[6], dim=1, numel=6)
[ 0 days 00:00:00.000000001, 0 days 00:00:00.000000002, 0 days 00:00:00.000000003, 0 days 00:00:00.000000003, NaT, NaT ]
```

Example 2. The original remains unchanged.

```cpp
auto ordered = values.sort();
std::cout << pluss::duration::print(values);
```

The first lane is still `3 ns`; `sort()` does not provide an in-place form.

Example 3. An empty vector is already sorted.

```cpp
auto empty = pluss::duration::nanos(std::vector<std::int64_t>{});
std::cout << pluss::duration::print(empty.sort());
```

### Output

```text
Duration(shape=[0], dim=1, numel=0)[  ]
```

Example 4. Higher-rank input fails explicitly.

```cpp
pluss::duration::nanos({1,2,3,4}, {2,2}).sort();
// throws: Duration::sort requires rank-one input
```

## See Also

- [`argsort()`](argsort.md): return source positions instead of values.
- [`unique()`](unique.md): remove later duplicates while preserving source order.

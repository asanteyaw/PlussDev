<!-- pluss-hand-authored -->

# Duration::unique

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Duration Duration::unique() const;
```

## Keep First Occurrences

`unique()` returns one copy of every distinct elapsed nanosecond value,
preserving the order in which values first appear. It is not a sorting
operation. `NaT` is treated as one distinct value, so the first `NaT` is kept
at its original first-occurrence position and later `NaT` lanes are removed.

The receiver must be rank one. The result is a newly allocated rank-one
`Duration`; its length is the number of distinct values. Empty input returns
shape `[0]`. Exact signed nanoseconds define identity, so no formatting or
floating-point conversion can merge nearby values.

Internally, membership is tracked with a reserved hash set while output is
appended in source order, giving expected linear work rather than repeatedly
scanning all previously retained values.

## Examples

Example 1. First occurrence order differs from sorted order.

```cpp
auto values = pluss::duration::nanos(
    {3, pluss::Duration::kNaT, 1, 3, pluss::Duration::kNaT, 2});
std::cout << pluss::duration::print(values.unique());
```

### Output

```text
Duration(shape=[4], dim=1, numel=4)
[ 0 days 00:00:00.000000003, NaT, 0 days 00:00:00.000000001, 0 days 00:00:00.000000002 ]
```

Example 2. Positive and negative durations compare by exact stored value.

```cpp
auto values = pluss::duration::nanos({-1, 0, -1, 1, 0});
std::cout << pluss::duration::print(values.unique());
```

The result is `[-1 ns, 0 ns, 1 ns]` in that source order.

Example 3. Empty and invalid-rank behavior are distinct.

```cpp
auto empty = pluss::duration::nanos(std::vector<std::int64_t>{});
auto none = empty.unique();                 // valid, shape [0]

pluss::duration::nanos({1,2,3,4}, {2,2}).unique();
// throws: Duration::unique requires rank-one input
```

## See Also

- [`sort()`](sort.md): arrange all lanes without removing duplicates.
- [`argsort()`](argsort.md): obtain a stable ordering permutation.

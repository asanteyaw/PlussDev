<!-- pluss-hand-authored -->
# pluss::duration::range
[Back to Duration Factory API Reference](../../duration_factory_api_reference)

## Signature
```cpp
Duration pluss::duration::range(const Duration& start,
                                const Duration& end,
                                const Duration& step);
```

`range()` builds a rank-one fixed-width progression. All three arguments must
be finite scalar Durations and `step` must be nonzero. `start` is included;
`end` is included only when a whole number of steps lands on it. A step whose
sign points away from the endpoint returns shape `[0]`.

Element count is computed with widened integer arithmetic before allocation,
so progressions touching the signed-nanosecond limits cannot wrap or loop
forever. This is elapsed-time sequencing, not calendar recurrence.

## Examples
```cpp
std::cout << pluss::duration::range(pluss::duration::seconds(0),
    pluss::duration::seconds(5), pluss::duration::seconds(2));
```
```text
Duration(shape=[3], dim=1, numel=3)
[ 0 days 00:00:00.000000000, 0 days 00:00:02.000000000, 0 days 00:00:04.000000000 ]
```

```cpp
std::cout << pluss::duration::range(pluss::duration::seconds(5),
    pluss::duration::seconds(0), pluss::duration::seconds(-2));
```
```text
Duration(shape=[3], dim=1, numel=3)
[ 0 days 00:00:05.000000000, 0 days 00:00:03.000000000, 0 days 00:00:01.000000000 ]
```

```cpp
auto empty = pluss::duration::range(pluss::duration::seconds(5),
    pluss::duration::seconds(0), pluss::duration::seconds(2));
// empty.sizes() == [0]
```

```cpp
pluss::duration::range(pluss::duration::seconds(0),
    pluss::duration::seconds(1), pluss::duration::seconds(0));
// throws: Duration::range step must be non-zero
```


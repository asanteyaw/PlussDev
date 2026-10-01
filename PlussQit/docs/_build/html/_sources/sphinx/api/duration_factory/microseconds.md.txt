<!-- pluss-hand-authored -->
# pluss::duration::microseconds
[Back to Duration Factory API Reference](../../duration_factory_api_reference)

## Call Forms
```cpp
Duration pluss::duration::microseconds(integer_or_nested_values values);
Duration pluss::duration::microseconds(integer_range flat, integer_range sizes);
```

Each input count is multiplied by exactly 1,000 nanoseconds with checked
integer arithmetic. Use this factory for microsecond-resolution counters and
external formats; it does not discard precision after construction, so later
nanosecond arithmetic remains available. Scalar, rank-one, nested, and
flat-plus-shape inputs follow the landing-page geometry contract.

## Examples
```cpp
std::cout << pluss::duration::microseconds(-2);
```
```text
Duration(shape=[], dim=0, numel=1)
-1 days 23:59:59.999998000
```

```cpp
std::cout << pluss::duration::microseconds({0,1,2});
```
```text
Duration(shape=[3], dim=1, numel=3)
[ 0 days 00:00:00.000000000, 0 days 00:00:00.000001000, 0 days 00:00:00.000002000 ]
```

```cpp
std::cout << pluss::duration::microseconds({1,2,3,4}, {2,2});
```
```text
Duration(shape=[2, 2], dim=2, numel=4)
[ 0 days 00:00:00.000001000 0 days 00:00:00.000002000 ]
[ 0 days 00:00:00.000003000 0 days 00:00:00.000004000 ]
```


<!-- pluss-hand-authored -->
# pluss::duration::weeks
[Back to Duration Factory API Reference](../../duration_factory_api_reference)

## Call Forms
```cpp
Duration pluss::duration::weeks(integer_or_nested_values values);
Duration pluss::duration::weeks(integer_range flat, integer_range sizes);
```

Each count is exactly seven fixed elapsed days, or 604,800 seconds. The factory
does not align values to weekday boundaries. Large counts are checked during
nanosecond scaling and throw rather than wrap into an unrelated duration.

## Examples
```cpp
std::cout << pluss::duration::weeks(-2);
```
```text
Duration(shape=[], dim=0, numel=1)
-14 days 00:00:00.000000000
```

```cpp
std::cout << pluss::duration::weeks({0,1,2});
```
```text
Duration(shape=[3], dim=1, numel=3)
[ 0 days 00:00:00.000000000, 7 days 00:00:00.000000000, 14 days 00:00:00.000000000 ]
```

```cpp
std::cout << pluss::duration::weeks({1,2,3,4}, {2,2});
```
```text
Duration(shape=[2, 2], dim=2, numel=4)
[  7 days 00:00:00.000000000 14 days 00:00:00.000000000 ]
[ 21 days 00:00:00.000000000 28 days 00:00:00.000000000 ]
```

An unrepresentable count throws `Duration::mul: nanosecond overflow`.


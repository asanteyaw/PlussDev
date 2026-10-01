<!-- pluss-hand-authored -->
# pluss::duration::minutes
[Back to Duration Factory API Reference](../../duration_factory_api_reference)

## Call Forms
```cpp
Duration pluss::duration::minutes(integer_or_nested_values values);
Duration pluss::duration::minutes(integer_range flat, integer_range sizes);
```

`minutes()` constructs fixed spans of sixty seconds per count. Counts remain
integers and scaling is overflow-checked, so there is no rounding step. It is
suited to intervals, lead times, and fixed schedule gaps, not calendar-aware
wall-clock adjustment.

## Examples
```cpp
std::cout << pluss::duration::minutes(-2);
```
```text
Duration(shape=[], dim=0, numel=1)
-1 days 23:58:00.000000000
```

```cpp
std::cout << pluss::duration::minutes({0,1,2});
```
```text
Duration(shape=[3], dim=1, numel=3)
[ 0 days 00:00:00.000000000, 0 days 00:01:00.000000000, 0 days 00:02:00.000000000 ]
```

```cpp
std::cout << pluss::duration::minutes({{1,2},{3,4}});
```
```text
Duration(shape=[2, 2], dim=2, numel=4)
[ 0 days 00:01:00.000000000 0 days 00:02:00.000000000 ]
[ 0 days 00:03:00.000000000 0 days 00:04:00.000000000 ]
```


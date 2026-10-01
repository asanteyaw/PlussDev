<!-- pluss-hand-authored -->
# pluss::duration::milliseconds
[Back to Duration Factory API Reference](../../duration_factory_api_reference)

## Call Forms
```cpp
Duration pluss::duration::milliseconds(integer_or_nested_values values);
Duration pluss::duration::milliseconds(integer_range flat, integer_range sizes);
```

`milliseconds()` scales each count by exactly 1,000,000 nanoseconds. It is the
appropriate boundary for millisecond timers and serialized values, while still
producing a full nanosecond-capable `Duration`. Scaling overflow is rejected;
shape and lane order are preserved.

## Examples
```cpp
std::cout << pluss::duration::milliseconds(-2);
```
```text
Duration(shape=[], dim=0, numel=1)
-1 days 23:59:59.998000000
```

```cpp
std::cout << pluss::duration::milliseconds({0,1,2});
```
```text
Duration(shape=[3], dim=1, numel=3)
[ 0 days 00:00:00.000000000, 0 days 00:00:00.001000000, 0 days 00:00:00.002000000 ]
```

```cpp
std::cout << pluss::duration::milliseconds({{1,2},{3,4}});
```
```text
Duration(shape=[2, 2], dim=2, numel=4)
[ 0 days 00:00:00.001000000 0 days 00:00:00.002000000 ]
[ 0 days 00:00:00.003000000 0 days 00:00:00.004000000 ]
```


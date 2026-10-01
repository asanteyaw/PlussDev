<!-- pluss-hand-authored -->
# pluss::duration::hours
[Back to Duration Factory API Reference](../../duration_factory_api_reference)

## Call Forms
```cpp
Duration pluss::duration::hours(integer_or_nested_values values);
Duration pluss::duration::hours(integer_range flat, integer_range sizes);
```

Each count is exactly 3,600 elapsed seconds. `hours()` is useful for fixed
offsets and windows; it never consults a timezone. In particular, adding a
24-hour Duration to a Timeline and advancing one local calendar day can differ
across a clock transition.

## Examples
```cpp
std::cout << pluss::duration::hours(-2);
```
```text
Duration(shape=[], dim=0, numel=1)
-1 days 22:00:00.000000000
```

```cpp
std::cout << pluss::duration::hours({0,1,2});
```
```text
Duration(shape=[3], dim=1, numel=3)
[ 0 days 00:00:00.000000000, 0 days 01:00:00.000000000, 0 days 02:00:00.000000000 ]
```

```cpp
std::cout << pluss::duration::hours({1,2,3,4}, {2,2});
```
```text
Duration(shape=[2, 2], dim=2, numel=4)
[ 0 days 01:00:00.000000000 0 days 02:00:00.000000000 ]
[ 0 days 03:00:00.000000000 0 days 04:00:00.000000000 ]
```


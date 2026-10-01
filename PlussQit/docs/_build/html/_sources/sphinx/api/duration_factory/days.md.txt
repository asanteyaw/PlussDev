<!-- pluss-hand-authored -->
# pluss::duration::days
[Back to Duration Factory API Reference](../../duration_factory_api_reference)

## Call Forms
```cpp
Duration pluss::duration::days(integer_or_nested_values values);
Duration pluss::duration::days(integer_range flat, integer_range sizes);
```

`days()` means exact 86,400-second spans. It is appropriate for elapsed-day
measurements and fixed stepping. It is not a civil-date offset: month length,
leap days, timezone transitions, and local midnight do not alter its width.
Use Timeline calendar methods when those rules are part of the meaning.

## Examples
```cpp
std::cout << pluss::duration::days(-2);
```
```text
Duration(shape=[], dim=0, numel=1)
-2 days 00:00:00.000000000
```

```cpp
std::cout << pluss::duration::days({0,1,2});
```
```text
Duration(shape=[3], dim=1, numel=3)
[ 0 days 00:00:00.000000000, 1 days 00:00:00.000000000, 2 days 00:00:00.000000000 ]
```

```cpp
std::cout << pluss::duration::days({{1,2},{3,4}});
```
```text
Duration(shape=[2, 2], dim=2, numel=4)
[ 1 days 00:00:00.000000000 2 days 00:00:00.000000000 ]
[ 3 days 00:00:00.000000000 4 days 00:00:00.000000000 ]
```


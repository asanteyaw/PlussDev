<!-- pluss-hand-authored -->
# pluss::duration::seconds
[Back to Duration Factory API Reference](../../duration_factory_api_reference)

## Call Forms
```cpp
Duration pluss::duration::seconds(integer_or_nested_values values);
Duration pluss::duration::seconds(integer_range flat, integer_range sizes);
```

Each lane becomes that many exact SI seconds, or one billion nanoseconds. This
is fixed elapsed time: timezone and civil-clock transitions are irrelevant.
Use Timeline calendar operations when “one day later” means a local calendar
transition rather than 86,400 elapsed seconds.

## Examples
```cpp
std::cout << pluss::duration::seconds(-2);
```
```text
Duration(shape=[], dim=0, numel=1)
-1 days 23:59:58.000000000
```

```cpp
std::cout << pluss::duration::seconds({0,1,2});
```
```text
Duration(shape=[3], dim=1, numel=3)
[ 0 days 00:00:00.000000000, 0 days 00:00:01.000000000, 0 days 00:00:02.000000000 ]
```

```cpp
std::cout << pluss::duration::seconds({1,2,3,4}, {2,2});
```
```text
Duration(shape=[2, 2], dim=2, numel=4)
[ 0 days 00:00:01.000000000 0 days 00:00:02.000000000 ]
[ 0 days 00:00:03.000000000 0 days 00:00:04.000000000 ]
```


<!-- pluss-hand-authored -->
# pluss::duration::make
[Back to Duration Factory API Reference](../../duration_factory_api_reference)

## Call Forms
```cpp
Duration pluss::duration::make(values, pluss::duration::Unit unit);
Duration pluss::duration::make(integer_range flat, integer_range sizes,
                               pluss::duration::Unit unit);
Duration pluss::make_duration(values, pluss::duration::Unit unit);
```

`make()` supplies the same exact scalar, range, nested, and explicit-shape
construction as the named unit factories, but chooses the scale at runtime.
`Unit` provides `Nanos`, `Microseconds`, `Milliseconds`, `Seconds`, `Minutes`,
`Hours`, `Days`, and `Weeks`. Use it for configuration-driven ingestion;
prefer a named factory when the unit is fixed in source code.

`pluss::make_duration` intentionally forwards to this contract. It does not
copy differently or infer a unit. Scaling and shape validation happen before a
result is returned, and invalid enum values or overflow throw.

## Examples
```cpp
std::cout << pluss::duration::make(
    {1,2,3,4}, {2,2}, pluss::duration::Unit::Minutes);
```
```text
Duration(shape=[2, 2], dim=2, numel=4)
[ 0 days 00:01:00.000000000 0 days 00:02:00.000000000 ]
[ 0 days 00:03:00.000000000 0 days 00:04:00.000000000 ]
```

```cpp
auto a = pluss::duration::make({1,2,3}, pluss::duration::Unit::Hours);
auto b = pluss::make_duration({1,2,3}, pluss::duration::Unit::Hours);
std::cout << std::boolalpha << (a.host() == b.host());
```
```text
true
```

```cpp
pluss::duration::make(1, static_cast<pluss::duration::Unit>(999));
```
```text
pluss::duration::make: unsupported unit
```


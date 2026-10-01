<!-- pluss-hand-authored -->
# pluss::duration::nanos

[Back to Duration Factory API Reference](../../duration_factory_api_reference)

## Call Forms
```cpp
Duration pluss::duration::nanos(integer value);
Duration pluss::duration::nanos(integer_range values);
Duration pluss::duration::nanos(nested_integer_values values);
Duration pluss::duration::nanos(integer_range flat, integer_range sizes);
```

`nanos()` is the lossless entrance for values already expressed in the native
storage unit. No scaling occurs, so every signed 64-bit value is retained
exactly, including values beyond floating-point integer precision. The reserved
`Duration::kNaT` value constructs an absent lane rather than a finite span.

Use it for serialized nanosecond counts, clock deltas, and precision-sensitive
interop. Prefer a named coarser unit when literals such as `60` are intended to
mean seconds or minutes; `nanos(60)` always means sixty nanoseconds.

## Examples
```cpp
std::cout << pluss::duration::nanos(-2);
```
```text
Duration(shape=[], dim=0, numel=1)
-1 days 23:59:59.999999998
```

```cpp
std::cout << pluss::duration::nanos({{1,2},{3,4}});
```
```text
Duration(shape=[2, 2], dim=2, numel=4)
[ 0 days 00:00:00.000000001 0 days 00:00:00.000000002 ]
[ 0 days 00:00:00.000000003 0 days 00:00:00.000000004 ]
```

```cpp
auto exact = pluss::duration::nanos(9'007'199'254'740'993LL);
std::cout << exact.host()[0];
```
```text
9007199254740993
```

```cpp
std::cout << pluss::duration::nanos(pluss::Duration::kNaT);
```
```text
Duration(shape=[], dim=0, numel=1)
NaT
```


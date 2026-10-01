<!-- pluss-hand-authored -->
# pluss::duration::full
[Back to Duration Factory API Reference](../../duration_factory_api_reference)

## Signature
```cpp
Duration pluss::duration::full(integer_range sizes,
                               const Duration& fill_value);
```

`full()` allocates the requested shape and repeats one scalar Duration exactly.
The fill may be finite, negative, or `NaT`; a non-scalar fill is rejected
rather than broadcast. Shape validation precedes allocation, and native
nanoseconds are copied without floating conversion.

## Examples
```cpp
std::cout << pluss::duration::full({2,2}, pluss::duration::seconds(-3));
```
```text
Duration(shape=[2, 2], dim=2, numel=4)
[ -1 days 23:59:57.000000000 -1 days 23:59:57.000000000 ]
[ -1 days 23:59:57.000000000 -1 days 23:59:57.000000000 ]
```

```cpp
std::cout << pluss::duration::full(
    {3}, pluss::duration::nanos(pluss::Duration::kNaT));
```
```text
Duration(shape=[3], dim=1, numel=3)
[ NaT, NaT, NaT ]
```

```cpp
auto empty = pluss::duration::full({2,0}, pluss::duration::seconds(7));
pluss::duration::full({2}, pluss::duration::seconds({1,2}));
// throws: pluss::duration::full: fill_value must be a scalar Duration
```


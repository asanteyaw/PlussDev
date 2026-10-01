<!-- pluss-hand-authored -->
# pluss::duration::full_like
[Back to Duration Factory API Reference](../../duration_factory_api_reference)

## Signature
```cpp
Duration pluss::duration::full_like(const Duration& like,
                                    const Duration& fill_value);
```

`full_like()` combines the shape of `like` with the exact scalar value of
`fill_value`. It allocates independent packed storage; neither input is
mutated or retained as backing storage. The fill must be scalar.

## Examples
```cpp
auto like = pluss::duration::seconds({1,2,3,4}, {2,2});
std::cout << pluss::duration::full_like(like, pluss::duration::nanos(7));
```
```text
Duration(shape=[2, 2], dim=2, numel=4)
[ 0 days 00:00:00.000000007 0 days 00:00:00.000000007 ]
[ 0 days 00:00:00.000000007 0 days 00:00:00.000000007 ]
```

```cpp
auto scalar = pluss::duration::full_like(
    pluss::duration::hours(1), pluss::duration::minutes(-5));
```

```cpp
auto absent = pluss::duration::full_like(
    like, pluss::duration::nanos(pluss::Duration::kNaT));
```


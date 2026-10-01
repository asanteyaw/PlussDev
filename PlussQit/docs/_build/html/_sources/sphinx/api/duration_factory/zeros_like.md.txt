<!-- pluss-hand-authored -->
# pluss::duration::zeros_like
[Back to Duration Factory API Reference](../../duration_factory_api_reference)

## Signature
```cpp
Duration pluss::duration::zeros_like(const Duration& like);
```

`zeros_like()` takes only the receiver's shape and allocates packed zero-valued
Duration storage. It does not copy values or share storage. Scalar, N-D, and
zero-element geometry are preserved exactly.

## Examples
```cpp
auto like = pluss::duration::seconds({1,2,3,4}, {2,2});
std::cout << pluss::duration::zeros_like(like);
```
```text
Duration(shape=[2, 2], dim=2, numel=4)
[ 0 days 00:00:00.000000000 0 days 00:00:00.000000000 ]
[ 0 days 00:00:00.000000000 0 days 00:00:00.000000000 ]
```

```cpp
auto scalar = pluss::duration::zeros_like(pluss::duration::hours(3));
// scalar.dim() == 0
```

```cpp
auto empty = pluss::duration::zeros_like(pluss::duration::zeros({2,0,3}));
// empty.sizes() == [2,0,3], empty.numel() == 0
```


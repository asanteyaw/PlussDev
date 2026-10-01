<!-- pluss-hand-authored -->
# pluss::duration::zeros
[Back to Duration Factory API Reference](../../duration_factory_api_reference)

## Signature
```cpp
Duration pluss::duration::zeros(integer_range sizes);
```

`zeros()` allocates a Duration of the requested shape and initializes every
lane to exactly zero nanoseconds. Shape `{}` creates a scalar; zero extents
create a defined empty N-D value. Rank above three, negative extents, and shape
product overflow are rejected. Allocation uses the packed integer zero path.

## Examples
```cpp
std::cout << pluss::duration::zeros({});
```
```text
Duration(shape=[], dim=0, numel=1)
0 days 00:00:00.000000000
```

```cpp
std::cout << pluss::duration::zeros({2,3});
```
```text
Duration(shape=[2, 3], dim=2, numel=6)
[ 0 days 00:00:00.000000000 0 days 00:00:00.000000000 0 days 00:00:00.000000000 ]
[ 0 days 00:00:00.000000000 0 days 00:00:00.000000000 0 days 00:00:00.000000000 ]
```

```cpp
auto empty = pluss::duration::zeros({2,0,3}); // shape retained, numel == 0
pluss::duration::zeros({2,-1});
// throws: pluss::duration::zeros: shape extents must be non-negative
```


# Series methods (method-first philosophy)

PlussQit prefers **methods on an existing Series** instead of duplicating free functions.

Examples:

```cpp
pluss::Series x = pluss::series::rand({1024}, {{"dtype","float32"}});
pluss::Series y = x.exp().add(1.0).div(2);     // scalar overloads
pluss::Series z = x.pile({y}, /*dim=*/0);      // Pluss naming deviations
```

## Naming deviations

- `pile(...)` is the preferred name for `stack(...)`
- `unite(...)` is the preferred name for `cat(...)`

(Existing Torch-like names are retained internally for compatibility and testing.)

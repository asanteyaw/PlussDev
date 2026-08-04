# Reductions

PlussQit exposes Torch-inspired reductions as **Series methods**:

```cpp
pluss::Series a = pluss::series::rand({2,3,4});
auto s = a.sum();           // scalar
auto s0 = a.sum(0);         // reduce over dim 0
auto m1 = a.mean(1, true);  // keepdim
auto mx = a.amax(2);
auto mn = a.amin(2);
auto p = a.prod(1);
auto b = a.all();           // bool scalar
auto any0 = a.any(0);
auto am = a.argmax(1);
```

Notes (CPU-only v1):
- `mean` promotes integral inputs to Float32 (Torch-like in practice).
- Float8 reductions are not yet supported (needs quantization policy).

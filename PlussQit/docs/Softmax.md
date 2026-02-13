# Softmax family

Torch-inspired softmax utilities are available as **Series methods**:

```cpp
auto y = x.softmax(dim);
auto l = x.log_softmax(dim);
auto s = x.logsumexp(dim);       // keepdim=false
auto s2 = x.logsumexp(dim, true);
```

Notes:
- CPU-only.
- Floating dtypes only (Float16/BFloat16/Float32/Float64).
- Complex is not supported (matches Torch behavior).

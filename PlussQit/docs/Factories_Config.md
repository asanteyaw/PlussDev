# Factories and Config

PlussQit supports both typed option tags and unordered key/value config lists.

## Typed option tags (fast, compile-time)

```cpp
auto x = pluss::zeros({128,64},
  pluss::cfg::dtype(pluss::ScalarType::Float16),
  pluss::cfg::pinned(false));
```

## Unordered key/value config list (ergonomic)

```cpp
auto x = pluss::zeros({128,64}, {{"dtype","float16"}, {"pinned", false}});
auto y = pluss::full({2,3}, 1.0, {{"dtype","bfloat16"}});
```

Supported CPU-only keys:
- `dtype`
- `pinned` / `pinned_memory`

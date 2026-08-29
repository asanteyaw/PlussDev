# `lymph::prep::StandardScaler::learned`

## Signature

```cpp
bool lymph::prep::StandardScaler::learned() const noexcept;
```

## What It Does

`learned()` tells you whether the scaler currently carries fitted state.

That is the cheap guard before `map(...)` if the scaler may still be in a
construction-only or configuration-only phase.

## Related Pages

- [learn](learn.md)
- [map](map.md)
- [summary](summary.md)

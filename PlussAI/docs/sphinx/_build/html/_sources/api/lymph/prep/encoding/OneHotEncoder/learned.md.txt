# `lymph::prep::OneHotEncoder::learned`

## Signature

```cpp
bool lymph::prep::OneHotEncoder::learned() const noexcept;
```

## What It Does

`learned()` tells you whether the encoder currently carries a learned category
layout.

That is the cheap guard before `map(...)` when the encoder may still be
configuration-only.

## Related Pages

- [learn](learn.md)
- [map](map.md)
- [summary](summary.md)

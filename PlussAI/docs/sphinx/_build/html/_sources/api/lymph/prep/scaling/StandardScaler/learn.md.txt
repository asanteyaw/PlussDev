# `lymph::prep::StandardScaler::learn`

## Signature

```cpp
lymph::prep::StandardScaler& lymph::prep::StandardScaler::learn(
    const pluss::table::DataFrame& frame,
    const lymph::options::Train& options = {});
```

## What It Does

`learn(...)` fits the scaler state without immediately materializing the mapped
frame.

Use it when you need a two-step flow:

- fit now
- map one or more later frames afterward

That pattern is useful when the same fitted scaler will be reused across
several inference or validation splits.

## Related Pages

- [learn_map](learn_map.md)
- [map](map.md)
- [learned](learned.md)

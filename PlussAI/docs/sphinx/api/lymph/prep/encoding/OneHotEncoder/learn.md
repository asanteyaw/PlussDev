# `lymph::prep::OneHotEncoder::learn`

## Signature

```cpp
lymph::prep::OneHotEncoder& lymph::prep::OneHotEncoder::learn(
    const pluss::table::DataFrame& frame,
    const lymph::options::Train& options = {});

lymph::prep::OneHotEncoder& lymph::prep::OneHotEncoder::learn(
    const pluss::table::Thread& feature,
    const lymph::options::Train& options = {});
```

## What It Does

`learn(...)` fits the category layout without immediately materializing the
encoded output.

Use it when you want to fit once and map later, especially when a learned
encoder will be reused across several later datasets or splits.

## Related Pages

- [learn_map](learn_map.md)
- [map](map.md)
- [learned](learned.md)

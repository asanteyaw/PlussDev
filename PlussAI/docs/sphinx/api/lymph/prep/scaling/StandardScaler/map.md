# `lymph::prep::StandardScaler::map`

## Signature

```cpp
pluss::table::DataFrame lymph::prep::StandardScaler::map(
    const pluss::table::DataFrame& frame,
    const lymph::options::Infer& options = {}) const;
```

## What It Does

`map(...)` applies previously learned scaling state to a new frame.

This is where the object proves its reuse value: the same centers and scales
learned earlier can be applied again without refitting.

## Related Pages

- [learn](learn.md)
- [learn_map](learn_map.md)
- [learned](learned.md)

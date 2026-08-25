---
orphan: true
---

# `lymph::metrics::ValidationOptions`

`ValidationOptions` is owned by the [`metrics`](../metrics.md) family and consumed
by the [`validate`](../split.md) family.

## Signature

```cpp
lymph::ValidationOptions options{};
lymph::ValidationPlan lymph::validate::cross(
    lymph::ValidationOptions options = {});
lymph::ValidationPlan lymph::validate::time(
    lymph::ValidationOptions options = {});
lymph::HoldoutPlan lymph::validate::holdout(...);
```

Use the main page here instead:

- [ValidationOptions](../metrics/ValidationOptions.md)

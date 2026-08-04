# `ClassMix`

## API Signature

- `ClassMix`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `ClassMix` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `Series labels = Series::undefined()`
- `Series counts = Series::undefined()`
- `Series proportions = Series::undefined()`
- `Series inverse_frequency = Series::undefined()`
- `std::int64_t sample_count = 0`
- `std::int64_t class_count = 0`
- `double imbalance_ratio = 1.0`

## Examples

```cpp
ClassMix cfg{};
// cfg.labels = ...;
```

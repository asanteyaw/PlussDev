# `IndexSplit`

## API Signature

- `IndexSplit`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `IndexSplit` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `Series train_indices = Series::undefined()`
- `Series test_indices = Series::undefined()`

## Examples

### Example 1

```cpp
  holdout_options.random.seed = 42;
  holdout_options.random.deterministic = true;

  StatusOr<IndexSplit> holdout_or = holdout_split(frame.ncases(), holdout_options);
  if (!holdout_or.ok()) {
    std::cerr << "holdout_split failed: " << holdout_or.status().message << "\n";
    return 1;
  }
  const IndexSplit holdout = holdout_or.move_value();

  const std::vector<std::int64_t> holdout_train_idx = holdout.train_indices.to(pluss::pCPU, ScalarType::Int64)
                                                           .reshape({holdout.train_indices.numel()})
                                                           .to_vec<std::int64_t>();
  const std::vector<std::int64_t> holdout_test_idx = holdout.test_indices.to(pluss::pCPU, ScalarType::Int64)
                                                         .reshape({holdout.test_indices.numel()})
                                                         .to_vec<std::int64_t>();

  const DataFrame train_frame_full = frame.take(holdout_train_idx);
  const DataFrame test_frame_full = frame.take(holdout_test_idx);

  const DataFrame train_x = train_frame_full.features({
```

**Expected output**

```text
IndexSplit configured
```

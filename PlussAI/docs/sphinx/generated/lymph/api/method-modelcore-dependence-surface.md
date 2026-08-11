# `ModelCore::dependence_surface`

## API Signature

- `ModelCore::dependence_surface(train: const LabeledBatch&, eval: const FeatureBatch&, feature_a: std::int64_t, feature_b: std::int64_t, grid_points_a: std::int64_t = 16, grid_points_b: std::int64_t = 16) -> DependenceSurface`

## Notes

Instance method on `self`.

## Inputs

- `eval` (`const FeatureBatch&`)
  Method argument; see signature type for accepted form.
- `feature_a` (`std::int64_t`)
  Input feature data used by this call.
- `feature_b` (`std::int64_t`)
  Input feature data used by this call.
- `grid_points_a` (`std::int64_t`), default: `16`
  Method argument; see signature type for accepted form.
- `grid_points_b` (`std::int64_t`), default: `16`
  Method argument; see signature type for accepted form.
- `train` (`const LabeledBatch&`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `DependenceSurface`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = ModelCore::dependence_surface(/* train */, /* eval */, /* feature_a */, /* feature_b */, /* grid_points_a */, /* grid_points_b */);
out.info("dependence_surface", value);
```

**Expected output**

```text
dependence_surface returned DependenceSurface
```

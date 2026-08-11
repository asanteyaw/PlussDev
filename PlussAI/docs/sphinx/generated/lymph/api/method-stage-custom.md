# `Stage::custom`

## API Signature

- `Stage::custom(name: std::string, map_fn: Fn, contract: TransformContract = callable_contract() -> CustomStage`

## Notes

Instance method on `self`.

## Inputs

- `contract` (`TransformContract`), default: `callable_contract(`
  Method argument; see signature type for accepted form.
- `map_fn` (`Fn`)
  Method argument; see signature type for accepted form.
- `name` (`std::string`)
  Method argument; see signature type for accepted form.

## Output

- Returns `CustomStage`.

## Options

Option semantics used by this method:

- `Stage` (`struct`):
  - `TransformKind kind = TransformKind::StandardScaler`
    Option field used by this method.
  - `TransformOptions options = prep::StandardScalerOptions{}`
    Family-specific options payload used by the selected model/transform family.

- `TransformContract` (`struct`):
  - `bool reversible = false`
    Option field used by this method.
  - `bool inverse_available = false`
    Option field used by this method.
  - `bool lossy = true`
    Option field used by this method.
  - `bool stateful = true`
    Option field used by this method.
  - `bool target_aware = false`
    Option field used by this method.
  - `bool changes_sample_count = false`
    Option field used by this method.
  - `bool changes_feature_count = false`
    Option field used by this method.
  - `bool needs_inverse_memory = false`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Stage::custom(/* name */, /* map_fn */, /* contract */);
out.info("custom", value);
```

**Expected output**

```text
custom returned CustomStage
```

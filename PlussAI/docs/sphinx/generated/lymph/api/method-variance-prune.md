# `variance_prune`

## API Signature

- `variance_prune(batch: const FeatureBatch&, variance_floor: double = 1e-12) -> VariancePrune`

## Notes

Namespace-level API function.

## Inputs

- `batch` (`const FeatureBatch&`)
  Input feature data used by this call.
- `variance_floor` (`double`), default: `1e-12`
  Method argument; see signature type for accepted form.

## Output

- Returns `VariancePrune`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = variance_prune(/* batch */, /* variance_floor */);
out.info("variance_prune", value);
```

**Expected output**

```text
variance_prune returned VariancePrune
```

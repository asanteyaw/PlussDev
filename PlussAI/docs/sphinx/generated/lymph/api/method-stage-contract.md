# `Stage::contract`

## API Signature

- `Stage::contract() -> TransformContract`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `TransformContract`.

## Options

Option semantics used by this method:

- `Stage` (`struct`):
  - `TransformKind kind = TransformKind::StandardScaler`
    Option field used by this method.
  - `TransformOptions options = prep::StandardScalerOptions{}`
    Family-specific options payload used by the selected model/transform family.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Stage::contract();
out.info("contract", value);
```

**Expected output**

```text
contract returned TransformContract
```

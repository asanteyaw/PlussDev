# `ModelCore::append`

## API Signature

- `ModelCore::append(stage: Stage, router: ColumnRouter) -> ModelCore&`

## Notes

Instance method on `self`.

## Inputs

- `router` (`ColumnRouter`)
  Method argument; see signature type for accepted form.
- `stage` (`Stage`)
  Method argument; see signature type for accepted form.

## Output

- Returns `ModelCore&`.

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
auto value = ModelCore::append(/* stage */, /* router */);
out.info("append", value);
```

**Expected output**

```text
append returned ModelCore&
```

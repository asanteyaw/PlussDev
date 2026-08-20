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

No explicit option struct/enum parameters were detected for this method.

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

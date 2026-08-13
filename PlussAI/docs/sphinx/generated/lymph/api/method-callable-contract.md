# `callable_contract`

## API Signature

- `callable_contract() -> TransformContract`

## Notes

Namespace-level API function.

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
auto value = callable_contract();
out.info("callable_contract", value);
```

**Expected output**

```text
callable_contract returned TransformContract
```

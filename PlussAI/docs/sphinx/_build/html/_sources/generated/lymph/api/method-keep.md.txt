# `keep`

## API Signature

- `keep() -> BalanceTarget`

## Notes

Namespace-level API function.

## Inputs

None.

## Output

- Returns `BalanceTarget`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = keep();
out.info("keep", value);
```

**Expected output**

```text
keep returned BalanceTarget
```

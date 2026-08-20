# `ModelCore::clone`

## API Signature

- `ModelCore::clone() -> ModelCore`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `ModelCore`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = ModelCore::clone();
out.info("clone", value);
```

**Expected output**

```text
clone returned ModelCore
```

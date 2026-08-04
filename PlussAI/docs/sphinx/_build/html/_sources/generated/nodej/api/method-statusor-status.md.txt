# `StatusOr::status`

## API Signature

- `StatusOr::status() -> const Status&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `const Status&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = StatusOr::status();
out.info("status", value);
```

**Expected output**

```text
status returned const Status&
```

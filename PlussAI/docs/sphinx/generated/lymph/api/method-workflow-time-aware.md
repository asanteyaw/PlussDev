# `Workflow::time_aware`

## API Signature

- `Workflow::time_aware() -> bool`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `bool`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Workflow::time_aware();
out.info("time_aware", value);
```

**Expected output**

```text
time_aware: true
```

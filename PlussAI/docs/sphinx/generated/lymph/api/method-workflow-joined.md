# `Workflow::joined`

## API Signature

- `Workflow::joined() -> bool`

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
auto value = Workflow::joined();
out.info("joined", value);
```

**Expected output**

```text
joined: true
```

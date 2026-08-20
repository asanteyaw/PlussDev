# `workflow::joined`

## API Signature

- `workflow::joined() -> bool`

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
auto value = workflow::joined();
out.info("joined", value);
```

**Expected output**

```text
joined: true
```

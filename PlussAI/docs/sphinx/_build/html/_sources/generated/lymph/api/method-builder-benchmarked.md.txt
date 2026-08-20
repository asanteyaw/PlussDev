# `Builder::benchmarked`

## API Signature

- `Builder::benchmarked() -> bool`

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
auto value = Builder::benchmarked();
out.info("benchmarked", value);
```

**Expected output**

```text
benchmarked: true
```

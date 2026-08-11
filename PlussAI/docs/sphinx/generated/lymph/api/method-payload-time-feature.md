# `Payload::time_feature`

## API Signature

- `Payload::time_feature() -> std::string`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `std::string`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Payload::time_feature();
out.info("time_feature", value);
```

**Expected output**

```text
time_feature returned std::string
```

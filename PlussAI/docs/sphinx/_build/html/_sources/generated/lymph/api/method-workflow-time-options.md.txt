# `workflow::time_options`

## API Signature

- `workflow::time_options() -> const std::optional<TimeOptions>&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `const std::optional<TimeOptions>&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = workflow::time_options();
out.info("time_options", value);
```

**Expected output**

```text
time_options returned const std::optional<TimeOptions>&
```

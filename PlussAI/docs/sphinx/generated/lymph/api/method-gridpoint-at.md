# `GridPoint::at`

## API Signature

- `GridPoint::at(axis_name: std::string_view) -> double`

## Notes

Instance method on `self`.

## Inputs

- `axis_name` (`std::string_view`)
  Input feature data used by this call.

## Output

- Returns `double`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = GridPoint::at(/* axis_name */);
out.info("at", value);
```

**Expected output**

```text
at: <numeric_result>
```

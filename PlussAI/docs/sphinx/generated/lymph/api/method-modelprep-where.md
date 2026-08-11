# `ModelPrep::where`

## API Signature

- `ModelPrep::where(predicate: std::function<bool(std::string_view feature_name, std::string_view dtype)>) -> ColumnRouter`

## Notes

Instance method on `self`.

## Inputs

- `predicate` (`std::function<bool(std::string_view feature_name, std::string_view dtype)>`)
  Method argument; see signature type for accepted form.

## Output

- Returns `ColumnRouter`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = ModelPrep::where(/* predicate */);
out.info("where", value);
```

**Expected output**

```text
where returned ColumnRouter
```

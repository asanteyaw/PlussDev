# `where`

## API Signature

- `where(predicate: std::function<bool(std::string_view feature_name, std::string_view dtype)>) -> ColumnRouter`
- `where(predicate: std::function<bool(std::string_view feature_name, std::string_view dtype)>) -> ColumnRouter`

## Notes

Namespace-level API function.

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
auto value = where(/* predicate */);
out.info("where", value);
```

**Expected output**

```text
where returned ColumnRouter
```

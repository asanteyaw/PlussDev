# `Flow::nested_param`

## API Signature

- `Flow::nested_param(path: std::string_view) -> StatusOr<double>`

## Notes

Instance method on `self`.

## Inputs

- `path` (`std::string_view`)
  Path to persisted model/data artifact.

## Output

- Returns `StatusOr<double>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = Flow::nested_param(/* path */);
if (!res.ok()) {
  out.error("nested_param failed", res.status().message);
} else {
  out.info("nested_param ok");
}
```

**Expected output**

```text
nested_param: ok
status.ok() == true
result.value() is available
```

# `bool`

## API Signature

- `std::function<bool(std::string_view, std::string_view)> predicate_() -> auto`

## Notes

Namespace-level API function.

## Inputs

None.

## Output

- Returns `auto`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = std::function<bool(std::string_view, std::string_view)> predicate_();
out.info("bool", value);
```

**Expected output**

```text
bool returned auto
```

# `Builder::get`

## API Signature

- `Builder::get(name: std::string_view) -> const Model&`
- `Builder::get(name: std::string_view) -> Model&`

## Notes

Instance method on `self`.

## Inputs

- `name` (`std::string_view`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Model&`.
- Returns `const Model&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Builder::get(/* name */);
out.info("get", value);
```

**Expected output**

```text
get returned const Model&
```

# `Builder::has`

## API Signature

- `Builder::has(name: std::string_view) -> bool`

## Notes

Instance method on `self`.

## Inputs

- `name` (`std::string_view`)
  Method argument; see signature type for accepted form.

## Output

- Returns `bool`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Builder::has(/* name */);
out.info("has", value);
```

**Expected output**

```text
has: true
```

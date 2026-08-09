# `Builder::eject`

## API Signature

- `Builder::eject(name: std::string_view) -> Model`

## Notes

Instance method on `self`.

## Inputs

- `name` (`std::string_view`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Model`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Builder::eject(/* name */);
out.info("eject", value);
```

**Expected output**

```text
eject returned Model
```

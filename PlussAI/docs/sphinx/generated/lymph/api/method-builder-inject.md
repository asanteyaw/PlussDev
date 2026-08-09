# `Builder::inject`

## API Signature

- `Builder::inject(name: std::string, model: Model) -> Builder&`

## Notes

Instance method on `self`.

## Inputs

- `model` (`Model`)
  Method argument; see signature type for accepted form.
- `name` (`std::string`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Builder&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Builder::inject(/* name */, /* model */);
out.info("inject", value);
```

**Expected output**

```text
inject returned Builder&
```

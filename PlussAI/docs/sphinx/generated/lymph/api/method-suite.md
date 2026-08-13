# `suite`

## API Signature

- `suite(builder: const Builder&) -> Suite`

## Notes

Namespace-level API function.

## Inputs

- `builder` (`const Builder&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Suite`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = suite(/* builder */);
out.info("suite", value);
```

**Expected output**

```text
suite returned Suite
```

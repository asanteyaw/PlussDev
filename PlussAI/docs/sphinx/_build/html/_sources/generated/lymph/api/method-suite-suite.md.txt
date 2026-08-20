# `Suite::Suite`

## API Signature

- `Suite(builder: const Builder&) -> auto`

## Notes

Instance method on `self`.

## Inputs

- `builder` (`const Builder&`)
  Method argument; see signature type for accepted form.

## Output

- Returns method result for the selected overload.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Suite(/* builder */);
out.info("Suite", value);
```

**Expected output**

```text
Suite returned result
```

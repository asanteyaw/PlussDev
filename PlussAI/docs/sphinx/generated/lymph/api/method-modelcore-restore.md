# `ModelCore::restore`

## API Signature

- `ModelCore::restore(bundle: const io::FlowBundle&) -> ModelCore`

## Notes

Instance method on `self`.

## Inputs

- `bundle` (`const io::FlowBundle&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `ModelCore`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = ModelCore::restore(/* bundle */);
out.info("restore", value);
```

**Expected output**

```text
restore returned ModelCore
```

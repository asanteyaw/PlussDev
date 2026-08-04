# `encoded_param_table`

## API Signature

- `encoded_param_table(report: const TuneReport&) -> Series`

## Notes

Namespace-level API function.

## Inputs

- `report` (`const TuneReport&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Series`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = encoded_param_table(/* report */);
out.info("encoded_param_table", value);
```

**Expected output**

```text
encoded_param_table returned Series
```

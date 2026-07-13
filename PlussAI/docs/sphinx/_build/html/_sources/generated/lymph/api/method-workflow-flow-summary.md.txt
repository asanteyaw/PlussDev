# `Workflow::flow_summary`

## API Signature

- `Workflow::flow_summary() -> std::string`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `std::string`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Workflow::flow_summary();
out.info("flow_summary", value);
```

**Expected output**

```text
flow_summary returned std::string
```

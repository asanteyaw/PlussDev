# `Workflow::dump_model_state`

## API Signature

- `Workflow::dump_model_state() -> std::vector<Series>`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `std::vector<Series>`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Workflow::dump_model_state();
out.info("dump_model_state", value);
```

**Expected output**

```text
dump_model_state returned std::vector<Series>
```

# `Workflow::save_blueprint`

## API Signature

- `Workflow::save_blueprint() -> std::string`

## Notes

Persists the current fitted artifact so it can be loaded for inference later.

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
auto value = Workflow::save_blueprint();
out.info("save_blueprint", value);
```

**Expected output**

```text
save_blueprint returned std::string
```

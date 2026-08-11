# `Model::dump_transform_states`

## API Signature

- `Model::dump_transform_states() -> std::vector<std::vector<Series>>`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `std::vector<std::vector<Series>>`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Model::dump_transform_states();
out.info("dump_transform_states", value);
```

**Expected output**

```text
dump_transform_states returned std::vector<std::vector<Series>>
```

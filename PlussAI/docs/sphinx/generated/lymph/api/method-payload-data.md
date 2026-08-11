# `Payload::data`

## API Signature

- `Payload::data() -> const DataFrameBatchOptions&`
- `Payload::data() -> DataFrameBatchOptions&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `DataFrameBatchOptions&`.
- Returns `const DataFrameBatchOptions&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Payload::data();
out.info("data", value);
```

**Expected output**

```text
data returned const DataFrameBatchOptions&
```

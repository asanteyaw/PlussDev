# `Workflow::stage_count`

## API Signature

- `Workflow::stage_count() -> std::int64_t`

## Notes

Reports a cardinality/count statistic from the current object state.

## Inputs

None.

## Output

- Returns `std::int64_t`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Workflow::stage_count();
out.info("stage_count", value);
```

**Expected output**

```text
stage_count: <numeric_result>
```

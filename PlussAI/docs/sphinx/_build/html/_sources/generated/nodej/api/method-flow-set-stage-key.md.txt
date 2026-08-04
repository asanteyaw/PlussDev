# `Flow::set_stage_key`

## API Signature

- `Flow::set_stage_key(stage_index: std::int64_t, stage_key: std::string) -> Status`

## Notes

Sets or updates internal configuration/state used by later train/infer/evaluate calls.

## Inputs

- `stage_index` (`std::int64_t`)
  Input feature data used by this call.
- `stage_key` (`std::string`)
  Target values aligned to input observations.

## Output

- Returns a status object indicating success/failure and an error message on failure.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto st = Flow::set_stage_key(/* stage_index */, /* stage_key */);
if (!st.ok()) {
  out.error("set_stage_key failed", st.message);
} else {
  out.info("set_stage_key ok");
}
```

**Expected output**

```text
set_stage_key: ok
status.ok() == true
```

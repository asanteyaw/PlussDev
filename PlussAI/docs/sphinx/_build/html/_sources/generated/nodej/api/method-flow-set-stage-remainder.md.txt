# `Flow::set_stage_remainder`

## API Signature

- `Flow::set_stage_remainder(stage_index: std::int64_t, remainder: RouteRemainderPolicy) -> Status`

## Notes

Sets or updates internal configuration/state used by later train/infer/evaluate calls.

## Inputs

- `remainder` (`RouteRemainderPolicy`)
  Method argument; see signature type for accepted form.
- `stage_index` (`std::int64_t`)
  Input feature data used by this call.

## Output

- Returns a status object indicating success/failure and an error message on failure.

## Options

Option semantics used by this method:

- `RouteRemainderPolicy` (`enum`):
  - `PassThrough`: Accepted enum choice for this method.
  - `Drop`: Accepted enum choice for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto st = Flow::set_stage_remainder(/* stage_index */, /* remainder */);
if (!st.ok()) {
  out.error("set_stage_remainder failed", st.message);
} else {
  out.info("set_stage_remainder ok");
}
```

**Expected output**

```text
set_stage_remainder: ok
status.ok() == true
```

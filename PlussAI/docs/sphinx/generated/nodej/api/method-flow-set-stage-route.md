# `Flow::set_stage_route`

## API Signature

- `Flow::set_stage_route(stage_index: std::int64_t, feature_route: RangeView::Int) -> Status`
- `Flow::set_stage_route(stage_index: std::int64_t, feature_route: std::initializer_list<std::int64_t>) -> Status`

## Notes

Sets or updates internal configuration/state used by later train/infer/evaluate calls.

## Inputs

- `feature_route` (`RangeView::Int | std::initializer_list<std::int64_t>`)
  Input feature data used by this call.
- `stage_index` (`std::int64_t`)
  Input feature data used by this call.

## Output

- Returns a status object indicating success/failure and an error message on failure.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto st = Flow::set_stage_route(/* stage_index */, /* feature_route */);
if (!st.ok()) {
  out.error("set_stage_route failed", st.message);
} else {
  out.info("set_stage_route ok");
}
```

**Expected output**

```text
set_stage_route: ok
status.ok() == true
```

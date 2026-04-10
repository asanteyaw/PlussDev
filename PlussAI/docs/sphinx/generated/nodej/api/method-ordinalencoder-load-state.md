# `OrdinalEncoder::load_state`

## API Signature

- `OrdinalEncoder::load_state(state: RangeView::Series) -> Status`

## Notes

Loads a previously persisted artifact and restores its inference-ready state.

## Inputs

- `state` (`RangeView::Series`)
  Randomization control for reproducible behavior.

## Output

- Returns a status object indicating success/failure and an error message on failure.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto st = OrdinalEncoder::load_state(/* state */);
if (!st.ok()) {
  out.error("load_state failed", st.message);
} else {
  out.info("load_state ok");
}
```

**Expected output**

```text
load_state: ok
status.ok() == true
```

# `Flow::load_transform_states`

## API Signature

- `Flow::load_transform_states(states: const std::vector<std::vector<Series>>&) -> Status`

## Notes

Loads a previously persisted artifact and restores its inference-ready state.

## Inputs

- `states` (`const std::vector<std::vector<Series>>&`)
  Randomization control for reproducible behavior.

## Output

- Returns a status object indicating success/failure and an error message on failure.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto st = Flow::load_transform_states(/* states */);
if (!st.ok()) {
  out.error("load_transform_states failed", st.message);
} else {
  out.info("load_transform_states ok");
}
```

**Expected output**

```text
load_transform_states: ok
status.ok() == true
```

# `MinMaxScaler::load_state`

## API Signature

- `MinMaxScaler::load_state(state: const std::vector<Series>&) -> Status`

## Notes

Loads a previously persisted artifact and restores its inference-ready state.

## Inputs

- `state` (`const std::vector<Series>&`)
  Randomization control for reproducible behavior.

## Output

- Returns a status object indicating success/failure and an error message on failure.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto st = MinMaxScaler::load_state(/* state */);
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

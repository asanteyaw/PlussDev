# `LexiconMapper::load_state`

## API Signature

- `LexiconMapper::load_state(state: const LexiconMapperState&) -> Status`

## Notes

Loads a previously persisted artifact and restores its inference-ready state.

## Inputs

- `state` (`const LexiconMapperState&`)
  Randomization control for reproducible behavior.

## Output

- Returns a status object indicating success/failure and an error message on failure.

## Options

Option semantics used by this method:

- `LexiconMapperState` (`struct`):
  - `LexiconMapOptions options{}`
    Family-specific options payload used by the selected model/transform family.
  - `bool learned = false`
    Option field used by this method.
  - `std::int64_t learned_document_count = 0`
    Option field used by this method.
  - `std::vector<std::string> vocabulary{}`
    Option field used by this method.
  - `std::vector<std::int64_t> document_frequency{}`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto st = LexiconMapper::load_state(/* state */);
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

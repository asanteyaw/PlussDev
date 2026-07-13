# `Flow::load_blueprint`

## API Signature

- `Flow::load_blueprint(text: std::string_view) -> Status`

## Notes

Loads a previously persisted artifact and restores its inference-ready state.

## Inputs

- `text` (`std::string_view`)
  Input feature data used by this call.

## Output

- Returns a status object indicating success/failure and an error message on failure.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto st = Flow::load_blueprint(/* text */);
if (!st.ok()) {
  out.error("load_blueprint failed", st.message);
} else {
  out.info("load_blueprint ok");
}
```

**Expected output**

```text
load_blueprint: ok
status.ok() == true
```

# `Flow::append_transform`

## API Signature

- `Flow::append_transform(transformer: std::unique_ptr<TransformerBase>) -> Status`

## Notes

Instance method on `self`.

## Inputs

- `transformer` (`std::unique_ptr<TransformerBase>`)
  Method argument; see signature type for accepted form.

## Output

- Returns a status object indicating success/failure and an error message on failure.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto st = Flow::append_transform(/* transformer */);
if (!st.ok()) {
  out.error("append_transform failed", st.message);
} else {
  out.info("append_transform ok");
}
```

**Expected output**

```text
append_transform: ok
status.ok() == true
```

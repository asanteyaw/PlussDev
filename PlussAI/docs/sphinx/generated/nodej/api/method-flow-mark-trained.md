# `Flow::mark_trained`

## API Signature

- `Flow::mark_trained(trained: bool) -> void`

## Notes

Fits model/transform state from the provided training data. Call this before inference-oriented methods.

## Inputs

- `trained` (`bool`)
  Method argument; see signature type for accepted form.

## Output

- No direct return value; operation applies side effects to object state.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
Flow::mark_trained(/* trained */);
out.info("mark_trained completed");
```

**Expected output**

```text
mark_trained completed
```

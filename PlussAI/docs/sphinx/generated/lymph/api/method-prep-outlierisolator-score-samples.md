# `prep::OutlierIsolator::score_samples`

## API Signature

- `prep::OutlierIsolator::score_samples(frame: const pluss::table::DataFrame&, options: const InferOptions&) -> pluss::table::Thread`

## Notes

Instance method on `self`.

## Inputs

- `frame` (`const pluss::table::DataFrame&`)
  Input feature data used by this call.
- `options` (`const InferOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `pluss::table::Thread`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = prep::OutlierIsolator::score_samples(/* frame */, /* options */);
out.info("score_samples", value);
```

**Expected output**

```text
score_samples returned pluss::table::Thread
```

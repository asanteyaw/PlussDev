# `prep::OutlierIsolator::learn_infer`

## API Signature

- `prep::OutlierIsolator::learn_infer(frame: const pluss::table::DataFrame&, learn_options: const TrainOptions&, infer_options: const InferOptions&) -> pluss::table::Thread`

## Notes

Fits model/transform state from the provided training data. Call this before inference-oriented methods.

## Inputs

- `frame` (`const pluss::table::DataFrame&`)
  Input feature data used by this call.
- `infer_options` (`const InferOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.
- `learn_options` (`const TrainOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `pluss::table::Thread`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = prep::OutlierIsolator::learn_infer(/* frame */, /* learn_options */, /* infer_options */);
out.info("learn_infer", value);
```

**Expected output**

```text
learn_infer returned pluss::table::Thread
```

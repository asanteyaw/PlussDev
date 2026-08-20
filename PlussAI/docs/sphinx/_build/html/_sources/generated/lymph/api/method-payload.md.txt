# `Payload`

## API Signature

- `Payload(arg1: FeatureBatch features) : features_(std::move(features)) -> auto`
- `Payload(arg1: LabeledBatch labeled) : labeled_(std::move(labeled)) -> auto`
- `Payload(frame: pluss::table::DataFrame, arg2: DataFrameBatchOptions options) : frame_(std::move(frame)), arg3: options_(std::move(options)) -> auto`

## Notes

Loads a previously persisted artifact and restores its inference-ready state.

## Inputs

- `arg1` (`FeatureBatch features) : features_(std::move(features) | LabeledBatch labeled) : labeled_(std::move(labeled)`)
  Method argument; see signature type for accepted form.
- `arg2` (`DataFrameBatchOptions options) : frame_(std::move(frame))`)
  Method argument; see signature type for accepted form.
- `arg3` (`options_(std::move(options)`)
  Method argument; see signature type for accepted form.
- `frame` (`pluss::table::DataFrame`)
  Input feature data used by this call.

## Output

- Returns method result for the selected overload.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Payload(/* arg1 */);
out.info("Payload", value);
```

**Expected output**

```text
Payload returned result
```

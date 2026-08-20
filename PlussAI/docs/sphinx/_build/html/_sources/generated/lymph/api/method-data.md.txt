# `data`

## API Signature

- `data(frame: const pluss::table::DataFrame&, options: const DataFrameBatchOptions&) -> Runner`
- `data(x: const pluss::table::DataFrame&) -> Runner`

## Notes

Namespace-level API function.

## Inputs

- `frame` (`const pluss::table::DataFrame&`)
  Input feature data used by this call.
- `options` (`const DataFrameBatchOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.
- `x` (`const pluss::table::DataFrame&`)
  Input feature data used by this call.

## Output

- Returns `Runner`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = data(/* frame */, /* options */);
out.info("data", value);
```

**Expected output**

```text
data returned Runner
```

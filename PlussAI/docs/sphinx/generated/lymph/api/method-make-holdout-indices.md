# `make_holdout_indices`

## API Signature

- `make_holdout_indices(frame: const pluss::table::DataFrame&, options: const HoldoutSplitOptions&) -> IndexSplit`

## Notes

Namespace-level API function.

## Inputs

- `frame` (`const pluss::table::DataFrame&`)
  Input feature data used by this call.
- `options` (`const HoldoutSplitOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `IndexSplit`.

## Options

Option semantics used by this method:

- `HoldoutSplitOptions` (`struct`):
  - `double test_size = 0.2`
    Option field used by this method.
  - `Series stratify = Series::undefined()`
    Option field used by this method.
  - `std::string stratify_feature{}`
    Option field used by this method.
  - `std::uint64_t random_state = 0`
    Random seed for reproducible behavior.
  - `bool deterministic = true`
    Option field used by this method.
  - `bool shuffle = true`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = make_holdout_indices(/* frame */, /* options */);
out.info("make_holdout_indices", value);
```

**Expected output**

```text
make_holdout_indices returned IndexSplit
```

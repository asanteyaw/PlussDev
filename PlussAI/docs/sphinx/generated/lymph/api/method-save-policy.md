# `save_policy`

## API Signature

- `save_policy(bundle_path: const std::string&, policy: const Policy&) -> void`

## Notes

Persists the current fitted artifact so it can be loaded for inference later.

## Inputs

- `bundle_path` (`const std::string&`)
  Path to persisted model/data artifact.
- `policy` (`const Policy&`)
  Target values aligned to input observations.

## Output

- No direct return value; operation applies side effects to object state.

## Options

Option semantics used by this method:

- `Policy` (`struct`):
  - `std::vector<std::string> drop_columns{}`
    Option field used by this method.
  - `std::vector<std::string> require_columns{}`
    Option field used by this method.
  - `std::vector<std::string> forbid_columns{}`
    Option field used by this method.
  - `std::vector<std::string> reject_missing{}`
    Option field used by this method.
  - `std::vector<std::string> allow_missing{}`
    Option field used by this method.
  - `bool filter_nulls = false`
    Option field used by this method.
  - `std::vector<std::string> null_filter_columns{}`
    Option field used by this method.
  - `bool drop_only_if_all_null = false`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
save_policy(/* bundle_path */, /* policy */);
out.info("save_policy completed");
```

**Expected output**

```text
save_policy completed
```

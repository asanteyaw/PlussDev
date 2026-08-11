# `restore`

## API Signature

- `restore(bundle: const io::FlowBundle&, policy: Policy) -> Workflow`

## Notes

Namespace-level API function.

## Inputs

- `bundle` (`const io::FlowBundle&`)
  Method argument; see signature type for accepted form.
- `policy` (`Policy`)
  Target values aligned to input observations.

## Output

- Returns `Workflow`.

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
auto value = restore(/* bundle */, /* policy */);
out.info("restore", value);
```

**Expected output**

```text
restore returned Workflow
```

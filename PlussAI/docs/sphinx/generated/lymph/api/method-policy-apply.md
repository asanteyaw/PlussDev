# `Policy::apply`

## API Signature

- `Policy::apply(frame: const pluss::table::DataFrame&) -> pluss::table::DataFrame`

## Notes

Instance method on `self`.

## Inputs

- `frame` (`const pluss::table::DataFrame&`)
  Input feature data used by this call.

## Output

- Returns `pluss::table::DataFrame`.

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
auto value = Policy::apply(/* frame */);
out.info("apply", value);
```

**Expected output**

```text
apply returned pluss::table::DataFrame
```

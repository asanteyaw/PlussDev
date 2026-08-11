# `Policy::reject`

## API Signature

- `Policy::reject(names: std::initializer_list<std::string>) -> Policy&`
- `Policy::reject(names: std::vector<std::string>) -> Policy&`

## Notes

Instance method on `self`.

## Inputs

- `names` (`std::initializer_list<std::string> | std::vector<std::string>`)
  List-like argument; pass values in the order expected by the signature.

## Output

- Returns `Policy&`.

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
auto value = Policy::reject(/* names */);
out.info("reject", value);
```

**Expected output**

```text
reject returned Policy&
```

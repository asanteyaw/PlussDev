# `ModelPrep::remaining`

## API Signature

- `ModelPrep::remaining(router: ColumnRouter) -> ColumnRouter`
- `ModelPrep::remaining(values: std::initializer_list<std::string>) -> ColumnRouter`
- `ModelPrep::remaining(values: std::vector<std::string>) -> ColumnRouter`

## Notes

Instance method on `self`.

## Inputs

- `router` (`ColumnRouter`)
  Method argument; see signature type for accepted form.
- `values` (`std::initializer_list<std::string> | std::vector<std::string>`)
  List-like argument; pass values in the order expected by the signature.

## Output

- Returns `ColumnRouter`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = ModelPrep::remaining(/* router */);
out.info("remaining", value);
```

**Expected output**

```text
remaining returned ColumnRouter
```

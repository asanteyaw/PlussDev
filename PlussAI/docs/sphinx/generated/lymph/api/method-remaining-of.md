# `remaining_of`

## API Signature

- `remaining_of(other: ColumnRouter) -> ColumnRouter`
- `remaining_of(names: std::vector<std::string>) -> ColumnRouter`
- `remaining_of(other: ColumnRouter) -> ColumnRouter`
- `remaining_of(names: std::vector<std::string>) -> ColumnRouter`

## Notes

Namespace-level API function.

## Inputs

- `names` (`std::vector<std::string>`)
  List-like argument; pass values in the order expected by the signature.
- `other` (`ColumnRouter`)
  Method argument; see signature type for accepted form.

## Output

- Returns `ColumnRouter`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = remaining_of(/* other */);
out.info("remaining_of", value);
```

**Expected output**

```text
remaining_of returned ColumnRouter
```

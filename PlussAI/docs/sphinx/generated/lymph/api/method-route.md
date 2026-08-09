# `route`

## API Signature

- `route(ordered_names: const std::vector<std::string>&, names: RangeView::Of<std::string_view>) -> std::vector<std::int64_t>`
- `route(ordered_names: const std::vector<std::string>&, names: std::initializer_list<std::string_view>) -> std::vector<std::int64_t>`
- `route(frame: const pluss::table::DataFrame&, names: RangeView::Of<std::string_view>) -> std::vector<std::int64_t>`
- `route(frame: const pluss::table::DataFrame&, names: std::initializer_list<std::string_view>) -> std::vector<std::int64_t>`

## Notes

Namespace-level API function.

## Inputs

- `frame` (`const pluss::table::DataFrame&`)
  Input feature data used by this call.
- `names` (`RangeView::Of<std::string_view> | std::initializer_list<std::string_view>`)
  List-like argument; pass values in the order expected by the signature.
- `ordered_names` (`const std::vector<std::string>&`)
  List-like argument; pass values in the order expected by the signature.

## Output

- Returns `std::vector<std::int64_t>`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = route(/* ordered_names */, /* names */);
out.info("route", value);
```

**Expected output**

```text
route returned std::vector<std::int64_t>
```

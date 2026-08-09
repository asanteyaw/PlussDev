# `route_where`

## API Signature

- `route_where(frame: const pluss::table::DataFrame&, predicate: const std::function<bool(std::string_view feature_name, std::string_view dtype)>&) -> std::vector<std::int64_t>`
- `route_where(frame: const pluss::table::DataFrame&, ordered_names: const std::vector<std::string>&, predicate: const std::function<bool(std::string_view feature_name, std::string_view dtype)>&) -> std::vector<std::int64_t>`

## Notes

Namespace-level API function.

## Inputs

- `frame` (`const pluss::table::DataFrame&`)
  Input feature data used by this call.
- `ordered_names` (`const std::vector<std::string>&`)
  List-like argument; pass values in the order expected by the signature.
- `predicate` (`const std::function<bool(std::string_view feature_name, std::string_view dtype)>&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `std::vector<std::int64_t>`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = route_where(/* frame */, /* predicate */);
out.info("route_where", value);
```

**Expected output**

```text
route_where returned std::vector<std::int64_t>
```

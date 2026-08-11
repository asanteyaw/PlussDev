# `build`

## API Signature

- `build() -> Builder`
- `build(entries: const std::map<std::string, Model>&) -> Builder`
- `build(entries: const std::vector<std::pair<std::string, Model>>&) -> Builder`
- `build(entries: std::initializer_list<std::pair<std::string, Model>>) -> Builder`
- `build(entries: std::map<std::string, Model>&&) -> Builder`
- `build(entries: std::vector<std::pair<std::string, Model>>&&) -> Builder`
- `build(axes: std::vector<GridAxis>, options: TuneOptions) -> Search`

## Notes

Namespace-level API function.

## Inputs

- `axes` (`std::vector<GridAxis>`)
  Input feature data used by this call.
- `entries` (`const std::map<std::string, Model>& | const std::vector<std::pair<std::string, Model>>& | std::initializer_list<std::pair<std::string, Model>> | std::map<std::string, Model>&& | std::vector<std::pair<std::string, Model>>&&`)
  List-like argument; pass values in the order expected by the signature.
- `options` (`TuneOptions`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `Builder`.
- Returns `Search`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = build();
out.info("build", value);
```

**Expected output**

```text
build returned Builder
```

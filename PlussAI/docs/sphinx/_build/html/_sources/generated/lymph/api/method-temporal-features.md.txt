# `temporal_features`

## API Signature

- `temporal_features(frame: const pluss::table::DataFrame&) -> std::vector<std::string>`

## Notes

Namespace-level API function.

## Inputs

- `frame` (`const pluss::table::DataFrame&`)
  Input feature data used by this call.

## Output

- Returns `std::vector<std::string>`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = temporal_features(/* frame */);
out.info("temporal_features", value);
```

**Expected output**

```text
temporal_features returned std::vector<std::string>
```

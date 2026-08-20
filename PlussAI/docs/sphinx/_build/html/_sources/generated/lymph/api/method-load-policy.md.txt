# `load_policy`

## API Signature

- `load_policy(bundle_path: const std::string&) -> Policy`

## Notes

Loads a previously persisted artifact and restores its inference-ready state.

## Inputs

- `bundle_path` (`const std::string&`)
  Path to persisted model/data artifact.

## Output

- Returns `Policy`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = load_policy(/* bundle_path */);
out.info("load_policy", value);
```

**Expected output**

```text
load_policy returned Policy
```

# `Payload::time`

## API Signature

- `Payload::time() -> const Series&`
- `Payload::time(values: Series) -> Payload&`
- `Payload::time(feature_name: std::string) -> Payload&`
- `Payload::time(options: TimeOptions) -> Payload&`

## Notes

Instance method on `self`.

## Inputs

- `feature_name` (`std::string`)
  Input feature data used by this call.
- `options` (`TimeOptions`)
  Configuration object controlling algorithm behavior and safety defaults.
- `values` (`Series`)
  Pluss numeric vector/tensor input.

## Output

- Returns `Payload&`.
- Returns `const Series&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Payload::time();
out.info("time", value);
```

**Expected output**

```text
time returned const Series&
```

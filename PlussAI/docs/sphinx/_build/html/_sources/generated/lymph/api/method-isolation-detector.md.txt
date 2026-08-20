# `isolation_detector`

## API Signature

- `isolation_detector(options: prep::OutlierIsolatorOptions) -> Model`
- `isolation_detector(options: prep::OutlierIsolatorOptions) -> Model`

## Notes

Namespace-level API function.

## Inputs

- `options` (`prep::OutlierIsolatorOptions`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `Model`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = isolation_detector(/* options */);
out.info("isolation_detector", value);
```

**Expected output**

```text
isolation_detector returned Model
```

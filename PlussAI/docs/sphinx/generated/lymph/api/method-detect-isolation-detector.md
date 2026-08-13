# `detect::isolation_detector`

## API Signature

- `detect::isolation_detector(options: prep::OutlierIsolatorOptions) -> detail::ModelCore`

## Notes

Instance method on `self`.

## Inputs

- `options` (`prep::OutlierIsolatorOptions`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `detail::ModelCore`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = detect::isolation_detector(/* options */);
out.info("isolation_detector", value);
```

**Expected output**

```text
isolation_detector returned detail::ModelCore
```

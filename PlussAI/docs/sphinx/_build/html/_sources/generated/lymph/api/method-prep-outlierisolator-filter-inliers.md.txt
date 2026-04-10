# `prep::OutlierIsolator::filter_inliers`

## API Signature

- `prep::OutlierIsolator::filter_inliers(frame: const pluss::table::DataFrame&, options: const InferOptions&) -> pluss::table::DataFrame`

## Notes

Instance method on `self`.

## Inputs

- `frame` (`const pluss::table::DataFrame&`)
  Input feature data used by this call.
- `options` (`const InferOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `pluss::table::DataFrame`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = prep::OutlierIsolator::filter_inliers(/* frame */, /* options */);
out.info("filter_inliers", value);
```

**Expected output**

```text
filter_inliers returned pluss::table::DataFrame
```

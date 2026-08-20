# `Plan::last`

## API Signature

- `Plan::last(frame: const pluss::table::DataFrame&, time_feature: std::string_view) -> prep::TimeSeriesSplit`

## Notes

Instance method on `self`.

## Inputs

- `frame` (`const pluss::table::DataFrame&`)
  Input feature data used by this call.
- `time_feature` (`std::string_view`)
  Input feature data used by this call.

## Output

- Returns `prep::TimeSeriesSplit`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Plan::last(/* frame */, /* time_feature */);
out.info("last", value);
```

**Expected output**

```text
last returned prep::TimeSeriesSplit
```

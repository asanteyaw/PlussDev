# `Plan::splits`

## API Signature

- `Plan::splits(frame: const pluss::table::DataFrame&, time_feature: std::string_view) -> std::vector<prep::TimeSeriesSplit>`

## Notes

Builds train/test index partitions according to split-plan options.

## Inputs

- `frame` (`const pluss::table::DataFrame&`)
  Input feature data used by this call.
- `time_feature` (`std::string_view`)
  Input feature data used by this call.

## Output

- Returns `std::vector<prep::TimeSeriesSplit>`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Plan::splits(/* frame */, /* time_feature */);
out.info("splits", value);
```

**Expected output**

```text
split completed
train/test partitions ready
```

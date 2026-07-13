# `encoded_param_table`

## API Signature

- `encoded_param_table(report: const TuneReport&) -> Series`

## Notes

Namespace-level API function.

## Inputs

- `report` (`const TuneReport&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Series`.

## Options

Option semantics used by this method:

- `TuneReport` (`struct`):
  - `std::vector<Trial> trials{}`
    Option field used by this method.
  - `std::int64_t best_index = -1`
    Option field used by this method.
  - `std::string best_name{}`
    Option field used by this method.
  - `MetricKind objective_metric = MetricKind::Accuracy`
    Selects optimization or evaluation objective.
  - `TuneObjectivePolicy objective_policy = TuneObjectivePolicy::Auto`
    Selects optimization or evaluation objective.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = encoded_param_table(/* report */);
out.info("encoded_param_table", value);
```

**Expected output**

```text
encoded_param_table returned Series
```

# `has_best_trial`

## API Signature

- `has_best_trial(report: const TuneReport&) -> bool`

## Notes

Boolean introspection helper that reports whether a capability/state is available.

## Inputs

- `report` (`const TuneReport&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `bool`.

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
auto value = has_best_trial(/* report */);
out.info("has_best_trial", value);
```

**Expected output**

```text
has_best_trial: true
```

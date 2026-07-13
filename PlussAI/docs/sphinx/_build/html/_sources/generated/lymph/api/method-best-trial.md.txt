# `best_trial`

## API Signature

- `best_trial(report: const TuneReport&) -> const TuneTrial&`

## Notes

Namespace-level API function.

## Inputs

- `report` (`const TuneReport&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `const TuneTrial&`.

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
auto value = best_trial(/* report */);
out.info("best_trial", value);
```

**Expected output**

```text
best_trial returned const TuneTrial&
```

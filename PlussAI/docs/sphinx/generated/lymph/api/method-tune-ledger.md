# `tune_ledger`

## API Signature

- `tune_ledger(report: const TuneReport&) -> const TuneLedger&`

## Notes

Runs parameter search and returns per-trial metrics plus best-trial selection.

## Inputs

- `report` (`const TuneReport&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `const TuneLedger&`.

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
auto value = tune_ledger(/* report */);
out.info("tune_ledger", value);
```

**Expected output**

```text
tune_ledger returned const TuneLedger&
```

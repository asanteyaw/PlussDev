# `CompareReport::board`

## API Signature

- `CompareReport::board() -> pluss::table::DataFrame`
- `CompareReport::board() -> pluss::table::DataFrame`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `pluss::table::DataFrame`.

## Options

Option semantics used by this method:

- `CompareReport` (`struct`):
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
  - `std::vector<std::int64_t> objective_rank{}`
    Selects optimization or evaluation objective.
  - `std::vector<std::int64_t> ranked_indices{}`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = CompareReport::board();
out.info("board", value);
```

**Expected output**

```text
board returned pluss::table::DataFrame
```

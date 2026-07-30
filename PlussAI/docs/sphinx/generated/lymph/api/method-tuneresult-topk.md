# `TuneResult::topk`

## API Signature

- `TuneResult::topk(k: std::int64_t) -> std::vector<std::reference_wrapper<const Workflow>>`
- `TuneResult::topk(k: std::int64_t) -> std::vector<std::reference_wrapper<Workflow>>`

## Notes

Instance method on `self`.

## Inputs

- `k` (`std::int64_t`)
  Algorithm-size/count argument used to shape fit or split behavior.

## Output

- Returns `std::vector<std::reference_wrapper<Workflow>>`.
- Returns `std::vector<std::reference_wrapper<const Workflow>>`.

## Options

Option semantics used by this method:

- `TuneResult` (`struct`):
  - `TuneReport report{}`
    Option field used by this method.
  - `Workflow best_workflow{}`
    Option field used by this method.
  - `std::vector<Workflow> fitted{}`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = TuneResult::topk(/* k */);
out.info("topk", value);
```

**Expected output**

```text
topk returned std::vector<std::reference_wrapper<const Workflow>>
```

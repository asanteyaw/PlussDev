# `tune_specs`

## API Signature

- `tune_specs(axes: const std::vector<GridAxis>&, arg2: const std::function<ModelSpec(const GridPoint&) -> SpecTuneResult`
- `tune_specs(axes: const std::vector<GridAxis>&, arg2: const std::function<ModelSpec(const GridPoint&) -> SpecTuneResult`
- `tune_specs(axes: const std::vector<GridAxis>&, arg2: const std::function<ModelSpec(const GridPoint&) -> SpecTuneResult`
- `tune_specs(axes: const std::vector<GridAxis>&, arg2: const std::function<ModelSpec(const GridPoint&) -> SpecTuneResult`

## Notes

Runs parameter search and returns per-trial metrics plus best-trial selection.

## Inputs

- `arg2` (`const std::function<ModelSpec(const GridPoint&`)
  Method argument; see signature type for accepted form.
- `axes` (`const std::vector<GridAxis>&`)
  Input feature data used by this call.

## Output

- Returns `SpecTuneResult`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = tune_specs(/* axes */, /* arg2 */);
out.info("tune_specs", value);
```

**Expected output**

```text
tune_specs returned SpecTuneResult
```

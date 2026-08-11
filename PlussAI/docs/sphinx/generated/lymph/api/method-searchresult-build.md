# `SearchResult::build`

## API Signature

- `SearchResult::build(axes: std::vector<GridAxis>, options: TuneOptions) -> Search`

## Notes

Instance method on `self`.

## Inputs

- `axes` (`std::vector<GridAxis>`)
  Input feature data used by this call.
- `options` (`TuneOptions`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `Search`.

## Options

Option semantics used by this method:

- `SearchResult` (`struct`):
  - `selection::CompareReport report{}`
    Option field used by this method.
  - `std::optional<Model> best_model{}`
    Option field used by this method.
  - `std::vector<Model> fitted{}`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = SearchResult::build(/* axes */, /* options */);
out.info("build", value);
```

**Expected output**

```text
build returned Search
```

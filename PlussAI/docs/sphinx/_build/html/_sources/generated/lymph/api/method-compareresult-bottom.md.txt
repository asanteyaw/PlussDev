# `CompareResult::bottom`

## API Signature

- `CompareResult::bottom(k: std::int64_t) -> std::vector<std::pair<std::string, Model>>`

## Notes

Instance method on `self`.

## Inputs

- `k` (`std::int64_t`)
  Algorithm-size/count argument used to shape fit or split behavior.

## Output

- Returns `std::vector<std::pair<std::string, Model>>`.

## Options

Option semantics used by this method:

- `CompareResult` (`struct`):
  - `CompareReport report{}`
    Option field used by this method.
  - `std::optional<Model> best_model{}`
    Option field used by this method.
  - `std::optional<Model> worst_model{}`
    Option field used by this method.
  - `std::vector<Model> fitted{}`
    Option field used by this method.
  - `std::vector<Model> bottom_fitted{}`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = CompareResult::bottom(/* k */);
out.info("bottom", value);
```

**Expected output**

```text
bottom returned std::vector<std::pair<std::string, Model>>
```

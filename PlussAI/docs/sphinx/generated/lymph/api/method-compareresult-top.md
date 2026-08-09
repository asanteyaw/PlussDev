# `CompareResult::top`

## API Signature

- `CompareResult::top(k: std::int64_t) -> std::vector<std::pair<std::string, Model>>`

## Notes

Instance method on `self`.

## Inputs

- `k` (`std::int64_t`)
  Algorithm-size/count argument used to shape fit or split behavior.

## Output

- Returns the top `k` fitted models as `(name, Model)` pairs.

## Options

Option semantics used by this method:

- `CompareResult` (`struct`):
  - `CompareReport report{}`
    Option field used by this method.
  - `std::optional<Model> best_model{}`
    Option field used by this method.
  - `std::vector<Model> fitted{}`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = CompareResult::top(/* k */);
out.info("top", value);
```

**Expected output**

```text
top returned std::vector<std::pair<std::string, Model>>
```

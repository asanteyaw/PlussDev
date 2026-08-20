# `CompareResult::worst`

## API Signature

- `CompareResult::worst() -> const Model&`
- `CompareResult::worst() -> Model&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `Model&`.
- Returns `const Model&`.

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
auto value = CompareResult::worst();
out.info("worst", value);
```

**Expected output**

```text
worst returned const Model&
```

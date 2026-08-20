# `TimeSeriesSplit::simple_imputer_strategy_name`

## API Signature

- `TimeSeriesSplit::simple_imputer_strategy_name(strategy: SimpleImputerStrategy) -> const char*`

## Notes

Applies preprocessing/transformation behavior configured by the current options.

## Inputs

- `strategy` (`SimpleImputerStrategy`)
  Target values aligned to input observations.

## Output

- Returns `const char*`.

## Options

Option semantics used by this method:

- `TimeSeriesSplit` (`struct`):
  - `pluss::table::DataFrame train_set{}`
    Controls fold/trial/count sizing.
  - `pluss::table::DataFrame test_set{}`
    Option field used by this method.
  - `LabeledBatch train_batch{}`
    Controls fold/trial/count sizing.
  - `LabeledBatch test_batch{}`
    Option field used by this method.
  - `IndexSplit indices{}`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = TimeSeriesSplit::simple_imputer_strategy_name(/* strategy */);
out.info("simple_imputer_strategy_name", value);
```

**Expected output**

```text
simple_imputer_strategy_name returned const char*
```

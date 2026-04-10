# `simple_imputer_strategy_name`

## API Signature

- `simple_imputer_strategy_name(strategy: SimpleImputerStrategy) -> const char*`

## Notes

Applies preprocessing/transformation behavior configured by the current options.

## Inputs

- `strategy` (`SimpleImputerStrategy`)
  Target values aligned to input observations.

## Output

- Returns `const char*`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = simple_imputer_strategy_name(/* strategy */);
out.info("simple_imputer_strategy_name", value);
```

**Expected output**

```text
simple_imputer_strategy_name returned const char*
```

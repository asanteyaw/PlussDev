# `columnrouter::with_remainder`

## API Signature

- `columnrouter::with_remainder(policy: RouteRemainderPolicy) -> ColumnRouter&`

## Notes

Instance method on `self`.

## Inputs

- `policy` (`RouteRemainderPolicy`)
  Target values aligned to input observations.

## Output

- Returns `ColumnRouter&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = columnrouter::with_remainder(/* policy */);
out.info("with_remainder", value);
```

**Expected output**

```text
with_remainder returned ColumnRouter&
```

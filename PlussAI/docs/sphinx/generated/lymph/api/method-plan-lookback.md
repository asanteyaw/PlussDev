# `Plan::lookback`

## API Signature

- `Plan::lookback(value: std::int64_t) -> Plan&`

## Notes

Instance method on `self`.

## Inputs

- `value` (`std::int64_t`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Plan&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Plan::lookback(/* value */);
out.info("lookback", value);
```

**Expected output**

```text
lookback returned Plan&
```

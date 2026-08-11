# `Plan::gap`

## API Signature

- `Plan::gap(value: std::int64_t) -> Plan&`

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
auto value = Plan::gap(/* value */);
out.info("gap", value);
```

**Expected output**

```text
gap returned Plan&
```

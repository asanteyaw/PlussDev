# `Plan::workers`

## API Signature

- `Plan::workers(value: std::int64_t) -> Plan&`

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
auto value = Plan::workers(/* value */);
out.info("workers", value);
```

**Expected output**

```text
workers returned Plan&
```

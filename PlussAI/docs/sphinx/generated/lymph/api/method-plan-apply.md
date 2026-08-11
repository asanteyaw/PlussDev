# `Plan::apply`

## API Signature

- `Plan::apply(model: Model&) -> Model&`

## Notes

Instance method on `self`.

## Inputs

- `model` (`Model&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Model&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Plan::apply(/* model */);
out.info("apply", value);
```

**Expected output**

```text
apply returned Model&
```

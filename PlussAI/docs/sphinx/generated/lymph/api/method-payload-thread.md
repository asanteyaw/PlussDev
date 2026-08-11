# `Payload::thread`

## API Signature

- `Payload::thread(policy: DataThreadPolicy) -> Payload&`

## Notes

Instance method on `self`.

## Inputs

- `policy` (`DataThreadPolicy`)
  Target values aligned to input observations.

## Output

- Returns `Payload&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Payload::thread(/* policy */);
out.info("thread", value);
```

**Expected output**

```text
thread returned Payload&
```

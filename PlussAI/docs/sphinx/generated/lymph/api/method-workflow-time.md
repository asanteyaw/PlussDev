# `workflow::time`

## API Signature

- `workflow::time(options: TimeOptions) -> Workflow&`

## Notes

Instance method on `self`.

## Inputs

- `options` (`TimeOptions`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `Workflow&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = workflow::time(/* options */);
out.info("time", value);
```

**Expected output**

```text
time returned Workflow&
```

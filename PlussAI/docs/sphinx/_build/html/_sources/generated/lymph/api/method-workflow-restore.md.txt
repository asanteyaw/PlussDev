# `Workflow::restore`

## API Signature

- `Workflow::restore(bundle: const io::FlowBundle&) -> Workflow`

## Notes

Instance method on `self`.

## Inputs

- `bundle` (`const io::FlowBundle&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Workflow`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Workflow::restore(/* bundle */);
out.info("restore", value);
```

**Expected output**

```text
restore returned Workflow
```

# `workflow::join`

## API Signature

- `workflow::join() -> Workflow&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `Workflow&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = workflow::join();
out.info("join", value);
```

**Expected output**

```text
join returned Workflow&
```

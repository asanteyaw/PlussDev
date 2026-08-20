# `workflow::task_kind`

## API Signature

- `workflow::task_kind() -> TaskKind`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `TaskKind`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = workflow::task_kind();
out.info("task_kind", value);
```

**Expected output**

```text
task_kind returned TaskKind
```

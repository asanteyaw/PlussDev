# `ModelCore::append`

## API Signature

- `ModelCore::append(stage: Stage, router: ColumnRouter) -> ModelCore&`

## Notes

Instance method on `self`.

## Inputs

- `router` (`ColumnRouter`)
  Method argument; see signature type for accepted form.
- `stage` (`Stage`)
  Method argument; see signature type for accepted form.

## Output

- Returns `ModelCore&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = ModelCore::append(/* stage */, /* router */);
out.info("append", value);
```

**Expected output**

```text
append returned ModelCore&
```

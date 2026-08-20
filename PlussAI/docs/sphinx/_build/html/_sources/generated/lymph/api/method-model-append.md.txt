# `Model::append`

## API Signature

- `Model::append(stage: Stage, router: ColumnRouter) -> Model&`

## Notes

Instance method on `self`.

## Inputs

- `router` (`ColumnRouter`)
  Method argument; see signature type for accepted form.
- `stage` (`Stage`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Model&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Model::append(/* stage */, /* router */);
out.info("append", value);
```

**Expected output**

```text
append returned Model&
```

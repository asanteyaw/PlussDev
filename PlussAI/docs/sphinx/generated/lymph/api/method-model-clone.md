# `Model::clone`

## API Signature

- `Model::clone() -> Model`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `Model`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Model::clone();
out.info("clone", value);
```

**Expected output**

```text
clone returned Model
```

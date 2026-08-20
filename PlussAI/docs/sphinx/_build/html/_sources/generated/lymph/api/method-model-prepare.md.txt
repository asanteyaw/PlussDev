# `Model::prepare`

## API Signature

- `Model::prepare() -> ModelPrep`
- `Model::prepare() -> ModelPrep`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `ModelPrep`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Model::prepare();
out.info("prepare", value);
```

**Expected output**

```text
prepare returned ModelPrep
```

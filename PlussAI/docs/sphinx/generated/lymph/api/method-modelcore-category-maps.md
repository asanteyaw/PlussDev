# `ModelCore::category_maps`

## API Signature

- `ModelCore::category_maps() -> const CategoryMaps&`

## Notes

Applies preprocessing/transformation behavior configured by the current options.

## Inputs

None.

## Output

- Returns `const CategoryMaps&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = ModelCore::category_maps();
out.info("category_maps", value);
```

**Expected output**

```text
category_maps returned const CategoryMaps&
```

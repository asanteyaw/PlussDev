# `LexiconMapper::vocabulary`

## API Signature

- `LexiconMapper::vocabulary() -> const std::vector<std::string>&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `const std::vector<std::string>&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = LexiconMapper::vocabulary();
out.info("vocabulary", value);
```

**Expected output**

```text
vocabulary returned const std::vector<std::string>&
```

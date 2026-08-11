# `Payload::groups`

## API Signature

- `Payload::groups() -> const Series&`
- `Payload::groups(groups: Series) -> Payload&`

## Notes

Instance method on `self`.

## Inputs

- `groups` (`Series`)
  Pluss numeric vector/tensor input.

## Output

- Returns `Payload&`.
- Returns `const Series&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Payload::groups();
out.info("groups", value);
```

**Expected output**

```text
groups returned const Series&
```

# `build`

## API Signature

- `build() -> Builder`

## Notes

Namespace-level API function.

## Inputs

None.

## Output

- Returns `Builder`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = build();
out.info("build", value);
```

**Expected output**

```text
build returned Builder
```

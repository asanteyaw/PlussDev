# `equalize_blend_neighbors`

## API Signature

- `equalize_blend_neighbors(neighbors: std::int64_t = 5) -> BalanceTarget`

## Notes

Namespace-level API function.

## Inputs

- `neighbors` (`std::int64_t`), default: `5`
  Algorithm-size/count argument used to shape fit or split behavior.

## Output

- Returns `BalanceTarget`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = equalize_blend_neighbors(/* neighbors */);
out.info("equalize_blend_neighbors", value);
```

**Expected output**

```text
equalize_blend_neighbors returned BalanceTarget
```

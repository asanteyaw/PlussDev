# `equalize_reuse_rows`

## API Signature

- `equalize_reuse_rows(neighbors: std::int64_t = 5) -> BalanceTarget`

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
auto value = equalize_reuse_rows(/* neighbors */);
out.info("equalize_reuse_rows", value);
```

**Expected output**

```text
equalize_reuse_rows returned BalanceTarget
```

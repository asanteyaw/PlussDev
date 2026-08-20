# `SpatialIndex::query_k`

## API Signature

- `SpatialIndex::query_k(query_rows: const Series&, k: std::int64_t) -> StatusOr<KNearestResult>`

## Notes

Instance method on `self`.

## Inputs

- `k` (`std::int64_t`)
  Algorithm-size/count argument used to shape fit or split behavior.
- `query_rows` (`const Series&`)
  Target values aligned to input observations.

## Output

- Returns `StatusOr<KNearestResult>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = SpatialIndex::query_k(/* query_rows */, /* k */);
if (!res.ok()) {
  out.error("query_k failed", res.status().message);
} else {
  out.info("query_k ok");
}
```

**Expected output**

```text
query_k: ok
status.ok() == true
result.value() is available
```

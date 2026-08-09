# `SpatialIndex::build`

## API Signature

- `SpatialIndex::build(rows: const Series&) -> StatusOr<SpatialIndex>`

## Notes

Instance method on `self`.

## Inputs

- `rows` (`const Series&`)
  Pluss numeric vector/tensor input.

## Output

- Returns `StatusOr<SpatialIndex>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = SpatialIndex::build(/* rows */);
if (!res.ok()) {
  out.error("build failed", res.status().message);
} else {
  out.info("build ok");
}
```

**Expected output**

```text
build: ok
status.ok() == true
result.value() is available
```

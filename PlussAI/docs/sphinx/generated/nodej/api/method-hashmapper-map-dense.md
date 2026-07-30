# `HashMapper::map_dense`

## API Signature

- `HashMapper::map_dense(documents: std::initializer_list<std::string>) -> StatusOr<FeatureBatch>`
- `HashMapper::map_dense(documents: RangeView::Of<std::string>) -> StatusOr<FeatureBatch>`

## Notes

Applies preprocessing/transformation behavior configured by the current options.

## Inputs

- `documents` (`RangeView::Of<std::string> | std::initializer_list<std::string>`)
  List-like argument; pass values in the order expected by the signature.

## Output

- Returns `StatusOr<FeatureBatch>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = HashMapper::map_dense(/* documents */);
if (!res.ok()) {
  out.error("map_dense failed", res.status().message);
} else {
  out.info("map_dense ok");
}
```

**Expected output**

```text
map_dense: ok
status.ok() == true
result.value() is available
```

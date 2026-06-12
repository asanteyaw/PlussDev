# `LexiconMapper::map_sparse`

## API Signature

- `LexiconMapper::map_sparse(documents: std::initializer_list<std::string>) -> StatusOr<SparseTripletMatrix>`
- `LexiconMapper::map_sparse(documents: RangeView::Of<std::string>) -> StatusOr<SparseTripletMatrix>`

## Notes

Applies preprocessing/transformation behavior configured by the current options.

## Inputs

- `documents` (`RangeView::Of<std::string> | std::initializer_list<std::string>`)
  List-like argument; pass values in the order expected by the signature.

## Output

- Returns `StatusOr<SparseTripletMatrix>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = LexiconMapper::map_sparse(/* documents */);
if (!res.ok()) {
  out.error("map_sparse failed", res.status().message);
} else {
  out.info("map_sparse ok");
}
```

**Expected output**

```text
map_sparse: ok
status.ok() == true
result.value() is available
```

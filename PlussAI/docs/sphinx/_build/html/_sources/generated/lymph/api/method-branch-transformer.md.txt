# `branch_transformer`

## API Signature

- `branch_transformer(frame: const pluss::table::DataFrame&, data_options: const DataFrameBatchOptions&, branches: RangeView::Of<workflow::feature::Branch>, remainder: prep::FeatureRemainderPolicy = prep::FeatureRemainderPolicy::PassThrough) -> ::pluss::ai::nodej::prep::FeatureTransformer`
- `branch_transformer(frame: const pluss::table::DataFrame&, data_options: const DataFrameBatchOptions&, branches: std::initializer_list<workflow::feature::Branch>, remainder: prep::FeatureRemainderPolicy = prep::FeatureRemainderPolicy::PassThrough) -> ::pluss::ai::nodej::prep::FeatureTransformer`
- `branch_transformer(frame: const pluss::table::DataFrame&, branches: RangeView::Of<workflow::feature::Branch>, remainder: prep::FeatureRemainderPolicy = prep::FeatureRemainderPolicy::PassThrough) -> ::pluss::ai::nodej::prep::FeatureTransformer`
- `branch_transformer(frame: const pluss::table::DataFrame&, branches: std::initializer_list<workflow::feature::Branch>, remainder: prep::FeatureRemainderPolicy = prep::FeatureRemainderPolicy::PassThrough) -> ::pluss::ai::nodej::prep::FeatureTransformer`
- `branch_transformer(feature_order: RangeView::Of<std::string>, branches: RangeView::Of<workflow::feature::Branch>, remainder: prep::FeatureRemainderPolicy = prep::FeatureRemainderPolicy::PassThrough) -> ::pluss::ai::nodej::prep::FeatureTransformer`
- `branch_transformer(feature_order: std::initializer_list<std::string>, branches: std::initializer_list<workflow::feature::Branch>, remainder: prep::FeatureRemainderPolicy = prep::FeatureRemainderPolicy::PassThrough) -> ::pluss::ai::nodej::prep::FeatureTransformer`

## Notes

Namespace-level API function.

## Inputs

- `branches` (`RangeView::Of<workflow::feature::Branch> | std::initializer_list<workflow::feature::Branch>`)
  List-like argument; pass values in the order expected by the signature.
- `data_options` (`const DataFrameBatchOptions&`)
  Input feature data used by this call.
- `feature_order` (`RangeView::Of<std::string> | std::initializer_list<std::string>`)
  Input feature data used by this call.
- `frame` (`const pluss::table::DataFrame&`)
  Input feature data used by this call.
- `remainder` (`prep::FeatureRemainderPolicy`), default: `prep::FeatureRemainderPolicy::PassThrough`
  Method argument; see signature type for accepted form.

## Output

- Returns `::pluss::ai::nodej::prep::FeatureTransformer`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = branch_transformer(/* frame */, /* data_options */, /* branches */, /* remainder */);
out.info("branch_transformer", value);
```

**Expected output**

```text
branch_transformer returned ::pluss::ai::nodej::prep::FeatureTransformer
```

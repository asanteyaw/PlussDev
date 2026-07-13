# `make_feature_transformer`

## API Signature

- `make_feature_transformer(frame: const pluss::table::DataFrame&, data_options: const DataFrameBatchOptions&, branches: const std::vector<workflow::feature::Branch>&, remainder: prep::FeatureRemainderPolicy = prep::FeatureRemainderPolicy::PassThrough) -> ::pluss::ai::nodej::prep::FeatureTransformer`
- `make_feature_transformer(frame: const pluss::table::DataFrame&, data_options: const DataFrameBatchOptions&, branches: RangeView::Of<workflow::feature::Branch>, remainder: prep::FeatureRemainderPolicy = prep::FeatureRemainderPolicy::PassThrough) -> ::pluss::ai::nodej::prep::FeatureTransformer`
- `make_feature_transformer(frame: const pluss::table::DataFrame&, data_options: const DataFrameBatchOptions&, branches: std::initializer_list<workflow::feature::Branch>, remainder: prep::FeatureRemainderPolicy = prep::FeatureRemainderPolicy::PassThrough) -> ::pluss::ai::nodej::prep::FeatureTransformer`
- `make_feature_transformer(frame: const pluss::table::DataFrame&, branches: const std::vector<workflow::feature::Branch>&, remainder: prep::FeatureRemainderPolicy = prep::FeatureRemainderPolicy::PassThrough) -> ::pluss::ai::nodej::prep::FeatureTransformer`
- `make_feature_transformer(frame: const pluss::table::DataFrame&, branches: RangeView::Of<workflow::feature::Branch>, remainder: prep::FeatureRemainderPolicy = prep::FeatureRemainderPolicy::PassThrough) -> ::pluss::ai::nodej::prep::FeatureTransformer`
- `make_feature_transformer(frame: const pluss::table::DataFrame&, branches: std::initializer_list<workflow::feature::Branch>, remainder: prep::FeatureRemainderPolicy = prep::FeatureRemainderPolicy::PassThrough) -> ::pluss::ai::nodej::prep::FeatureTransformer`
- `make_feature_transformer(feature_order: const std::vector<std::string>&, branches: const std::vector<workflow::feature::Branch>&, remainder: prep::FeatureRemainderPolicy = prep::FeatureRemainderPolicy::PassThrough) -> ::pluss::ai::nodej::prep::FeatureTransformer`
- `make_feature_transformer(feature_order: RangeView::Of<std::string>, branches: RangeView::Of<workflow::feature::Branch>, remainder: prep::FeatureRemainderPolicy = prep::FeatureRemainderPolicy::PassThrough) -> ::pluss::ai::nodej::prep::FeatureTransformer`
- `make_feature_transformer(feature_order: std::initializer_list<std::string>, branches: std::initializer_list<workflow::feature::Branch>, remainder: prep::FeatureRemainderPolicy = prep::FeatureRemainderPolicy::PassThrough) -> ::pluss::ai::nodej::prep::FeatureTransformer`

## Notes

Namespace-level API function.

## Inputs

- `branches` (`RangeView::Of<workflow::feature::Branch> | const std::vector<workflow::feature::Branch>& | std::initializer_list<workflow::feature::Branch>`)
  List-like argument; pass values in the order expected by the signature.
- `data_options` (`const DataFrameBatchOptions&`)
  Input feature data used by this call.
- `feature_order` (`RangeView::Of<std::string> | const std::vector<std::string>& | std::initializer_list<std::string>`)
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
auto value = make_feature_transformer(/* frame */, /* data_options */, /* branches */, /* remainder */);
out.info("make_feature_transformer", value);
```

**Expected output**

```text
make_feature_transformer returned ::pluss::ai::nodej::prep::FeatureTransformer
```

### Example 2 (Demo Pattern)

```cpp
  const auto categorical = housing.note_features().features();

  pluss::ai::nodej::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"median_income", "ocean_proximity", "target"};
  data_options.label_feature = "target";
  data_options.note_policy = pluss::ai::nodej::DataNotePolicy::Hash;
  data_options.layout = pluss::ai::nodej::ObservationLayout::Rows;

  auto preprocessing = lymph::make_feature_transformer(
      housing,
      data_options,
      std::vector<lymph::workflow::feature::Branch>{
          lymph::workflow::feature::compose(
              {
                  lymph::workflow::stage::simple_imputer(lymph::prep::SimpleImputerOptions{
                      .strategy = lymph::prep::SimpleImputerStrategy::Mean,
                  }),
                  lymph::workflow::stage::standard_scaler(),
              },
              {"median_income"}),
          lymph::workflow::feature::one_hot_encoder(lymph::prep::OneHotEncoderOptions{}, categorical),
```

**Expected output**

```text
numeric features: median_income target
categorical features: ocean_proximity
ordinal mapped names: ocean_proximity_ordinal
label mapped names: ocean_proximity_label
ordinal sample: ======================
     | ocean_proximity
----------------------
0    | 2
1    | 1
2    | 2
3    | 1
4    | 2
...
```

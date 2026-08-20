# `Payload::note`

## API Signature

- `Payload::note(policy: DataNotePolicy) -> Payload&`

## Notes

Instance method on `self`.

## Inputs

- `policy` (`DataNotePolicy`)
  Target values aligned to input observations.

## Output

- Returns `Payload&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Payload::note(/* policy */);
out.info("note", value);
```

**Expected output**

```text
note returned Payload&
```

### Example 2 (Demo Pattern)

```cpp
  const pluss::table::DataFrame housing({
      {"median_income", pluss::table::Thread(x)},
      {"ocean_proximity", cat},
      {"target", pluss::table::Thread(y)},
  });

  const auto numeric = housing.feataxis().series().features();
  const auto categorical = housing.feataxis().note().features();

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

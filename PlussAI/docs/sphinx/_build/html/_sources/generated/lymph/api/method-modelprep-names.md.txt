# `ModelPrep::names`

## API Signature

- `ModelPrep::names(values: std::initializer_list<std::string>) -> ColumnRouter`
- `ModelPrep::names(values: std::vector<std::string>) -> ColumnRouter`

## Notes

Instance method on `self`.

## Inputs

- `values` (`std::initializer_list<std::string> | std::vector<std::string>`)
  List-like argument; pass values in the order expected by the signature.

## Output

- Returns `ColumnRouter`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = ModelPrep::names(/* values */);
out.info("names", value);
```

**Expected output**

```text
names returned ColumnRouter
```

### Example 2 (Demo Pattern)

```cpp
  std::cout << "categorical features: ";
  for (const auto& n : categorical) std::cout << n << " ";
  std::cout << "\n";
  auto ord = lymph::prep::OrdinalEncoder({.include_unknown_bucket = true, .one_based = true});
  ord.features({"ocean_proximity"});
  const auto ocean_ord = ord.learn_map(housing.features({"ocean_proximity"}));
  auto lbl = lymph::prep::LabelEncoder({.include_unknown_bucket = true, .one_based = false});
  const auto ocean_lbl = lbl.learn_map(housing.features("ocean_proximity"));
  std::cout << "ordinal mapped names: ";
  for (const auto& name : ord.mapped_names_) std::cout << name << " ";
  std::cout << "\n";
  std::cout << "label mapped names: ";
  for (const auto& name : lbl.mapped_names_) std::cout << name << " ";
  std::cout << "\n";
  std::cout << "ordinal sample: " << ocean_ord << "\n";
  std::cout << "label sample: " << ocean_lbl << "\n";
  std::cout << "pred: " << pred << "\n";
  std::cout << "mse: " << metrics.mse << " r2: " << metrics.r2 << "\n";
  return 0;
}
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

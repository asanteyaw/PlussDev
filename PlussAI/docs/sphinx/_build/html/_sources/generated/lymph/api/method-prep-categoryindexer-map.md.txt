# `prep::CategoryIndexer::map`

## API Signature

- `prep::CategoryIndexer::map(frame: const pluss::table::DataFrame&, options: const InferOptions&) -> pluss::table::DataFrame`
- `prep::CategoryIndexer::map(feature: const pluss::table::Thread&, options: const InferOptions&) -> Series`

## Notes

Applies preprocessing/transformation behavior configured by the current options.

## Inputs

- `feature` (`const pluss::table::Thread&`)
  Input feature data used by this call.
- `frame` (`const pluss::table::DataFrame&`)
  Input feature data used by this call.
- `options` (`const InferOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `Series`.
- Returns `pluss::table::DataFrame`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = prep::CategoryIndexer::map(/* frame */, /* options */);
out.info("map", value);
```

**Expected output**

```text
map returned pluss::table::DataFrame
```

### Example 2 (Demo Pattern)

```cpp
  if (!require_ok(poly_or, "PolynomialLift.learn_map")) return 1;
  print_shape("polynomial-lift", poly_or.value());

  const Series x_query = pluss::make_series(std::vector<double>{
                                                7.0, 5.0, 1.0,
                                                8.0, 5.5, 0.0},
                                            {2, 3},
                                            f64());
  auto mapped_or = zscore.map(FeatureBatch{.x = x_query, .layout = ObservationLayout::Rows});
  if (!require_ok(mapped_or, "StandardScaler.map")) return 1;
  print_shape("standardized query", mapped_or.value());

  const Series categories = pluss::make_series(std::vector<double>{
                                                   0.0, 1.0,
                                                   1.0, 1.0,
                                                   2.0, 0.0,
                                                   1.0, 2.0,
                                                   0.0, 2.0,
                                                   2.0, 1.0},
                                               {6, 2},
                                               f64());
```

**Expected output**

```text
NodeJ split + prep demo
holdout train idx count=4 test idx count=2
strat holdout train idx count=4 test idx count=2
kfold folds=3
imputed shape=[6, 3]
standardized shape=[6, 3]
minmax shape=[6, 3]
robust shape=[6, 3]
neighbor-imputed shape=[6, 3]
iterative-imputed shape=[6, 3]
vector-normalized shape=[6, 3]
maxabs shape=[6, 3]
...
```

### Example 3 (Demo Pattern)

```cpp
    std::cerr << "make_transformer(kmeans) failed: " << kmeans_or.status().message << "\n";
    return 1;
  }
  auto kmeans = kmeans_or.move_value();
  if (!kmeans->learn(features).ok()) {
    std::cerr << "kmeans learn failed\n";
    return 1;
  }
  auto labels_or = kmeans->map(features);
  if (!labels_or.ok()) {
    std::cerr << "kmeans map failed: " << labels_or.status().message << "\n";
    return 1;
  }
  const auto labels = labels_or.value().reshape({60}).to(pluss::pCPU, pluss::ScalarType::Float64).to_vec<double>();
  std::cout << "kmeans labels (first 10): ";
  for (int i = 0; i < 10; ++i) std::cout << labels[static_cast<std::size_t>(i)] << " ";
  std::cout << "\n";

  std::vector<double> pca_vals{};
  for (int i = 0; i < 80; ++i) {
    const double x0 = -2.0 + 4.0 * static_cast<double>(i) / 79.0;
```

**Expected output**

```text
kmeans labels (first 10): 0 0 0 0 0 0 0 0 0 0
pca output shape: [80, 2]
nodej_unsupervised_demo complete
```

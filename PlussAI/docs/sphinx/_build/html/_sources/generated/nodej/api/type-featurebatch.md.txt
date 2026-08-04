# `FeatureBatch`

## API Signature

- `FeatureBatch`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `FeatureBatch` object used by NodeJ/Lymph workflows.

## Notes

Container for model input features and their observation layout. Use `layout` to declare whether rows or columns represent observations.

## Fields

Public fields on this type:

- `Series x = Series::undefined()`
- `ObservationLayout layout = ObservationLayout::Rows`

## Examples

### Example 1

```cpp
                                              1.0,  2.0, std::numeric_limits<double>::quiet_NaN(),
                                              2.0,  2.5, 1.0,
                                              3.0,  std::numeric_limits<double>::quiet_NaN(), 0.0,
                                              4.0,  3.5, 1.0,
                                              5.0,  4.0, 0.0,
                                              6.0,  4.5, 1.0},
                                          {6, 3},
                                          f64());
  const FeatureBatch dense{.x = x_raw, .layout = ObservationLayout::Rows};
  const Series labels = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64());

  SplitOptions split{};
  split.holdout_fraction = 0.33;
  split.random.seed = 7;

  auto holdout_or = pluss::ai::nodej::holdout_split(dense.sample_count(), split);
  if (!require_ok(holdout_or, "holdout_split")) return 1;
  std::cout << "holdout train idx count=" << holdout_or.value().train_indices.numel()
            << " test idx count=" << holdout_or.value().test_indices.numel() << "\n";

  auto strat_or = pluss::ai::nodej::strat_holdout_split(labels, split);
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

### Example 2

```cpp
  for (int i = 0; i < 30; ++i) {
    xvals.push_back(-3.0 + 0.1 * i);
    xvals.push_back(-2.5 + 0.05 * i);
  }
  for (int i = 0; i < 30; ++i) {
    xvals.push_back(2.0 + 0.08 * i);
    xvals.push_back(2.5 + 0.06 * i);
  }
  const FeatureBatch features{
      .x = pluss::make_series(xvals, {60, 2}, cfg),
      .layout = ObservationLayout::Rows,
  };

  ModelSpec kmeans_spec{
      .task = TaskKind::Transform,
      .family = ModelFamily::Cluster,
      .options = ClusterFamilyOptions{
          .method = ClusterMethod::KMeans,
          .clusters = 2,
          .max_iterations = 200,
          .restarts = 5,
```

**Expected output**

```text
kmeans labels (first 10): 0 0 0 0 0 0 0 0 0 0
pca output shape: [80, 2]
nodej_unsupervised_demo complete
```

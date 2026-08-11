# `Result::method`

## API Signature

- `Result::method() -> const std::string&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `const std::string&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Result::method();
out.info("method", value);
```

**Expected output**

```text
method returned const std::string&
```

### Example 2 (Demo Pattern)

```cpp
                                    .family = ModelFamily::Tree,
                                    .options = TreeFamilyOptions{},
                                },
                        cls_train);
  run_classifier_family("ensemble", ModelSpec{
                                        .task = TaskKind::Classification,
                                        .family = ModelFamily::Ensemble,
                                        .options = EnsembleFamilyOptions{
                                            .method = EnsembleMethod::RandomForest,
                                            .learners = 21,
                                            .subsample = 0.9,
                                        },
                                    },
                        cls_train);
  run_classifier_family("neighbor", ModelSpec{
                                        .task = TaskKind::Classification,
                                        .family = ModelFamily::Neighbor,
                                        .options = NeighborFamilyOptions{
                                            .k = 3,
                                            .distance_weighted = true,
                                            .standardize = true,
```

**Expected output**

```text
NodeJ model families demo
Classification families:
  linear (acc, logloss): 1 4.47877e-06
  linear_hinge (acc, logloss): 1 0.208572
  tree (acc, logloss): 1 9.99978e-13
  ensemble (acc, logloss): 1 1.00003e-12
  neighbor (acc, logloss): 1 9.99978e-13
  kernel (acc, logloss): 1 0.318438
  bayes (acc, logloss): 1 9.99978e-13
Regression families:
  linear (mse, r2): 1.52381e-19 1
  tree (mse, r2): 0 1
...
```

### Example 3 (Demo Pattern)

```cpp
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
          .tolerance = 1e-5,
      },
  };
  auto kmeans_or = make_transformer(kmeans_spec);
  if (!kmeans_or.ok()) {
    std::cerr << "make_transformer(kmeans) failed: " << kmeans_or.status().message << "\n";
    return 1;
  }
  auto kmeans = kmeans_or.move_value();
```

**Expected output**

```text
kmeans labels (first 10): 0 0 0 0 0 0 0 0 0 0
pca output shape: [80, 2]
nodej_unsupervised_demo complete
```

# `Result::components`

## API Signature

- `Result::components() -> Series`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `Series`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Result::components();
out.info("components", value);
```

**Expected output**

```text
components returned Series
```

### Example 2 (Demo Pattern)

```cpp
      .layout = ObservationLayout::Rows,
  };

  ModelSpec pca_spec{
      .task = TaskKind::Transform,
      .family = ModelFamily::Decompose,
      .options = DecomposeFamilyOptions{
          .method = DecomposeMethod::Pca,
          .components = 2,
          .whiten = false,
          .max_iterations = 400,
          .tolerance = 1e-6,
      },
  };
  auto pca_or = make_transformer(pca_spec);
  if (!pca_or.ok()) {
    std::cerr << "make_transformer(pca) failed: " << pca_or.status().message << "\n";
    return 1;
  }
  auto pca = pca_or.move_value();
  if (!pca->learn(pca_features).ok()) {
```

**Expected output**

```text
kmeans labels (first 10): 0 0 0 0 0 0 0 0 0 0
pca output shape: [80, 2]
nodej_unsupervised_demo complete
```

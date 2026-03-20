# `make_transformer`

## API Signature

- `make_transformer(spec: const ModelSpec&) -> StatusOr<std::unique_ptr<TransformerBase>>`

## Notes

Factory entry point that validates spec/options and returns a ready-to-train component.

## Inputs

- `spec` (`const ModelSpec&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `StatusOr<std::unique_ptr<TransformerBase>>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

Option semantics used by this method:

- `ModelSpec` (`struct`):
  - `TaskKind task = TaskKind::Regression`
    Selects task type (for example classification vs regression).
  - `ModelFamily family = ModelFamily::Linear`
    Selects the model family implementation.
  - `FamilyOptions options = LinearFamilyOptions{}`
    Family-specific options payload used by the selected model/transform family.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = make_transformer(/* spec */);
if (!res.ok()) {
  out.error("make_transformer failed", res.status().message);
} else {
  out.info("make_transformer ok");
}
```

**Expected output**

```text
make_transformer: ok
status.ok() == true
result.value() is available
```

### Example 2 (Demo Pattern)

```cpp
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
  if (!kmeans->learn(features).ok()) {
    std::cerr << "kmeans learn failed\n";
    return 1;
  }
  auto labels_or = kmeans->map(features);
  if (!labels_or.ok()) {
    std::cerr << "kmeans map failed: " << labels_or.status().message << "\n";
```

**Expected output**

```text
kmeans labels (first 10): 0 0 0 0 0 0 0 0 0 0
pca output shape: [80, 2]
nodej_unsupervised_demo complete
```

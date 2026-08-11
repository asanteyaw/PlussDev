# `Result::labels`

## API Signature

- `Result::labels() -> const Series&`
- `Result::labels() -> const Series&`
- `Result::labels() -> const Series&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `const Series&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Result::labels();
out.info("labels", value);
```

**Expected output**

```text
labels returned const Series&
```

### Example 2 (Demo Pattern)

```cpp
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
    const double x1 = 0.8 * x0 + 0.2 * std::sin(3.0 * x0);
    const double x2 = -0.5 * x0 + 0.7 * x1;
    pca_vals.push_back(x0);
    pca_vals.push_back(x1);
    pca_vals.push_back(x2);
  }
```

**Expected output**

```text
kmeans labels (first 10): 0 0 0 0 0 0 0 0 0 0
pca output shape: [80, 2]
nodej_unsupervised_demo complete
```

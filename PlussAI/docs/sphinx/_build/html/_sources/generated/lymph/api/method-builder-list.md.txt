# `Builder::list`

## API Signature

- `Builder::list(n: std::int64_t = -1) -> std::vector<std::string>`

## Notes

Instance method on `self`.

## Inputs

- `n` (`std::int64_t`), default: `-1`
  Method argument; see signature type for accepted form.

## Output

- Returns `std::vector<std::string>`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Builder::list(/* n */);
out.info("list", value);
```

**Expected output**

```text
list returned std::vector<std::string>
```

### Example 2 (Demo Pattern)

```cpp
  const ModelSpec top3_spec = broad_sweep.trials[top3_idx].spec;

  std::cout << "\n========== top three candidates ========\n";
  std::cout << "top1_index=" << top1_idx << " family=" << static_cast<int>(top1_spec.family) << "\n";
  std::cout << "top2_index=" << top2_idx << " family=" << static_cast<int>(top2_spec.family) << "\n";
  std::cout << "top3_index=" << top3_idx << " family=" << static_cast<int>(top3_spec.family) << "\n";

  //========== tuning: focused search around top-three families ========
  // This section builds a denser candidate list around the winning families.
  std::vector<ModelSpec> tuned_specs{};
  std::vector<ModelFamily> top_families = {top1_spec.family, top2_spec.family, top3_spec.family};

  bool include_linear = false;
  bool include_tree = false;
  bool include_ensemble = false;
  bool include_neighbor = false;
  bool include_kernel = false;
  bool include_bayes = false;

  for (const auto fam : top_families) {
    if (fam == ModelFamily::Linear) include_linear = true;
```

**Expected output**

```text
list returned std::vector<std::string>
```

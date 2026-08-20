# `Builder::empty`

## API Signature

- `Builder::empty() -> bool`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `bool`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Builder::empty();
out.info("empty", value);
```

**Expected output**

```text
empty: true
```

### Example 2 (Demo Pattern)

```cpp
  TuneReport broad_sweep = broad_sweep_or.move_value();

  std::vector<std::size_t> broad_success_indices{};
  for (std::size_t i = 0; i < broad_sweep.trials.size(); ++i) {
    if (!broad_sweep.trials[i].status.ok()) continue;
    if (!std::isfinite(broad_sweep.trials[i].objective)) continue;
    broad_success_indices.push_back(i);
  }
  if (broad_success_indices.empty()) {
    std::cerr << "broad sweep produced no successful trials.\n";
    return 1;
  }

  std::sort(
      broad_success_indices.begin(),
      broad_success_indices.end(),
      [&](std::size_t a, std::size_t b) {
        return broad_sweep.trials[a].objective < broad_sweep.trials[b].objective;
      });

  std::cout << "\n========== broad cv ranking (rmse, lower is better) ========\n";
```

**Expected output**

```text
empty: true
```

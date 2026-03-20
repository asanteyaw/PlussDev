# `OnlineLinearRegressor::seen_samples`

## API Signature

- `OnlineLinearRegressor::seen_samples() -> std::int64_t`

## Notes

Reports a cardinality/count statistic from the current object state.

## Inputs

None.

## Output

- Returns `std::int64_t`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = OnlineLinearRegressor::seen_samples();
out.info("seen_samples", value);
```

**Expected output**

```text
seen_samples: <numeric_result>
```

### Example 2 (Demo Pattern)

```cpp
  pluss::ai::nodej::AssessOptions assess{};
  assess.metrics = {MetricKind::Accuracy, MetricKind::LogLoss};
  auto metric_or = model.assess(full, assess);
  if (!metric_or.ok()) {
    std::cerr << "assess failed: " << metric_or.status().message << '\n';
    return 1;
  }

  std::cout << "online seen_samples=" << model.seen_samples() << '\n';
  std::cout << "online metrics [accuracy, logloss]: ";
  print_vec(metric_or.value());
  return 0;
}
```

**Expected output**

```text
online seen_samples=1872
online metrics [accuracy, logloss]: 1 0.27341
```

### Example 3 (Demo Pattern)

```cpp
    warm_opts.max_iterations = 80;

    auto model = pluss::ai::lymph::online_factory::linear_classifier(online_options);
    model.learn_(warm, warm_opts).advance_(stream);

    const auto metrics = model.assess(full);
    const Series metric_values = metrics.accuracy.defined() ? metrics.accuracy : metrics.values;

    std::cout << "lymph online seen_samples=" << model.seen_samples() << '\n';
    std::cout << "lymph online metrics [accuracy]: ";
    print_vec(metric_values);
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "lymph_online_demo failed: " << ex.what() << '\n';
    return 1;
  }
}
```

**Expected output**

```text
lymph online seen_samples=1660
lymph online metrics [accuracy]: 0.9875
```

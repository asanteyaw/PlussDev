# `GridPoint::size`

## API Signature

- `GridPoint::size() -> std::size_t`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `std::size_t`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = GridPoint::size();
out.info("size", value);
```

**Expected output**

```text
size: <numeric_result>
```

### Example 2 (Demo Pattern)

```cpp
  auto pred_after_or = restored_or.value().infer(batch.features);
  if (!pred_after_or.ok()) {
    std::cerr << "restored flow infer failed: " << pred_after_or.status().message << "\n";
    return 1;
  }

  const auto before = pred_before_or.value().to(pluss::pCPU, pluss::ScalarType::Float64).to_vec<double>();
  const auto after = pred_after_or.value().to(pluss::pCPU, pluss::ScalarType::Float64).to_vec<double>();
  bool same = before.size() == after.size();
  for (std::size_t i = 0; same && i < before.size(); ++i) {
    same = std::abs(before[i] - after[i]) < 1e-9;
  }
  std::cout << "flow bundle roundtrip exact prediction parity: " << (same ? "true" : "false") << "\n";
  std::cout << "nodej_flow_bundle_demo complete\n";
  return same ? 0 : 2;
}
```

**Expected output**

```text
libc++abi: terminating due to uncaught exception of type pluss::Error: DataReader::with_sizes: explicit sizes are only supported for host-buffer inputs
Triggered at: <path> (DataReader &pluss::detail::DataReader::with_sizes(std::span<const std::int64_t>))
Native stack trace:
  0  pluss::detail::DataReader::with_sizes(std::__1::span<long long const, 18446744073709551615ul>)(data_reader.h:265)
  1  pluss::codem::Series pluss::detail::DataReader::to_series<pluss::codem::Config const&>(pluss::detail::DataReader::Option, pluss::codem::Config const&) const(data_reader.h:310)
  2  pluss::codem::Series pluss::codem::value<pluss::codem::Config const&>(pluss::detail::DataReader, pluss::detail::DataReader::Option, pluss::codem::Config const&)(series_value.h:13)
  3  pluss::codem::Series pluss::series::make<pluss::codem::Config const&>(pluss::detail::DataReader, pluss::detail::DataReader::Option, pluss::codem::Config const&)(series_factory.h:973)
  4  pluss::codem::Series pluss::make_series<pluss::codem::Config const&>(pluss::detail::DataReader, pluss::detail::DataReader::Option, pluss::codem::Config const&)(series_factory_compat.h:20)
  5  main(nodej_flow_bundle_demo.cpp:13)
  6  start + 6992 [dyld]


...
```

### Example 3 (Demo Pattern)

```cpp
  return LabeledBatch{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y,
  };
}

void print_vec(const Series& v) {
  const auto vals = v.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  for (std::size_t i = 0; i < vals.size(); ++i) {
    std::cout << vals[i] << (i + 1 == vals.size() ? '\n' : ' ');
  }
}

} // namespace

int main() {
  const LabeledBatch batch = make_batch();

  const ModelSpec spec{
      .task = TaskKind::Classification,
      .family = ModelFamily::Linear,
```

**Expected output**

```text
scores shape: [12, 3]
metrics [accuracy, f1, logloss]: 1 1 1.24859e-10
flow metrics [accuracy, f1, logloss]: 1 1 1.48016e-09
```

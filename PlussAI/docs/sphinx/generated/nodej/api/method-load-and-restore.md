# `load_and_restore`

## API Signature

- `load_and_restore(path: const std::string&) -> StatusOr<Flow>`

## Notes

Loads a previously persisted artifact and restores its inference-ready state.

## Inputs

- `path` (`const std::string&`)
  Path to persisted model/data artifact.

## Output

- Returns `StatusOr<Flow>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = load_and_restore(/* path */);
if (!res.ok()) {
  out.error("load_and_restore failed", res.status().message);
} else {
  out.info("load_and_restore ok");
}
```

**Expected output**

```text
load_and_restore: ok
status.ok() == true
result.value() is available
```

### Example 2 (Demo Pattern)

```cpp
  const std::string bundle_path = model_dir + "/nodej_flow_bundle_demo.bin";
  auto save_status = nio::save(bundle_path, flow, batch, train);
  if (!save_status.ok()) {
    std::cerr << "save failed: " << save_status.message << "\n";
    return 1;
  }
  std::cout << "saved flow bundle to: " << bundle_path << "\n";

  auto restored_or = nio::load_and_restore(bundle_path);
  if (!restored_or.ok()) {
    std::cerr << "load_and_restore failed: " << restored_or.status().message << "\n";
    return 1;
  }
  auto pred_after_or = restored_or.value().infer(batch.features);
  if (!pred_after_or.ok()) {
    std::cerr << "restored flow infer failed: " << pred_after_or.status().message << "\n";
    return 1;
  }

  const auto before = pred_before_or.value().to(pluss::pCPU, pluss::ScalarType::Float64).to_vec<double>();
  const auto after = pred_after_or.value().to(pluss::pCPU, pluss::ScalarType::Float64).to_vec<double>();
```

**Expected output**

```text
libc++abi: terminating due to uncaught exception of type pluss::Error: DataReader::with_sizes: explicit sizes are only supported for host-buffer inputs
Triggered at: <path> (DataReader &pluss::detail::DataReader::with_sizes(std::span<const std::int64_t>))
Native stack trace:
  0  pluss::detail::DataReader::with_sizes(std::__1::span<long long const, 18446744073709551615ul>)(data_reader.h:250)
  1  pluss::codem::Series pluss::detail::DataReader::to_series<pluss::codem::Config const&>(pluss::detail::DataReader::Option, pluss::codem::Config const&) const(data_reader.h:295)
  2  pluss::codem::Series pluss::codem::value<pluss::codem::Config const&>(pluss::detail::DataReader, pluss::detail::DataReader::Option, pluss::codem::Config const&)(series_value.h:13)
  3  pluss::codem::Series pluss::make_series<pluss::codem::Config const&>(pluss::detail::DataReader, pluss::detail::DataReader::Option, pluss::codem::Config const&)(series_factory.h:780)
  4  main(nodej_flow_bundle_demo.cpp:13)
  5  start + 6992 [dyld]


[exit_code] -6
```

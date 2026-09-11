#include <pluss/pluss.h>

#include <exception>
#include <iostream>
#include <vector>

namespace {

pluss::ScalarType prefer_float32_for_integrals(pluss::ScalarType dtype) {
  switch (dtype) {
    case pluss::ScalarType::UInt8:
    case pluss::ScalarType::UInt16:
    case pluss::ScalarType::UInt32:
    case pluss::ScalarType::UInt64:
    case pluss::ScalarType::Int8:
    case pluss::ScalarType::Int16:
    case pluss::ScalarType::Int32:
    case pluss::ScalarType::Int64:
      return pluss::ScalarType::Float32;
    default:
      return dtype;
  }
}

struct DataReaderSettingsGuard final {
  DataReaderSettingsGuard()
      : policy(pluss::DataReader::default_dtype_policy()),
        float_dtype(pluss::DataReader::default_float_dtype()),
        hook(pluss::DataReader::dtype_hook()),
        strict(pluss::DataReader::strict_mixed_init_list()) {}

  ~DataReaderSettingsGuard() {
    pluss::DataReader::set_default_dtype_policy(policy);
    pluss::DataReader::set_default_float_dtype(float_dtype);
    pluss::DataReader::set_dtype_hook(hook);
    pluss::DataReader::set_strict_mixed_init_list(strict);
  }

  pluss::DataReader::DefaultDTypePolicy policy;
  pluss::ScalarType float_dtype;
  pluss::DataReader::DTypeHook hook;
  bool strict;
};

} // namespace

int main() {
  DataReaderSettingsGuard guard;

  std::cout << "DataReader parity demo\n";

  pluss::DataReader::set_default_dtype_policy(pluss::DataReader::DefaultDTypePolicy::TorchLike);
  pluss::DataReader::set_default_float_dtype(pluss::ScalarType::Float64);

  const auto ints = pluss::make_series(pluss::DataReader(std::vector<int>{1, 2, 3}));
  const auto floats = pluss::make_series(pluss::DataReader(std::vector<float>{1.25f, 2.5f, 3.75f}));

  std::cout << "\n[TorchLike dtype policy]\n";
  std::cout << "ints dtype=" << ints.dtype() << ", values=" << ints << "\n";
  std::cout << "floats dtype=" << floats.dtype() << ", values=" << floats << "\n";

  pluss::DataReader::set_strict_mixed_init_list(true);
  std::cout << "\n[strict mixed init-list failure]\n";
  try {
    const auto mixed = pluss::make_series(pluss::DataReader(
        std::initializer_list<pluss::detail::NestedSeriesLiteral>{
            pluss::detail::NestedSeriesLiteral{1},
            pluss::detail::NestedSeriesLiteral{2.5}}));
    std::cout << "unexpected success: " << mixed << "\n";
    return 1;
  } catch (const std::exception& e) {
    std::cout << e.what() << "\n";
  }

  pluss::DataReader::set_strict_mixed_init_list(false);
  pluss::DataReader::set_dtype_hook(&prefer_float32_for_integrals);

  const auto hooked = pluss::make_series(pluss::DataReader(std::vector<int>{4, 5, 6}));
  std::cout << "\n[custom dtype hook behavior]\n";
  std::cout << "hooked dtype=" << hooked.dtype() << ", values=" << hooked << "\n";

  const auto traced = pluss::make_series(
      pluss::DataReader(std::vector<double>{7.0, 8.0, 9.0}),
      pluss::series_options().with_tracks_slope(true).with_retain_sensitivity(true));
  std::cout << "\n[factory autodiff options]\n";
  std::cout << "is_leaf=" << traced.is_leaf()
            << ", retains_grad=" << traced.retains_grad().to_vec<bool>(0)
            << ", values=" << traced << "\n";

  return 0;
}

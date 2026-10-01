#include <pluss/pluss.h>

#include <iostream>
#include <limits>

namespace {
void heading(const char* label) {
  std::cout << "\n=== " << label << " ===\n";
}
}  // namespace

int main() {
  heading("celu_()");
  { auto x = pluss::make_series({-2.0, 0.0, 2.0}, {3}); x.celu_(); std::cout << x; }
  heading("celu_(double alpha)");
  { auto x = pluss::make_series({-2.0, 0.0, 2.0}, {3}); x.celu_(2.0); std::cout << x; }

  heading("elu_()");
  { auto x = pluss::make_series({-2.0, 0.0, 2.0}, {3}); x.elu_(); std::cout << x; }
  heading("elu_(double alpha)");
  { auto x = pluss::make_series({-2.0, 0.0, 2.0}, {3}); x.elu_(0.5); std::cout << x; }

  heading("gelu_()");
  { auto x = pluss::make_series({-1.0, 0.0, 1.0}, {3}); x.gelu_(); std::cout << x; }

  heading("hardsigmoid_()");
  { auto x = pluss::make_series({-4.0, 0.0, 4.0}, {3}); x.hardsigmoid_(); std::cout << x; }

  heading("hardswish_()");
  { auto x = pluss::make_series({-4.0, 0.0, 4.0}, {3}); x.hardswish_(); std::cout << x; }

  heading("leaky_relu_(double negative_slope)");
  { auto x = pluss::make_series({-2.0, 0.0, 2.0}, {3}); x.leaky_relu_(0.1); std::cout << x; }

  heading("mish_()");
  { auto x = pluss::make_series({-2.0, 0.0, 2.0}, {3}); x.mish_(); std::cout << x; }

  heading("rrelu_(bool training=false)");
  { auto x = pluss::make_series({-2.0, 0.0, 2.0}, {3}); x.rrelu_(false); std::cout << x; }

  heading("selu_()");
  { auto x = pluss::make_series({-2.0, 0.0, 2.0}, {3}); x.selu_(); std::cout << x; }

  heading("isnan_()");
  { auto x = pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), 3.0}, {3});
    x.isnan_(); std::cout << x; }
  heading("isinf_()");
  { auto x = pluss::make_series({1.0, std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()}, {3});
    x.isinf_(); std::cout << x; }
  heading("isfinite_()");
  { auto x = pluss::make_series({1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}, {3});
    x.isfinite_(); std::cout << x; }
  heading("nan_to_num_(nan, posinf, neginf)");
  { auto x = pluss::make_series({std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()}, {3});
    x.nan_to_num_(0.0, 1e9, -1e9); std::cout << x; }

  heading("logical_and_");
  { auto x = pluss::make_series({true, true, false}, pluss::ScalarType::Bool);
    auto y = pluss::make_series({true, false, false}, pluss::ScalarType::Bool);
    x.logical_and_(y); std::cout << x; }
  heading("logical_or_");
  { auto x = pluss::make_series({true, true, false}, pluss::ScalarType::Bool);
    auto y = pluss::make_series({true, false, false}, pluss::ScalarType::Bool);
    x.logical_or_(y); std::cout << x; }
  heading("logical_xor_");
  { auto x = pluss::make_series({true, true, false}, pluss::ScalarType::Bool);
    auto y = pluss::make_series({true, false, false}, pluss::ScalarType::Bool);
    x.logical_xor_(y); std::cout << x; }

  heading("bitwise_and_");
  { auto x = pluss::make_series(std::vector<std::int64_t>{6, 12}, {2});
    auto y = pluss::make_series(std::vector<std::int64_t>{3, 10}, {2});
    x.bitwise_and_(y); std::cout << x; }
  heading("bitwise_or_");
  { auto x = pluss::make_series(std::vector<std::int64_t>{6, 12}, {2});
    auto y = pluss::make_series(std::vector<std::int64_t>{3, 10}, {2});
    x.bitwise_or_(y); std::cout << x; }
  heading("bitwise_xor_");
  { auto x = pluss::make_series(std::vector<std::int64_t>{6, 12}, {2});
    auto y = pluss::make_series(std::vector<std::int64_t>{3, 10}, {2});
    x.bitwise_xor_(y); std::cout << x; }
  heading("bitwise_not_");
  { auto x = pluss::make_series(std::vector<std::int64_t>{0, -1, 5}, {3});
    x.bitwise_not_(); std::cout << x; }

  return 0;
}

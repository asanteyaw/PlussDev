#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

static void show(const char* label, const std::vector<std::int64_t>& idx) {
  std::cout << label;
  for (auto i : idx) std::cout << " " << i;
  std::cout << "\n";
}

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series amount = pluss::make_series({1200., 1800., 3400., 4100., 5200., 6000.}, {6}, f64);
  const pluss::Series ratio  = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Slate(amount)}, {"ratio", Slate(ratio)}, {"region", Slate(region)}});

  // The remainder policy is plain data on the Route, readable back after it is
  // set. with_remainder returns the Route so it chains onto a factory call.
  lymph::prep::Route r = lymph::prep::route::names({"amount"});
  std::cout << "default remainder is PassThrough: " << std::boolalpha
            << (r.remainder == lymph::RouteRemainderPolicy::PassThrough) << "\n";
  r.with_remainder(lymph::RouteRemainderPolicy::Drop);
  std::cout << "after with_remainder(Drop):       "
            << (r.remainder == lymph::RouteRemainderPolicy::Drop) << "\n";

  return 0;
}

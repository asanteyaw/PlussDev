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
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series amount = pluss::make_series({1200., 1800., 3400., 4100., 5200., 6000.}, {6}, f64);
  const pluss::Series ratio  = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Thread(amount)}, {"ratio", Thread(ratio)}, {"region", Thread(region)}});

  // remaining_of(Route) keeps the complement tied to another route's logic, so a
  // predicate-driven selection and its remainder can never drift apart.
  const lymph::prep::Route numeric = lymph::prep::route::where(
      [](std::string_view, std::string_view dtype) { return dtype == "float64"; });
  show("numeric (where float64):", numeric.resolve(x));
  show("remaining_of(numeric):  ", lymph::prep::route::remaining_of(numeric).resolve(x));
  // Every column here is float64, so the remainder is empty -- exactly the
  // complement of whatever the predicate selects.

  return 0;
}

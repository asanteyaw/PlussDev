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

  // A Route is a value: build it once, resolve it against as many frames as you
  // like, in whatever column order each frame happens to have.
  const lymph::prep::Route r = lymph::prep::route::names({"region", "amount"});
  show("against x (amount, ratio, region): ", r.resolve(x));
  const DataFrame x2({{"region", Slate(region)}, {"amount", Slate(amount)}, {"ratio", Slate(ratio)}});
  show("against x2 (region, amount, ratio):", r.resolve(x2));
  std::cout << "the selected columns are the same; only their positions differ.\n";

  return 0;
}

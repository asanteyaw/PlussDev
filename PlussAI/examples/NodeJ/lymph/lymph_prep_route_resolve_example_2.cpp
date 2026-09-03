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
  const pluss::Series amount = pluss::make_series({1., 2., 3., 4., 5., 6., 7., 8.}, {8}, f64);
  const pluss::Series ratio  = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4, 0.8, 1.2}, {8}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2, 0, 1}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"amount", Thread(amount)}, {"ratio", Thread(ratio)}, {"region", Thread(region)}});

  // The vector has one entry per route name, in the route's own list order; each
  // entry is that name's position in the ordered space you resolve against.
  const lymph::prep::Route r = lymph::prep::route::names({"region", "amount"});
  show("ordered = amount, ratio, region -> [pos(region), pos(amount)]:", r.resolve(x, {"amount", "ratio", "region"}));
  show("ordered = region, ratio, amount -> [pos(region), pos(amount)]:", r.resolve(x, {"region", "ratio", "amount"}));

  return 0;
}

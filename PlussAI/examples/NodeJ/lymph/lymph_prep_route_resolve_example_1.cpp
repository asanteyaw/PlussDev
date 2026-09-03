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

  // resolve(frame) uses the frame's own column order; resolve(frame, ordered)
  // uses the list you pass. They agree when the list matches the frame.
  const lymph::prep::Route r = lymph::prep::route::names({"ratio", "region"});
  show("resolve(x):                          ", r.resolve(x));
  show("resolve(x, {amount, ratio, region}): ", r.resolve(x, {"amount", "ratio", "region"}));

  return 0;
}

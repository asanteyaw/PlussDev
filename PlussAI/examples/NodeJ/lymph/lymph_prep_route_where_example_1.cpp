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

  // route::where takes a predicate over (feature name, dtype). dtype is the
  // column element type as a string, e.g. "float64".
  const lymph::prep::Route by_name = lymph::prep::route::where(
      [](std::string_view name, std::string_view) { return name.rfind("r", 0) == 0; });
  show("where(name starts with r):", by_name.resolve(x));
  const lymph::prep::Route by_type = lymph::prep::route::where(
      [](std::string_view, std::string_view dtype) { return dtype == "float64"; });
  show("where(dtype == float64):  ", by_type.resolve(x));

  return 0;
}

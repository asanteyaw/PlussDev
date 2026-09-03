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

  const pluss::Series price_usd = pluss::make_series({12., 18., 34., 41., 52., 60.}, {6}, f64);
  const pluss::Series price_eur = pluss::make_series({11., 17., 31., 39., 48., 55.}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"price_usd", Thread(price_usd)}, {"price_eur", Thread(price_eur)}, {"region", Thread(region)}});

  // Route a scaler at every "price_" column by prefix -- no need to list them.
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::standard_scaler(),
               lymph::prep::route::where([](std::string_view n, std::string_view) {
                 return n.rfind("price_", 0) == 0;
               }));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"region", Thread(region)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}

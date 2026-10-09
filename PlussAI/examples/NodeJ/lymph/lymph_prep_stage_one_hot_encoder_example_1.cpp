#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // A categorical region code and one numeric feature.
  const pluss::Series spend = pluss::make_series({120.0, 200.0, 90.0, 260.0, 140.0, 300.0}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 1, 0, 1, 0, 1}, {6}, f64);
  const DataFrame x({{"spend", Slate(spend)}, {"region", Slate(region)}});

  lymph::models::Model model = lymph::models::logistic_regression();

  // The default stage expands the routed categorical column into one indicator
  // column per category. No options argument means a default-constructed
  // lymph::options::OneHotEncoder.
  model.append(lymph::prep::stage::one_hot_encoder(),
               lymph::prep::route::names({"region"}));
  model.learn_(x, y);

  std::cout << "stage_count = " << model.stage_count() << "\n";
  std::cout << "trained     = " << std::boolalpha << model.trained() << "\n\n";

  const DataFrame review({
      {"spend", Slate(spend)},
      {"region", Slate(region)},
      {"actual", Slate(y)},
      {"predicted", Slate(model.infer(x))},
  });
  std::cout << review;

  return 0;
}

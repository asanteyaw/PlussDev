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

  // Three region categories plus one numeric feature. The default stage maps
  // each routed category to a small integer code (1-based; 0 is reserved for
  // an unseen category).
  const pluss::Series spend = pluss::make_series({120.0, 200.0, 90.0, 260.0, 140.0, 300.0}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 1, 0, 1, 0, 1}, {6}, f64);
  const DataFrame x({{"spend", Slate(spend)}, {"region", Slate(region)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::ordinal_encoder(),
               lymph::prep::route::names({"region"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"region", Slate(region)},
                          {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}

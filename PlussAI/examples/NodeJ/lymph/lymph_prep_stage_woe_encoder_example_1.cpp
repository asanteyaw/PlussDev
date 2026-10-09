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

  // woe_encoder is supervised and strictly for a binary 0/1 target. Each
  // category becomes its log-odds contribution: positive if the category skews
  // toward class 1, negative toward class 0.
  const pluss::Series region = pluss::make_series({0,0,1,1,2,2,3,3,0,2}, {10}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,0,1,1,1,1,0,1}, {10}, f64);
  const DataFrame x({{"region", Slate(region)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::woe_encoder(), lymph::prep::route::names({"region"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"region", Slate(region)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}

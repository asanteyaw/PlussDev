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

  // target_encoder is supervised: attach it and the fitted target reaches the
  // stage during Model::learn(x, y). Each category is replaced by a smoothed
  // mean of the target for that category.
  const pluss::Series region = pluss::make_series({0,0,1,1,2,2,3,3,0,2}, {10}, f64);
  const pluss::Series noise  = pluss::make_series({.1,.9,.2,.8,.3,.7,.4,.6,.5,.5}, {10}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,0,1,1,1,1,0,1}, {10}, f64);
  const DataFrame x({{"region", Slate(region)}, {"noise", Slate(noise)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::target_encoder(), lymph::prep::route::names({"region"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"region", Slate(region)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}

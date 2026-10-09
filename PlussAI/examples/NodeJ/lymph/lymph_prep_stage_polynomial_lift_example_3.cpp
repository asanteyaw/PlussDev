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

  const pluss::Series a = pluss::make_series({-2., -1., 0., 1., 2., 3.}, {6}, f64);
  const pluss::Series b = pluss::make_series({1., 0., -1., 0., 1., 2.}, {6}, f64);
  const pluss::Series region = pluss::make_series({0,1,2,0,1,2}, {6}, f64);
  const pluss::Series y = pluss::make_series({1,0,0,0,1,1}, {6}, f64);
  const DataFrame x({{"a", Slate(a)}, {"b", Slate(b)}, {"region", Slate(region)}});

  // lift the numeric pair, scale the lifted result, one-hot the categorical one.
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::polynomial_lift(), lymph::prep::route::names({"a", "b"}))
       .append(lymph::prep::stage::standard_scaler(), lymph::prep::route::names({"a", "b"}))
       .append(lymph::prep::stage::one_hot_encoder(), lymph::prep::route::names({"region"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"a", Slate(a)}, {"b", Slate(b)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}

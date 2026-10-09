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

  // A high-cardinality categorical alongside a numeric feature: target-encode
  // the category to a single informative column, scale the numeric one.
  const pluss::Series city = pluss::make_series({0,1,2,3,4,5,6,7,0,3}, {10}, f64);
  const pluss::Series size = pluss::make_series({1.,2.,3.,4.,5.,6.,7.,8.,1.,4.}, {10}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,0,1,1,1,1,0,0}, {10}, f64);
  const DataFrame x({{"city", Slate(city)}, {"size", Slate(size)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::standard_scaler(), lymph::prep::route::names({"size"}))
       .append(lymph::prep::stage::target_encoder(), lymph::prep::route::names({"city"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"city", Slate(city)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}

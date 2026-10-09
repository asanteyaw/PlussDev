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

  const pluss::Series amount = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series grade = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Slate(amount)}, {"grade", Slate(grade)}});

  // label-encode the ordinal-ish column, minmax-scale the numeric one.
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append({
      {lymph::prep::stage::minmax_scaler(), lymph::prep::route::names({"amount"})},
      {lymph::prep::stage::label_encoder(), lymph::prep::route::names({"grade"})},
  });
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"grade", Slate(grade)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}

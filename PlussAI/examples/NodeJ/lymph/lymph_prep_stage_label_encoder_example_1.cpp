#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {

  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // label_encoder integer-codes a routed column, like ordinal_encoder but with
  // one_based = false by default -- the shape used for a target-style label
  // column that a downstream step expects as 0..K-1.
  const pluss::Series grade = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series score = pluss::make_series({0.2, 0.5, 0.9, 0.3, 0.6, 0.8}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 1, 0, 1, 1}, {6}, f64);
  const DataFrame x({{"grade", Thread(grade)}, {"score", Thread(score)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::label_encoder(), lymph::prep::route::names({"grade"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"grade", Thread(grade)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}

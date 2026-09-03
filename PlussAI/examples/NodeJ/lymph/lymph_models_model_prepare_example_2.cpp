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

  // A numeric column and a categorical one encoded as small integer codes:
  // region in {0, 1, 2}.
  const pluss::Series amount = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 1, 0, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Thread(amount)}, {"region", Thread(region)}});

  lymph::models::Model model = lymph::models::logistic_regression();

  // Calls chain. Stages are recorded in the order written and later fitted in
  // that same order: scale the numeric column, one-hot expand the categorical
  // one.
  model.prepare()
      .scale({"amount"})
      .encode({"region"});

  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n";
  std::cout << "trained     = " << std::boolalpha << model.trained() << "\n\n";

  const DataFrame review({
      {"amount", Thread(amount)},
      {"region", Thread(region)},
      {"actual", Thread(y)},
      {"predicted", Thread(model.infer(x))},
  });
  std::cout << review;

  return 0;
}

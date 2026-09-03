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

  const pluss::Series a = pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}, {6}, f64);
  const pluss::Series b = pluss::make_series({1.0, 3.0, 2.0, 5.0, 4.0, 6.0}, {6}, f64);
  const pluss::Series c = pluss::make_series({0.1, 0.2, 0.15, 0.3, 0.25, 0.35}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"a", Thread(a)}, {"b", Thread(b)}, {"c", Thread(c)}});

  lymph::models::Model model = lymph::models::logistic_regression();

  // A route can be given by explicit names, or built from a predicate over
  // (feature_name, dtype), or as "everything not already named". Here: scale
  // `a` by name, then scale every remaining column with one more stage.
  lymph::models::ModelPrep prep = model.prepare();
  prep.scale(prep.names({"a"}));
  prep.scale(prep.remaining({"a"}));

  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count()
            << "   (one for `a`, one for the rest)\n";
  std::cout << "trained     = " << std::boolalpha << model.trained() << "\n\n";

  const DataFrame review({
      {"a", Thread(a)},
      {"b", Thread(b)},
      {"c", Thread(c)},
      {"predicted", Thread(model.infer(x))},
  });
  std::cout << review;

  return 0;
}

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

  const pluss::Series amount = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series ratio = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Thread(amount)}, {"ratio", Thread(ratio)}});

  // A Stage is inert until it is paired with a Route and handed to a model.
  // append(stage, route) is that pairing: the stage says what, the route says
  // which columns. The full-scope spelling and the family alias are the same
  // call.
  lymph::models::Model explicit_scope = lymph::models::logistic_regression();
  explicit_scope.append(lymph::prep::stage::standard_scaler(),
                        lymph::prep::route::names({"amount", "ratio"}));
  explicit_scope.learn_(x, y);

  // prepare() wraps the same append with a named shorthand; scale({...}) emits
  // exactly the stage above.
  lymph::models::Model via_prepare = lymph::models::logistic_regression();
  via_prepare.prepare().scale({"amount", "ratio"});
  via_prepare.learn_(x, y);

  const DataFrame review({
      {"amount", Thread(amount)},
      {"ratio", Thread(ratio)},
      {"pred_append", Thread(explicit_scope.infer(x))},
      {"pred_prepare", Thread(via_prepare.infer(x))},
  });
  std::cout << "stage_count: append=" << explicit_scope.stage_count()
            << ", prepare=" << via_prepare.stage_count() << "\n\n";
  std::cout << review;

  return 0;
}

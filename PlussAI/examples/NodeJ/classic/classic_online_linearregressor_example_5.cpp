#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::LeastSquares;
  cls::online::OnlineLinearRegressor model(options);

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  batch.y = pluss::make_series({1.0, 2.0}, {2}, f64);

  const cls::Status before_learn = model.advance(batch);
  std::cout << "advance() before learn() ok = " << std::boolalpha << before_learn.ok() << "\n";
  std::cout << "message = \"" << before_learn.message << "\"\n";

  return 0;
}

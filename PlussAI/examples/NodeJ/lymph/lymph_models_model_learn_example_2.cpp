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

  const pluss::Series income = pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"income", Thread(income)}, {"balance", Thread(balance)}});

  // A template held by const: it must never become trained itself.
  const lymph::models::Model blueprint = lymph::models::logistic_regression();

  // learn returns a trained copy and leaves `blueprint` untouched. Two fits
  // from the same template with different iteration budgets are independent
  // values.
  lymph::options::Train converged{};
  converged.max_iterations = 800;
  const lymph::models::Model fitted_converged = blueprint.learn(x, y, converged);

  lymph::options::Train hurried{};
  hurried.max_iterations = 2;
  const lymph::models::Model fitted_hurried = blueprint.learn(x, y, hurried);

  std::cout << "blueprint.trained()        = " << std::boolalpha << blueprint.trained() << "\n";
  std::cout << "fitted_converged.trained() = " << fitted_converged.trained() << "\n";
  std::cout << "fitted_hurried.trained()   = " << fitted_hurried.trained() << "\n\n";

  // Hard predictions can agree while the underlying decision scores differ,
  // which is how you see the two fitted models really are distinct values.
  const DataFrame compare({
      {"actual", Thread(y)},
      {"score_800_iters", Thread(fitted_converged.infer_scores(x))},
      {"score_2_iters", Thread(fitted_hurried.infer_scores(x))},
  });
  std::cout << compare;

  return 0;
}

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

  const pluss::Series income = pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"income", Slate(income)}, {"balance", Slate(balance)}});

  // The common mistake: assuming `learn` mutates the caller.
  lymph::models::Model candidate = lymph::models::logistic_regression();
  const lymph::models::Model fitted = candidate.learn(x, y);
  std::cout << "candidate.trained() = " << std::boolalpha << candidate.trained()
            << "   (still a template -- learn returned a copy)\n";
  std::cout << "fitted.trained()    = " << fitted.trained()
            << "   (the value you must keep)\n\n";

  // Asking an untrained model to infer is a hard error, not a silent guess.
  try {
    const pluss::Series ignored = candidate.infer(x);
    (void)ignored;
  } catch (const std::exception& err) {
    std::cout << "candidate.infer(x) threw: " << err.what() << "\n\n";
  }

  // warm_start controls whether a second learn_ continues from current
  // parameters or restarts. Two short bursts with warm_start should land in
  // the same place as one longer run.
  lymph::options::Train burst{};
  burst.max_iterations = 50;
  burst.warm_start = true;

  lymph::models::Model incremental = lymph::models::logistic_regression();
  incremental.learn_(x, y, burst);
  incremental.learn_(x, y, burst);

  lymph::models::Model one_shot = lymph::models::logistic_regression();
  lymph::options::Train single{};
  single.max_iterations = 100;
  one_shot.learn_(x, y, single);

  const DataFrame compare({
      {"income", Slate(income)},
      {"balance", Slate(balance)},
      {"two_warm_bursts", Slate(incremental.infer(x))},
      {"one_100_iter_run", Slate(one_shot.infer(x))},
  });
  std::cout << compare;

  return 0;
}

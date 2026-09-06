#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// calibrate_with_platt() returns a TrainOptions with calibration set; passed to
// learn(), the fitted model calibrates the *probabilities* it uses for infer()
// and probability-based metrics. The raw infer_scores() decision function is
// left alone by design.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0, 5.5,
       6.0, 6.5, 7.0, 7.5, 8.0, 8.5}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x", Thread(x)}, {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x"};
  data_options.label_feature = "target";

  const lymph::TrainOptions calibrated = lymph::calibrate_with_platt();
  std::cout << "calibration mode       = " << static_cast<int>(calibrated.calibration)
            << " (Platt)\n";
  std::cout << "calibration_iterations = " << calibrated.calibration_iterations << "\n";
  std::cout << "calibration_tolerance  = " << calibrated.calibration_tolerance << "\n";

  const lymph::models::Model plain =
      lymph::models::logistic_regression().learn(frame, data_options);
  const lymph::models::Model platt =
      lymph::models::logistic_regression().learn(frame, data_options, calibrated);

  std::cout << "platt model trained    = " << std::boolalpha << platt.trained() << "\n";
  std::cout << "infer_scores identical = "
            << (lymph::metrics::max_abs_diff(plain.infer_scores(frame, data_options),
                                             platt.infer_scores(frame, data_options)) == 0.0)
            << "  (calibration does not touch the raw decision function)\n";

  return 0;
}

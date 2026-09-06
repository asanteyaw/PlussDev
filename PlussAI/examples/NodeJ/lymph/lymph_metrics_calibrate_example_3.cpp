#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// Each helper takes a TrainOptions and returns it with only the calibration
// fields changed -- so you can layer calibration onto training settings you
// already configured, and without_calibration(...) strips it back off.
int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::TrainOptions base{};
  base.max_iterations = 750;
  base.tolerance = 1e-7;
  base.random = {.seed = 42ULL, .deterministic = true};

  const lymph::TrainOptions with_platt = lymph::calibrate_with_platt(base, 200);
  std::cout << "max_iterations kept       = " << with_platt.max_iterations << "\n";
  std::cout << "seed kept                 = " << with_platt.random.seed << "\n";
  std::cout << "calibration               = " << static_cast<int>(with_platt.calibration) << "\n";
  std::cout << "calibration_iterations    = " << with_platt.calibration_iterations << "\n";

  const lymph::TrainOptions stripped = lymph::without_calibration(with_platt);
  std::cout << "after without_calibration  = " << static_cast<int>(stripped.calibration)
            << "  (max_iterations still " << stripped.max_iterations << ")\n";

  return 0;
}

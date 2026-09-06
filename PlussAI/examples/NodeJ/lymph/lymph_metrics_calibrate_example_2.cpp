#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// The other calibration presets: isotonic (monotone step fit) and temperature
// (single-parameter logit rescale). Each just sets TrainOptions.calibration to
// the matching ScoreCalibration mode.
int main() {
  namespace lymph = pluss::ai::lymph;

  const lymph::TrainOptions none = lymph::without_calibration();
  const lymph::TrainOptions platt = lymph::calibrate_with_platt();
  const lymph::TrainOptions isotonic = lymph::calibrate_with_isotonic();
  const lymph::TrainOptions temperature = lymph::calibrate_with_temperature();

  std::cout << "without_calibration    -> " << static_cast<int>(none.calibration) << "\n";
  std::cout << "calibrate_with_platt   -> " << static_cast<int>(platt.calibration) << "\n";
  std::cout << "calibrate_with_isotonic -> " << static_cast<int>(isotonic.calibration) << "\n";
  std::cout << "calibrate_with_temperature -> " << static_cast<int>(temperature.calibration) << "\n";

  return 0;
}

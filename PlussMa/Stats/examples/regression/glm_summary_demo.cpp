#include <pluss/ma.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(std::vector<double>{-2, -1, 0, 1, 2, 3, 4, 5}, {8, 1}, cfg);
  const pluss::Series y = pluss::make_series(
      std::vector<double>{0.06, 0.12, 0.22, 0.40, 0.62, 0.79, 0.90, 0.96}, {8}, cfg);

  pluss::ma::reg::GLMOptions options;
  options.family = pluss::ma::reg::GLMFamily::Binomial;
  options.link = pluss::ma::reg::GLMLink::Logit;
  options.response_name = "success_prob";
  options.predictor_names = {"feature"};

  const auto fit = pluss::ma::reg::glm_fit(y, x, options, cfg);
  std::cout << pluss::ma::reg::glm_summary(fit) << '\n';

  const pluss::Series x_test = pluss::make_series(std::vector<double>{-1.5, 0.5, 2.5, 4.5}, {4, 1}, cfg);
  const pluss::Series pred = pluss::ma::reg::glm_predict(fit, x_test, pluss::ma::reg::GLMPredictionType::Mean, cfg);
  std::cout << "Predictions\n" << pred << '\n';
  return 0;
}

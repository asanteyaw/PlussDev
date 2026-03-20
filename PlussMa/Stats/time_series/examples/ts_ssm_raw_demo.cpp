#include <pluss/ma.h>

#include <cstdint>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace {

template <class T>
pluss::Series make_series_cfg(const std::vector<T>& values,
                              std::initializer_list<std::int64_t> sizes,
                              pluss::Config cfg) {
  return pluss::make_series(values,
                            sizes,
                            pluss::cfg::dtype(cfg.dtype),
                            pluss::cfg::platform(cfg.platform));
}

std::string shape_of(const pluss::Series& x) {
  std::ostringstream oss;
  oss << "[";
  const auto s = x.sizes();
  for (std::size_t i = 0; i < s.size(); ++i) {
    if (i > 0) {
      oss << ",";
    }
    oss << s[i];
  }
  oss << "]";
  return oss.str();
}

} // namespace

int main() {
  using pluss::subsp::stats::ts::Ssm;
  using pluss::subsp::stats::ts::SsmModel;
  using pluss::subsp::stats::ts::SsmOptions;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const double nan = std::numeric_limits<double>::quiet_NaN();

  SsmModel mdl;
  mdl.transition = make_series_cfg(std::vector<double>{0.92}, {1, 1}, cfg);
  mdl.state_loading = make_series_cfg(std::vector<double>{0.20}, {1, 1}, cfg);
  mdl.observation = make_series_cfg(std::vector<double>{1.10}, {1, 1}, cfg);
  mdl.innovation_loading = make_series_cfg(std::vector<double>{0.35}, {1, 1}, cfg);
  mdl.mean0 = make_series_cfg(std::vector<double>{0.0}, {1}, cfg);
  mdl.cov0 = make_series_cfg(std::vector<double>{1.0}, {1, 1}, cfg);

  const pluss::Series y = make_series_cfg(
      std::vector<double>{0.20, -0.10, 0.40, nan, 0.10, -0.20, 0.30, 0.00},
      {8, 1},
      cfg);

  SsmOptions opt;
  opt.tolerance = 1e-12;

  const auto filt = Ssm::filter(y, mdl, opt, cfg);
  const auto smth = Ssm::smooth(y, mdl, opt, cfg);
  const auto fcst = Ssm::forecast(6, y, mdl, opt, cfg);
  const auto sim = Ssm::simulate(12, mdl, 3, cfg, 20260319ULL);

  std::cout << "SSM filter loglik=" << filt.loglik << "\n";
  std::cout << "filtered_states shape=" << shape_of(filt.filtered_states) << "\n";
  std::cout << "forecasted_obs shape=" << shape_of(filt.forecasted_obs) << "\n";
  std::cout << "data_used=" << filt.data_used << "\n\n";

  std::cout << "smoothed_states shape=" << shape_of(smth.smoothed_states) << "\n";
  std::cout << "smoothed_states_cov shape=" << shape_of(smth.smoothed_states_cov) << "\n\n";

  std::cout << "forecast observations shape=" << shape_of(fcst.observations) << "\n";
  std::cout << "forecast observations=" << fcst.observations << "\n";
  std::cout << "forecast observations_cov=" << fcst.observations_cov << "\n\n";

  std::cout << "simulate observations shape=" << shape_of(sim.observations) << "\n";
  std::cout << "simulate states shape=" << shape_of(sim.states) << "\n";
  std::cout << "simulate state_disturbance shape=" << shape_of(sim.state_disturbance) << "\n";
  std::cout << "simulate obs_innovation shape=" << shape_of(sim.obs_innovation) << "\n";
  return 0;
}

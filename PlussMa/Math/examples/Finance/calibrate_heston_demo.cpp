#include <pluss/mic.h>
#include <pluss/ma.h>

#include <filesystem>
#include <iostream>

namespace {

std::filesystem::path data_file(const std::string& name) {
  return std::filesystem::path(PLUSSMIC_SOURCE_DIR) / "data" / name;
}

} // namespace

int main() {
  namespace fin = pluss::subsp::math::finance;
  namespace opt = pluss::subsp::math::optimization;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  auto df = pluss::table::DataFrame::load_csv(data_file("options_data.csv").string());
  df = df.filter(
      (df.features("bDTM") <= 252.0) &
      (df.features("OptionPrice") > 0.0));

  fin::module::Field pipe{
      .instrument = fin::instrument::Vanilla{
          .type = fin::pricing::OptionType::Auto,
          .is_call = df.features("isCall").as_series(),
          .strike = df.features("Strike").as_series(),
          .maturity = (df.features("bDTM") / 252.0).as_series(),
      },
      .market = fin::market::State{
          .spot = df.features("StockPrice").as_series(),
          .rate = df.features("RiskFree").as_series(),
          .dividend_yield = df.features("Dividends").as_series(),
      },
      .model = fin::model::make(1.5, 0.04, 0.3, -0.7, 0.04, cfg),
      .controls = fin::module::Controls{
          .engine = fin::module::Engine::Cos,
          .risk = fin::module::Risk::PriceOnly,
          .loss = fin::module::Loss::MSE,
      },
  };

  const auto market_px = df.features("OptionPrice").as_series();

  const pluss::Series x0 = pluss::make_series({1.2, 0.04, 0.35, -0.6, 0.05},  cfg);
  opt::PSConfig ps{};
  ps.max_iterations = 300;
  ps.max_evaluations = 4000;
  ps.mesh_tolerance = 1.0e-4;
  ps.step_tolerance = 1.0e-4;
  ps.value_tolerance = 1.0e-8;
  ps.initial_mesh_size = 0.25;
  ps.mesh_expansion = 2.0;
  ps.mesh_contraction = 0.5;
  ps.lower_bounds = pluss::make_series({0.05, 1.0e-4, 0.05, -0.99, 1.0e-4}, {5}, cfg);
  ps.upper_bounds = pluss::make_series({12.0, 0.5, 2.5, 0.99, 0.5}, {5}, cfg);

  const auto out = opt::pattern_search_minimizer([&](const pluss::Series& x) {
    pipe.model.parameters(x, cfg);
    return pipe.objective(market_px, cfg);
  }, x0, ps);
  const pluss::Series xb = out.minimizer.to(pluss::pCPU, pluss::ScalarType::Float64).reshape({5});

  std::cout << "rows=" << df.ncases() << "\n";
  std::cout << "status=" << opt::to_string(out.status)
            << " evals=" << out.diagnostics.evaluations
            << " mse=" << out.minimum << "\n";
  std::cout << "kappa=" << xb.select(0, 0).item<double>()
            << " theta=" << xb.select(0, 1).item<double>()
            << " sigma=" << xb.select(0, 2).item<double>()
            << " rho=" << xb.select(0, 3).item<double>()
            << " v0=" << xb.select(0, 4).item<double>() << "\n";
  return 0;
}

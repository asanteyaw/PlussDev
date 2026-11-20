// Is this series predictable? The time-series diagnostics in one program: lag structure, autocorrelation, partial autocorrelation,
// cross-correlation with another series, and the bootstrap spread of the mean.
#include <pluss/pl/plusspl.h>
#include <pluss/pluss.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace pluss::pl;

static std::string out(const char *name) {
  const char *dir = std::getenv("PLUSSMA_GUIDE_ASSETS");
  return ((dir ? std::filesystem::path(dir) : std::filesystem::path(".")) / name).string();
}


int main() {
  auto loaded = flash::load_dataset("regimes");
  const auto data = loaded->frame();
  auto signal = data.features("signal").as_series().slice(0, 0, 600);
  auto trend = data.features("trend").as_series().slice(0, 0, 600);
  auto volatility = data.features("volatility").as_series().slice(0, 0, 600);
  const double lag1 = ((signal.slice(0, 1, 600) - signal.mean()) * (signal.slice(0, 0, 599) - signal.mean())).sum().item<double>() / (signal - signal.mean()).square().sum().item<double>();
  std::cout << "lag-1 autocorrelation " << lag1 << "\n";

  auto lag = studio("diag_lag");
  flash::lagplot(lag, signal);
  lag.show();
  auto acf = studio("diag_acf");
  flash::acfplot(acf, signal);
  acf.show();
  auto pacf = studio("diag_pacf");
  flash::pacfplot(pacf, signal);
  pacf.show();
  auto act = studio("diag_act");
  flash::actplot(act, signal);
  act.show();
  auto ccf = studio("diag_ccf");
  flash::ccfplot(ccf, signal, volatility);
  ccf.show();
  auto resid = studio("diag_resid_acf");
  flash::resid_acfplot(resid, trend, signal);
  resid.show();
  auto boot = studio("diag_bootstrap");
  flash::bootstrapplot(boot, signal);
  boot.show();
  std::cout << "seven diagnostic figures written\n";
}

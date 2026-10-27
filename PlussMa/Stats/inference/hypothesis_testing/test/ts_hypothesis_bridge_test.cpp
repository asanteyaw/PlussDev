#include <pluss/ma.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

Series make_random_walk(std::int64_t n, std::uint64_t seed) {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto eps = pluss::series::randn({n}, cfg, seed).host<double>();
  std::vector<double> y(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t t = 1; t < n; ++t) {
    y[static_cast<std::size_t>(t)] = y[static_cast<std::size_t>(t - 1)] + eps[static_cast<std::size_t>(t)];
  }
  return pluss::make_series(y, {n}, cfg);
}

} // namespace

int main() {
  try {
    const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
    const Series y = make_random_walk(2500, 20260422ULL);

    pluss::subsp::stats::ts::AdfOptions adf_opt;
    adf_opt.lags = 1;
    adf_opt.model = pluss::subsp::stats::ts::AdfModel::ARD;
    const auto ts_adf = pluss::subsp::stats::ts::adf_test(y, adf_opt, cfg);
    const auto inf_adf = pluss::subsp::stats::inference::hypothesis::adf_test(y, adf_opt, cfg);

    expect(std::abs(ts_adf.stat - inf_adf.stat) < 1e-12, "adf bridge stat mismatch");
    expect(std::abs(ts_adf.pvalue - inf_adf.pvalue) < 1e-12, "adf bridge pvalue mismatch");

    pluss::subsp::stats::ts::KpssOptions kpss_opt;
    kpss_opt.lags = 8;
    kpss_opt.trend = false;
    const auto ts_kpss = pluss::subsp::stats::ts::kpss_test(y, kpss_opt, cfg);
    const auto inf_kpss = pluss::subsp::stats::inference::hypothesis::kpss_test(y, kpss_opt, cfg);

    expect(std::abs(ts_kpss.stat - inf_kpss.stat) < 1e-12, "kpss bridge stat mismatch");
    expect(std::abs(ts_kpss.pvalue - inf_kpss.pvalue) < 1e-12, "kpss bridge pvalue mismatch");

    pluss::subsp::stats::ts::ArchLmOptions arch_opt;
    arch_opt.lags = 8;
    const auto ts_arch = pluss::subsp::stats::ts::arch_lm(y, arch_opt, cfg);
    const auto inf_arch = pluss::subsp::stats::inference::hypothesis::arch_lm(y, arch_opt, cfg);

    expect(std::abs(ts_arch.stat - inf_arch.stat) < 1e-12, "arch_lm bridge stat mismatch");
    expect(std::abs(ts_arch.pvalue - inf_arch.pvalue) < 1e-12, "arch_lm bridge pvalue mismatch");

    const Series base = pluss::series::randn({2048}, cfg, 20260424ULL);
    const Series err_a = base.add(1.0);
    const Series err_b = base;
    pluss::subsp::stats::ts::DmOptions dm_opt;
    dm_opt.loss = pluss::subsp::stats::ts::DmLoss::Squared;
    dm_opt.tail = pluss::subsp::stats::ts::DmTail::Right;
    dm_opt.horizon = 1;
    dm_opt.alpha = 0.01;

    const auto ts_dm = pluss::subsp::stats::ts::dm_test(err_a, err_b, dm_opt, cfg);
    const auto inf_dm = pluss::subsp::stats::inference::hypothesis::dm_test(err_a, err_b, dm_opt, cfg);

    expect(std::abs(ts_dm.stat - inf_dm.stat) < 1e-12, "dm_test bridge stat mismatch");
    expect(std::abs(ts_dm.pvalue - inf_dm.pvalue) < 1e-12, "dm_test bridge pvalue mismatch");

    std::cout << "ts_hypothesis_bridge_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "ts_hypothesis_bridge_test failed: " << e.what() << '\n';
    return 1;
  }
}

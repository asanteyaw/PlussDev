#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <cmath>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

bool approx_vec(const std::vector<double>& a, const std::vector<double>& b, double tol = 1e-8) {
  if (a.size() != b.size()) return false;
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (std::abs(a[i] - b[i]) > tol) return false;
  }
  return true;
}

bool throws(const auto& fn) {
  try {
    fn();
  } catch (...) {
    return true;
  }
  return false;
}

bool run_module_snapshot_checks(std::ostream* err = nullptr) {
  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  pluss::nn::Fusion layer(3, 2, true, cfg, 17ULL);
  const Series x = pluss::make_series(std::vector<double>{
                                          0.4, -0.1, 0.7,
                                          1.0, 0.2, -0.5},
                                      {2, 3},
                                      cfg);

  const Series y_before = layer->forward(x);
  const auto snap = layer->snapshot();

  layer->weight() = pluss::series::zeros(layer->weight().sizes(), cfg);
  layer->bias() = pluss::series::zeros(layer->bias().sizes(), cfg);
  const Series y_mutated = layer->forward(x);
  if (approx_vec(y_before.to_vec<double>(), y_mutated.to_vec<double>())) {
    if (err) *err << "mutation should change model output\n";
    return false;
  }

  const auto report = layer->restore(snap, true);
  if (!report.missing.empty() || !report.unexpected.empty()) {
    if (err) *err << "strict restore of matching snapshot should be clean\n";
    return false;
  }
  const Series y_after = layer->forward(x);
  if (!approx_vec(y_before.to_vec<double>(), y_after.to_vec<double>())) {
    if (err) *err << "snapshot restore should recover exact forward output\n";
    return false;
  }

  auto broken = snap;
  broken.parameters.pop_back();
  if (!throws([&] { (void)layer->restore(broken, true); })) {
    if (err) *err << "strict restore should fail on missing entries\n";
    return false;
  }

  auto noisy = snap;
  noisy.parameters.push_back(pluss::nn::Module::SnapshotEntry{
      .name = "ghost.weight",
      .value = pluss::series::zeros({1}, cfg),
  });
  const auto non_strict = layer->restore(noisy, false);
  if (non_strict.unexpected.empty()) {
    if (err) *err << "non-strict restore should report unexpected entries\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("module snapshot and restore roundtrip") {
  REQUIRE(run_module_snapshot_checks(&std::cerr));
}
#else
int main() {
  if (!run_module_snapshot_checks(&std::cerr)) return 1;
  std::cout << "module_snapshot_test passed\n";
  return 0;
}
#endif

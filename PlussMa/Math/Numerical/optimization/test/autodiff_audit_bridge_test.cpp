#include <pluss/subsp/math/optim.h>

#include <cmath>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::pCPU;
using pluss::subsp::math::optim::CurvatureAuditOptions;
using pluss::subsp::math::optim::CurvatureAuditReport;
using pluss::subsp::math::optim::GradientAuditOptions;
using pluss::subsp::math::optim::GradientAuditReport;
using pluss::subsp::math::optim::HvpOptions;
using pluss::subsp::math::optim::curvature_audit;
using pluss::subsp::math::optim::curvature_audit_report;
using pluss::subsp::math::optim::gradient_audit;
using pluss::subsp::math::optim::gradient_audit_report;

static_assert(std::is_same_v<GradientAuditOptions, CurvatureAuditOptions>,
              "optim audit options should share a common options type");
static_assert(std::is_same_v<GradientAuditReport, CurvatureAuditReport>,
              "optim audit reports should share a common report type");

void expect(bool cond, const std::string& msg, int* failures) {
  if (!cond) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << '\n';
  }
}

} // namespace

int main() {
  int failures = 0;

  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU);
  const Series x = pluss::make_series(std::vector<double>{0.3, -0.2, 0.5}, {3}, cfg);

  GradientAuditOptions good{};
  good.epsilon = 1e-6;
  good.atol = 1e-6;
  good.rtol = 1e-4;

  const bool grad_ok = gradient_audit(
      [](const auto& z) {
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        return sum(sin(z) + square(z));
      },
      x,
      good);
  expect(grad_ok, "optim bridge gradient_audit should pass smooth objective", &failures);

  CurvatureAuditOptions good_curv = good;
  good_curv.hvp.mode = HvpOptions::Mode::ExactOnly;
  const bool curv_ok = curvature_audit(
      [](const auto& z) {
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        return sum(sin(z) + square(z));
      },
      x,
      good_curv);
  expect(curv_ok, "optim bridge curvature_audit should pass smooth objective", &failures);

  GradientAuditOptions strict{};
  strict.epsilon = 0.2;
  strict.atol = 0.0;
  strict.rtol = 0.0;
  const GradientAuditReport grad_report = gradient_audit_report(
      [](const auto& z) {
        using pluss::subsp::math::diff::exp;
        using pluss::subsp::math::diff::sum;
        return sum(exp(z));
      },
      x,
      strict);
  expect(!grad_report.passed, "strict gradient_audit_report should fail for coarse finite differences", &failures);
  expect(grad_report.phase == "gradient", "gradient_audit_report phase mismatch", &failures);
  expect(grad_report.lane_index >= 0, "gradient_audit_report should record failing lane", &failures);
  expect(std::isfinite(grad_report.analytic) && std::isfinite(grad_report.numeric),
         "gradient_audit_report should record finite analytic/numeric values",
         &failures);

  const CurvatureAuditReport curv_report = curvature_audit_report(
      [](const auto& z) {
        using pluss::subsp::math::diff::exp;
        using pluss::subsp::math::diff::sum;
        return sum(exp(z));
      },
      x,
      strict);
  expect(!curv_report.passed, "strict curvature_audit_report should fail for coarse finite differences", &failures);
  expect(curv_report.phase == "curvature", "curvature_audit_report phase mismatch", &failures);
  expect(curv_report.lane_index >= 0, "curvature_audit_report should record failing lane", &failures);
  expect(std::isfinite(curv_report.analytic) && std::isfinite(curv_report.numeric),
         "curvature_audit_report should record finite analytic/numeric values",
         &failures);

  for (int trial = 0; trial < 5; ++trial) {
    const double shift = 0.15 * static_cast<double>(trial);
    const Series xs = pluss::make_series(
        std::vector<double>{0.2 + shift, -0.3 + 0.5 * shift, 0.4 - 0.25 * shift},
        {3},
        cfg);

    GradientAuditOptions gopt{};
    gopt.epsilon = 1e-6;
    gopt.atol = 2e-6;
    gopt.rtol = 2e-4;
    const bool g_ok = gradient_audit(
        [](const auto& z) {
          using pluss::subsp::math::diff::exp;
          using pluss::subsp::math::diff::sin;
          using pluss::subsp::math::diff::square;
          using pluss::subsp::math::diff::sum;
          return sum(square(z) + sin(z) + exp(z * 0.2));
        },
        xs,
        gopt);
    expect(g_ok, "gradient_audit stress sweep should pass", &failures);

    CurvatureAuditOptions copt = gopt;
    copt.hvp.mode = HvpOptions::Mode::ExactOnly;
    const bool c_ok = curvature_audit(
        [](const auto& z) {
          using pluss::subsp::math::diff::exp;
          using pluss::subsp::math::diff::sin;
          using pluss::subsp::math::diff::square;
          using pluss::subsp::math::diff::sum;
          return sum(square(z) + sin(z) + exp(z * 0.2));
        },
        xs,
        copt);
    expect(c_ok, "curvature_audit stress sweep should pass", &failures);
  }

  if (failures != 0) {
    std::cerr << "autodiff_audit_bridge_test failed with " << failures << " failure(s)\n";
    return 1;
  }

  std::cout << "autodiff_audit_bridge_test passed\n";
  return 0;
}

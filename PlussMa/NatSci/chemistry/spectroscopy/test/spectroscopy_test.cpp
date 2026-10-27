#include "../../kinetics/test/h2o2_mechanism.h"
#include "spectroscopy_reference.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace chem = pluss::subsp::natsci::chemistry;
namespace sp = chem::spectroscopy;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const double *p, int n) {
  return pluss::make_series(std::vector<double>(p, p + n), {n}).to(F64);
}
Series one(double v) { return pluss::series::make({v}).to(F64); }

void test_faddeeva_vs_scipy() {
  const Series x = vec(spectroscopy_reference::w_x, 84),
               y = vec(spectroscopy_reference::w_y, 84);
  const auto w = sp::faddeeva(x, y);
  double worst = 0;
  for (int i = 0; i < 84; ++i) {
    const double re = w.real.select(0, i).item<double>(),
                 im = w.imag.select(0, i).item<double>();
    worst = std::max({worst, std::abs(re - spectroscopy_reference::w_re[i]),
                      std::abs(im - spectroscopy_reference::w_im[i])});
  }
  expect(worst < 1e-10,
         "faddeeva vs scipy wofz, worst abs " + std::to_string(worst));
}

void test_voigt_vs_scipy_and_limits() {
  const Series xv = vec(spectroscopy_reference::v_x, 41);
  double worst = 0;
  for (int k = 0; k < 5; ++k) {
    const Series v =
        sp::voigt(xv, one(0.0), one(spectroscopy_reference::v_sigma[k]),
                  one(spectroscopy_reference::v_gamma[k]));
    for (int i = 0; i < 41; ++i)
      worst = std::max(worst,
                       std::abs(v.select(0, i).item<double>() -
                                spectroscopy_reference::v_profile[k * 41 + i]));
  }
  expect(worst < 1e-10, "voigt vs scipy, worst abs " + std::to_string(worst));
  const double d = sp::voigt(xv, one(0.0), one(1.3), one(0.0))
                       .sub(sp::gaussian(xv, one(0.0), one(1.3)))
                       .abs()
                       .max()
                       .item<double>();
  expect(d < 1e-12, "voigt(gamma=0) != gaussian");
  const double dx = 0.01;
  const Series xw = pluss::series::linspace(-4000.0, 4000.0, 800001, F64);
  const double area =
      sp::voigt(xw, one(0.0), one(1.0), one(0.5)).sum().item<double>() * dx;
  expect(std::abs(area - 1.0) < 3e-4, "voigt area " + std::to_string(area));
}

void test_beer_lambert() {
  const Series eps = pluss::series::make({100.0, 50.0}).to(F64);
  const Series c =
      pluss::make_series(std::vector<double>{0.01, 0.02, 0.0, 0.04}, {2, 2})
          .to(F64);
  const Series a = sp::absorbance(eps, c, 2.0);
  expect(std::abs(a.select(0, 0).item<double>() - 4.0) < 1e-12 &&
             std::abs(a.select(0, 1).item<double>() - 4.0) < 1e-12,
         "absorbance");
  expect(std::abs(sp::transmittance(a).select(0, 0).item<double>() - 1e-4) <
             1e-12,
         "transmittance");
}

// Statistical-mechanics cp/R of N2 against the NASA-7 fit (N2 is species 9 in
// the H2/O2 mechanism).
void test_cp_vs_nasa() {
  chem::kinetics::Mechanism mech(h2o2::species(), h2o2::reactions(),
                                 pluss::series::make({300.0}).to(F64),
                                 chem::kinetics::Units::cgs_cal());
  const Series t = pluss::series::make({300.0, 600.0, 1000.0, 1500.0}).to(F64);
  const Series nasa = mech.thermo().eval(t).cp_r.select(-1, 9);
  sp::Diatomic n2{2358.57, 14.324, 1.998241, 0.017318, 2.0, 1.0};
  const Series cp = sp::cp_over_r(n2, t);
  for (int i = 0; i < 4; ++i) {
    const double rel = std::abs(cp.select(0, i).item<double>() /
                                    nasa.select(0, i).item<double>() -
                                1.0);
    expect(rel < 0.01, "N2 cp/R rel error at T index " + std::to_string(i) +
                           ": " + std::to_string(rel));
  }
  expect(std::abs(sp::rotational(n2, one(1000.0)).var.host<double>().front() - 1.0) <
             2e-3,
         "rotational cv/R -> 1");
  expect(sp::vibrational(n2, one(300.0)).var.host<double>().front() < 3e-3,
         "vibration frozen at 300 K");
  const double kt_hc =
      sp::boltzmann() * 1000.0 / (sp::planck() * sp::speed_of_light() * 100.0);
  const double q = sp::rotational(n2, one(1000.0), 0, 400).q.host<double>().front();
  // Mulholland expansion q = (kT/sigma hcB)(1 + theta/3T + ...) with B = B_0 =
  // Be - alphae/2
  const double b0 = n2.be - 0.5 * n2.alphae;
  expect(std::abs(q / (kt_hc / (2.0 * b0) * (1.0 + b0 / (3.0 * kt_hc))) - 1.0) <
             1e-4,
         "q_rot classical limit");
}
} // namespace

int main() {
  try {
    test_faddeeva_vs_scipy();
    test_voigt_vs_scipy_and_limits();
    test_beer_lambert();
    test_cp_vs_nasa();
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  std::cout << "spectroscopy tests passed\n";
  return 0;
}

// The generic two-dimensional ADI solver on a problem that has nothing to do with Heston: an option to exchange one lognormal asset for
// another, whose European price is known exactly (Margrabe).
#include "../pde2d_internal.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

namespace pde2d = pluss::subsp::math::finance::internal::pde2d;

void expect(bool cond, const std::string& msg) {
  if (!cond) throw std::runtime_error(msg);
}

struct Exchange {
  double s1 = 100.0, s2 = 95.0, sigma1 = 0.3, sigma2 = 0.2, rho = 0.5, q1 = 0.02, q2 = 0.01, r = 0.03, maturity = 1.0, top = 400.0;

  // European price (Margrabe): an option to give up S2 for S1
  double margrabe() const {
    const auto cdf = [](double x) { return 0.5 * std::erfc(-x / std::sqrt(2.0)); };
    const double var = sigma1 * sigma1 + sigma2 * sigma2 - 2.0 * rho * sigma1 * sigma2, sd = std::sqrt(var * maturity);
    const double d1 = (std::log(s1 / s2) + (q2 - q1) * maturity) / sd + 0.5 * sd, d2 = d1 - sd;
    return s1 * std::exp(-q1 * maturity) * cdf(d1) - s2 * std::exp(-q2 * maturity) * cdf(d2);
  }

  pde2d::Problem problem(std::size_t nodes) const {
    pde2d::Problem p;
    p.x = pde2d::clustered_grid(0.0, top, s1, 40.0, nodes);
    p.y = pde2d::clustered_grid(0.0, top, s2, 40.0, nodes);
    p.maturity = maturity;
    const Exchange e = *this;
    p.generator = [e](double x, double y) {
      return pde2d::Coefficients{.a11 = e.sigma1 * e.sigma1 * x * x, .a12 = e.rho * e.sigma1 * e.sigma2 * x * y, .a22 = e.sigma2 * e.sigma2 * y * y,
                                 .b1 = (e.r - e.q1) * x, .b2 = (e.r - e.q2) * y, .c = e.r};
    };
    p.payoff = [](double x, double y) { return std::max(x - y, 0.0); };
    p.x_low = pde2d::Edge::Degenerate;   // S1 = 0: every coefficient with an S1 factor vanishes
    p.y_low = pde2d::Edge::Degenerate;
    p.x_high = pde2d::Edge::Dirichlet;   // S1 far above S2: the option is worth the discounted forward difference
    p.x_high_value = [e](double tau, double y) { return e.top * std::exp(-e.q1 * tau) - y * std::exp(-e.q2 * tau); };
    p.y_high = pde2d::Edge::Dirichlet;   // S2 far above S1: worthless
    p.y_high_value = [](double, double) { return 0.0; };
    return p;
  }

  double price(std::size_t nodes, std::int64_t steps, pde2d::Exercise exercise = pde2d::Exercise::None) const {
    pde2d::Solver solver(problem(nodes));
    pde2d::Options o;
    o.steps = steps;
    o.exercise = exercise;
    return solver.interpolate(solver.solve(o), s1, s2);
  }
};

void test_exchange_option_against_margrabe() {
  const Exchange e;
  const double exact = e.margrabe();
  const double coarse = std::abs(e.price(60, 100) - exact), mid = std::abs(e.price(120, 100) - exact), fine = std::abs(e.price(240, 100) - exact);
  expect(fine < 2e-3, "exchange option on 240 x 240 nodes: error " + std::to_string(fine) + " (exact " + std::to_string(exact) + ")");
  expect(mid < coarse && fine < mid, "the error falls as the grid is refined: " + std::to_string(coarse) + ", " + std::to_string(mid) + ", " + std::to_string(fine));
  expect(coarse / fine > 8.0, "and by about a factor of four per doubling: " + std::to_string(coarse / fine));
  // other correlations, including a negative one, exercise the mixed derivative
  for (const double rho : {-0.6, 0.0, 0.8}) {
    Exchange f;
    f.rho = rho;
    expect(std::abs(f.price(320, 100) - f.margrabe()) < 5e-4, "exchange option, correlation " + std::to_string(rho));  // strong correlation needs the finer grid: the kink lies along the diagonal
  }
}

void test_exercise_constraint() {
  Exchange no_dividend;  // S1 pays nothing: in units of S2 the right to buy S1 is a call on a non-dividend asset, which is never exercised early
  no_dividend.q1 = 0.0;
  const double european = no_dividend.price(120, 100), american = no_dividend.price(120, 100, pde2d::Exercise::American);
  expect(std::abs(american - european) < 1e-3, "American = European when the received asset pays no dividend: " + std::to_string(american) + " vs " + std::to_string(european));
  Exchange dividend;  // a dividend on S1 makes early exercise valuable
  dividend.q1 = 0.08;
  const double e2 = dividend.price(120, 100), a2 = dividend.price(120, 100, pde2d::Exercise::American);
  expect(a2 > e2 + 0.05 && a2 >= std::max(dividend.s1 - dividend.s2, 0.0), "early exercise adds value when S1 pays a dividend: " + std::to_string(a2) + " vs " + std::to_string(e2));
}

void test_misuse_is_refused() {
  Exchange e;
  auto p = e.problem(40);
  p.y_low_value = nullptr;
  p.y_low = pde2d::Edge::Dirichlet;  // a Dirichlet edge needs a value
  bool refused = false;
  try {
    pde2d::Solver s(p);
  } catch (const std::exception& ex) {
    refused = std::string(ex.what()).find("needs a value function") != std::string::npos;
  }
  expect(refused, "a Dirichlet edge without a value is refused");
  auto q = e.problem(40);
  q.generator = [e](double x, double y) {
    return pde2d::Coefficients{.a11 = x * x, .a22 = y * y, .b1 = -(e.r - e.q1) * x - 1.0, .b2 = (e.r - e.q2) * y, .c = e.r};
  };  // the drift in x points out of the domain at x = 0
  refused = false;
  try {
    pde2d::Solver s(q);
  } catch (const std::exception& ex) {
    refused = std::string(ex.what()).find("points out of the domain") != std::string::npos;
  }
  expect(refused, "a Degenerate edge with outward drift is refused");
}

}  // namespace

int main() {
  try {
    test_exchange_option_against_margrabe();
    test_exercise_constraint();
    test_misuse_is_refused();
    std::cout << "pde2d_test passed\n";
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "pde2d_test failed: " << ex.what() << '\n';
    return 1;
  }
}

// Dual values are prices. For the workshop of the first program, the multiplier of a row is the change in the optimal profit
// per extra unit of that row's right-hand side (here, per extra hour of the machine). A machine with idle hours has price zero
// (complementary slackness: a limit that does not bind is worth nothing); a machine that is fully used has a positive price.
// The program prints, for each machine, the hours left over at the optimum, the dual value the solver reports, and the price
// found independently by re-solving with the hours changed by plus and minus 0.01 and dividing the change in profit by 0.02.
#include <pluss/ma.h>

#include <iomanip>
#include <cmath>
#include <iostream>

namespace lp = pluss::subsp::math::linprog;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

lp::Problem workshop(std::vector<double> hours) {
  lp::Problem p;
  p.c = vec({-3.0, -5.0});
  p.a_ub = mat({1, 0, 0, 2, 3, 2}, 3, 2);
  p.b_ub = vec(hours);
  p.lower = vec({0.0, 0.0});
  return p;
}

// Round-off of an interior-point answer can print as -0.000; show it as zero.
double clean(double v) { return std::abs(v) < 5e-4 ? 0.0 : v; }

int main() {
  const std::vector<double> hours = {4.0, 12.0, 18.0};
  const auto base = lp::solve(workshop(hours));
  const auto x = base.x.to_vec<double>();
  const auto dual = base.dual_ub.to_vec<double>();
  const std::vector<double> used = {x[0], 2.0 * x[1], 3.0 * x[0] + 2.0 * x[1]};

  std::cout << std::fixed << std::setprecision(3);
  std::cout << "optimal profit " << -base.objective.item<double>() << " at (" << x[0] << ", " << x[1] << ")\n\n";
  std::cout << std::left << std::setw(11) << "machine" << std::setw(12) << "idle hours" << std::setw(14) << "dual value" << std::setw(16) << "price in profit" << "price by re-solving\n";
  const char* names[] = {"A", "B", "C"};
  for (std::size_t i = 0; i < 3; ++i) {
    auto more = hours, fewer = hours;
    more[i] += 0.01;
    fewer[i] -= 0.01;
    const double up = -lp::solve(workshop(more)).objective.item<double>();
    const double down = -lp::solve(workshop(fewer)).objective.item<double>();
    std::cout << std::left << std::setw(11) << names[i] << std::setw(12) << hours[i] - used[i] << std::setw(14) << clean(dual[i]) << std::setw(16) << clean(-dual[i]) << clean((up - down) / 0.02) << "\n";
  }
  std::cout << "\nstrong duality: primal objective " << base.objective.item<double>() << ", dual objective " << base.dual_objective.item<double>() << "\n";
  return 0;
}

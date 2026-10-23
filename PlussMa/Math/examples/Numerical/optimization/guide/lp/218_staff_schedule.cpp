// A weekly staff schedule. Each employee works five consecutive days and then has two days off, and the shift can start on any
// day of the week (the week wraps around). The number of people needed each day is known. How many people should start on each day
// so that every day is covered, with the fewest employees in total? The variables are the starts s_0 ... s_6, non-negative
// integers; day d is covered by everyone who started on any of the five days ending at d. The program solves the model with both
// integer solvers and checks the plan day by day. It then proves the answer optimal without either solver: if fewer employees
// could cover the week, some assignment of exactly one fewer employee would, because an extra employee never hurts, and there are
// only a few hundred thousand ways to split that many employees over seven start days, so every one of them is tried.
#include <pluss/ma.h>

#include <functional>
#include <iomanip>
#include <iostream>

namespace lp = pluss::subsp::math::lp;
namespace milp = pluss::subsp::math::milp;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

int main() {
  const std::vector<double> need = {17, 13, 15, 19, 14, 16, 11};  // Monday to Sunday
  const char* names[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
  const int days = 7, length = 5;

  milp::Problem problem;
  problem.lp.c = vec(std::vector<double>(days, 1.0));
  std::vector<double> cover(static_cast<std::size_t>(days * days), 0.0);
  for (int d = 0; d < days; ++d)
    for (int k = 0; k < length; ++k) cover[static_cast<std::size_t>(d * days + (d - k + days) % days)] = -1.0;  // -(starts covering day d) <= -need
  problem.lp.a_ub = mat(cover, days, days);
  std::vector<double> rhs;
  for (const double v : need) rhs.push_back(-v);
  problem.lp.b_ub = vec(rhs);
  problem.lp.lower = vec(std::vector<double>(days, 0.0));
  problem.lp.upper = vec(std::vector<double>(days, 30.0));
  problem.integer = vec(std::vector<double>(days, 1.0));

  const auto dense = milp::solve(problem);
  const auto sparse = lp::solve_sparse(problem);
  std::cout << std::fixed << std::setprecision(0);
  std::cout << "dense branch and bound: " << dense.objective.item<double>() << " employees, " << dense.nodes << " nodes\n";
  std::cout << "sparse branch and cut:  " << sparse.objective << " employees, " << sparse.nodes << " nodes\n\n";

  const auto starts = dense.x.to_vec<double>();
  std::cout << std::left << std::setw(6) << "day" << std::setw(8) << "starts" << std::setw(10) << "working" << "needed\n";
  bool covered = true;
  for (int d = 0; d < days; ++d) {
    double working = 0.0;
    for (int k = 0; k < length; ++k) working += starts[static_cast<std::size_t>((d - k + days) % days)];
    covered = covered && working >= need[static_cast<std::size_t>(d)];
    std::cout << std::left << std::setw(6) << names[d] << std::setw(8) << starts[static_cast<std::size_t>(d)] << std::setw(10) << working << need[static_cast<std::size_t>(d)] << "\n";
  }
  // Every way to split (optimum - 1) employees over the seven start days.
  const int fewer = static_cast<int>(dense.objective.item<double>()) - 1;
  long long tried = 0, feasible = 0;
  std::vector<int> split(days, 0);
  const auto covers = [&](const std::vector<int>& st) {
    for (int d = 0; d < days; ++d) {
      int working = 0;
      for (int k = 0; k < length; ++k) working += st[static_cast<std::size_t>((d - k + days) % days)];
      if (working < need[static_cast<std::size_t>(d)]) return false;
    }
    return true;
  };
  const std::function<void(int, int)> enumerate = [&](int day, int left) {
    if (day == days - 1) {
      split[static_cast<std::size_t>(day)] = left;
      ++tried;
      if (covers(split)) ++feasible;
      return;
    }
    for (int k = 0; k <= left; ++k) {
      split[static_cast<std::size_t>(day)] = k;
      enumerate(day + 1, left - k);
    }
  };
  enumerate(0, fewer);
  std::cout << std::boolalpha << "\nevery day covered: " << covered << "\nwith " << fewer << " employees: " << tried << " splits tried, " << feasible
            << " cover the week, so " << static_cast<int>(dense.objective.item<double>()) << " is optimal\n";
  return 0;
}

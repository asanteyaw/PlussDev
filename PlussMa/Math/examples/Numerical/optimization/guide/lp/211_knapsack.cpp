// An integer program with an answer you can verify. Ten projects each have a cost and a payoff, and a budget of 40 is available.
// Choose the projects (each is taken whole or not at all) to maximize the total payoff: maximize payoff . x subject to
// cost . x <= 40 with every x_j in {0, 1}. The solver minimizes, so the payoff is negated, and the integer variables are marked in
// `integer` with finite bounds (0 and 1). The program compares the branch-and-bound answer with a brute-force search of all
// 2^10 = 1024 subsets, and shows the answer of the linear relaxation (the same program with the integrality dropped), which is
// better than any whole selection and takes a fraction of one project.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace lp = pluss::subsp::math::linprog;
namespace milp = pluss::subsp::math::milp;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

int main() {
  const std::vector<double> cost = {12, 7, 11, 8, 9, 6, 14, 5, 10, 13};
  const std::vector<double> payoff = {24, 13, 23, 15, 16, 11, 28, 8, 19, 25};
  const double budget = 40.0;
  const int n = static_cast<int>(cost.size());

  milp::Problem problem;
  std::vector<double> negated;
  for (const double v : payoff) negated.push_back(-v);
  problem.lp.c = vec(negated);
  problem.lp.a_ub = mat(cost, 1, n);
  problem.lp.b_ub = vec({budget});
  problem.lp.lower = vec(std::vector<double>(static_cast<std::size_t>(n), 0.0));
  problem.lp.upper = vec(std::vector<double>(static_cast<std::size_t>(n), 1.0));
  problem.integer = vec(std::vector<double>(static_cast<std::size_t>(n), 1.0));  // every variable is an integer

  const auto r = milp::solve(problem);
  const auto x = r.x.to_vec<double>();
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "status " << (r.status == milp::Status::Optimal ? "Optimal" : "other") << ", payoff " << -r.objective.item<double>() << ", " << r.nodes << " nodes\nchosen:";
  double spent = 0.0;
  for (int j = 0; j < n; ++j)
    if (x[static_cast<std::size_t>(j)] > 0.5) { std::cout << " " << j; spent += cost[static_cast<std::size_t>(j)]; }
  std::cout << "   (cost " << spent << " of " << budget << ")\n";

  double best = 0.0;
  for (int mask = 0; mask < (1 << n); ++mask) {
    double c = 0.0, v = 0.0;
    for (int j = 0; j < n; ++j) if (mask & (1 << j)) { c += cost[static_cast<std::size_t>(j)]; v += payoff[static_cast<std::size_t>(j)]; }
    if (c <= budget && v > best) best = v;
  }
  std::cout << "brute force over 1024 subsets: payoff " << best << "\n";

  const auto relaxation = lp::solve(problem.lp);
  std::cout << "the linear relaxation (fractions allowed): payoff " << -relaxation.objective.item<double>() << "\n";
  return 0;
}

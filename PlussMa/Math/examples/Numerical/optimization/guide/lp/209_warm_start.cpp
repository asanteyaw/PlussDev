// Restarting the simplex method from a previous answer. A cost-minimizing mix of 30 foods must supply at least the required amounts
// of 12 nutrients: minimize c . x subject to N x >= need, x >= 0, with deterministic pseudo-random data. The program solves it from
// scratch (a cold start), keeps the optimal basis the result returns, and then solves two related programs, one with a changed
// requirement and one with changed prices, starting from that basis (a warm start). `warm_started` says whether the solver really used
// the basis. The iteration counts show the saving: a small change usually needs a few pivots instead of a full solve.
#include <pluss/ma.h>

#include <algorithm>
#include <iomanip>
#include <iostream>

namespace lp = pluss::subsp::math::linprog;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

int main() {
  const int foods = 30, nutrients = 12;
  std::uint64_t state = 7;
  const auto uniform = [&] { state = state * 6364136223846793005ULL + 1442695040888963407ULL; return ((state >> 11) + 0.5) / 9007199254740992.0; };
  std::vector<double> content, price(foods), need(nutrients);
  for (int j = 0; j < nutrients; ++j)
    for (int i = 0; i < foods; ++i) content.push_back(-(1.0 + 4.0 * uniform()));  // the row is written <=, so each content is negated
  for (int i = 0; i < foods; ++i) price[static_cast<std::size_t>(i)] = 1.0 + 3.0 * uniform();
  for (int j = 0; j < nutrients; ++j) need[static_cast<std::size_t>(j)] = -(20.0 + 10.0 * uniform());

  const auto build = [&](const std::vector<double>& c, const std::vector<double>& b) {
    lp::Problem p;
    p.c = vec(c);
    p.a_ub = mat(content, nutrients, foods);
    p.b_ub = vec(b);
    p.lower = vec(std::vector<double>(foods, 0.0));
    return p;
  };

  lp::Options simplex;
  simplex.method = lp::Method::Simplex;
  const auto cold = lp::solve(build(price, need), simplex);

  std::cout << std::left << std::setw(34) << "solve" << std::setw(12) << "iterations" << std::setw(14) << "warm started" << "objective\n";
  std::cout << std::setw(34) << "base problem, cold start" << std::setw(12) << cold.iterations << std::setw(14) << "-" << std::fixed << std::setprecision(5)
            << cold.objective.item<double>() << "\n";

  const auto plan = cold.x.host<double>();
  const auto used_most = static_cast<std::size_t>(std::max_element(plan.begin(), plan.end()) - plan.begin());
  auto changed_needs = need;
  changed_needs[3] *= 1.5;  // 50% more of nutrient 3 is required
  changed_needs[7] *= 0.5;  // half as much of nutrient 7
  auto dearer = price;
  dearer[used_most] *= 3.0;  // the food the plan relies on most becomes three times as dear
  for (const auto& [label, c, b] : {std::tuple{"two needs changed, cold", price, changed_needs}, std::tuple{"the main food 3x dearer, cold", dearer, need}}) {
    const auto again = lp::solve(build(c, b), simplex);
    std::cout << std::left << std::setw(34) << label << std::setw(12) << again.iterations << std::setw(14) << "-" << again.objective.item<double>() << "\n";
  }
  for (const auto& [label, c, b] : {std::tuple{"two needs changed, warm", price, changed_needs}, std::tuple{"the main food 3x dearer, warm", dearer, need}}) {
    auto p = build(c, b);
    p.warm_basis = cold.basis[0];
    p.warm_at_upper = cold.at_upper[0];
    const auto again = lp::solve(p, simplex);
    std::cout << std::left << std::setw(34) << label << std::setw(12) << again.iterations << std::setw(14) << (again.warm_started[0] ? "yes" : "no") << again.objective.item<double>() << "\n";
  }
  return 0;
}

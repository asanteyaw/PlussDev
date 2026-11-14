<!-- generated from methods_warm_starts_and_sweeps.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Methods, warm starts and sweeps

[Back to Linear Programming 3: Methods, Warm Starts and Sweeps](../linprog_03_methods_warm_starts_and_sweeps.md)

Three practical questions come up once a linear program is working. *Which method?* When many plans are equally good the two methods return different ones: the simplex stops at a corner (a plan that uses as few variables as possible) and the interior-point method approaches the optimal segment from inside and converges to a point in its middle. Neither is wrong; choose by the answer you want.

*How to re-solve cheaply?* When the data change a little, the simplex method can restart from the previous optimal basis. For a diet problem with 30 foods and 12 nutrients, a changed requirement can need *zero* simplex iterations from the old basis, because moving a right-hand side does not change which constraints bind until it moves far enough. *How to solve many related programs?* A batched call solves a whole family at once: one cost vector per row, shared constraints, and the same answers as separate solves with a bounded memory use.

## Program 1: The two methods on a tie

Minimize `x0 + x1` subject to `x0 + x1 >= 1` and `0 <= x <= 1`. Every point on the segment from `(1, 0)` to `(0, 1)` is optimal.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace lp = pluss::subsp::math::linprog;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

int main() {
  lp::Problem p;
  p.c = vec({1, 1});
  p.a_ub = mat({-1, -1}, 1, 2);
  p.b_ub = vec({-1});
  p.lower = vec({0, 0});
  p.upper = vec({1, 1});

  std::cout << std::fixed << std::setprecision(4);
  for (const auto method : {lp::Method::InteriorPoint, lp::Method::Simplex}) {
    lp::Options options;
    options.method = method;
    const auto r = lp::solve(p, options);
    const auto x = r.x.host<double>();
    std::cout << std::left << std::setw(16) << (method == lp::Method::InteriorPoint ? "interior point" : "simplex") << "objective " << r.objective.item<double>()
              << "   x = (" << x[0] << ", " << x[1] << ")\n";
  }
  return 0;
}
```

Output

```text
interior point  objective 1.0000   x = (0.5000, 0.5000)
simplex         objective 1.0000   x = (1.0000, 0.0000)
```
Both methods report the objective 1. The simplex method stops at a corner, `(1, 0)`. The interior point method approaches the optimal segment from inside and converges to a point in its middle, `(0.5, 0.5)`. Neither is wrong. A corner is a plan that uses as few variables as possible, which is usually what you want to act on; a central point spreads the weight evenly, which is what you want when the optimum is a statement about a mixture. If the answer will be used as a plan, ask for the simplex method; if only the optimal value matters, use the default.

## Program 2: Restarting from a previous answer

A cost-minimizing mix of 30 foods must supply at least the required amounts of 12 nutrients. The program solves it from scratch, keeps the optimal basis, and then solves two changed versions both ways: with two requirements changed (one up by 50%, one halved), and with the food the plan uses most made three times as dear.

```cpp
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
```

Output

```text
solve                             iterations  warm started  objective
base problem, cold start          14          -             10.95053
two needs changed, cold           15          -             13.51531
the main food 3x dearer, cold     18          -             13.19959
two needs changed, warm           0           yes           13.51531
the main food 3x dearer, warm     4           yes           13.19959
```
The base problem takes 14 simplex iterations from scratch. With two requirements changed, a cold start takes 15 iterations and the warm start **zero**: the old basis is still optimal, because moving a right-hand side does not change which constraints bind until it moves far enough, and the answer is read off at once. With the main food three times as dear, a cold start takes 18 iterations and the warm start 4: a price change makes the old corner no longer the best, and the method pivots from it to the new one. `warm_started` is `yes` in both, which confirms the basis was used. If a basis cannot be used (its size does not match, or it is singular for the new data), the solver silently falls back to a cold start and `warm_started` says `no`, so check it when you count on the saving.

A warm start is for the simplex method only. Reach for it when you re-solve the same model many times with small changes, as in a scenario study, a rolling plan or a branch-and-bound search.

## Program 3: Many programs in one call

A plan chooses quantities of four activities, each with a cost and an emission, to meet a demand of 10 units exactly, with no activity above 6. Minimizing `(1 - t) cost + t emission` for `t = 0, 0.25, 0.5, 0.75, 1` gives five programs. `c` has shape `[5, 4]`, and the call solves them together.

```cpp
#include <pluss/ma.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

namespace lp = pluss::subsp::math::linprog;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

int main() {
  const std::vector<double> cost = {1.0, 2.0, 3.0, 5.0}, emission = {9.0, 6.0, 3.0, 1.0};
  const std::vector<double> weights = {0.0, 0.25, 0.5, 0.75, 1.0};
  const int lanes = static_cast<int>(weights.size());

  std::vector<double> c;
  for (const double t : weights)
    for (int i = 0; i < 4; ++i) c.push_back((1.0 - t) * cost[static_cast<std::size_t>(i)] + t * emission[static_cast<std::size_t>(i)]);

  lp::Problem batch;
  batch.c = mat(c, lanes, 4);                    // [5, 4]: one objective per lane
  batch.a_eq = mat({1, 1, 1, 1}, 1, 4);          // the demand is met exactly
  batch.b_eq = vec({10.0});
  batch.lower = vec({0, 0, 0, 0});
  batch.upper = vec({6, 6, 6, 6});               // no activity can supply more than 6

  lp::Options simplex;
  simplex.method = lp::Method::Simplex;          // vertices, so that the plans are easy to read
  const auto result = lp::solve(batch, simplex);
  const auto x = result.x.host<double>();      // [5, 4], row-major
  const auto status = result.status.host<double>();

  std::cout << std::fixed << std::setprecision(2) << std::left << std::setw(6) << "t" << std::setw(10) << "status" << std::setw(26) << "plan" << std::setw(8) << "cost" << "emission\n";
  double worst = 0.0;
  for (int l = 0; l < lanes; ++l) {
    double total_cost = 0.0, total_emission = 0.0;
    std::ostringstream plan;
    for (int i = 0; i < 4; ++i) {
      const double q = x[static_cast<std::size_t>(l * 4 + i)];
      plan << q << " ";
      total_cost += cost[static_cast<std::size_t>(i)] * q;
      total_emission += emission[static_cast<std::size_t>(i)] * q;
    }
    std::cout << std::left << std::setw(6) << weights[static_cast<std::size_t>(l)] << std::setw(10) << (status[static_cast<std::size_t>(l)] == 0.0 ? "Optimal" : "other") << std::setw(26) << plan.str()
              << std::setw(8) << total_cost << total_emission << "\n";

    lp::Problem single = batch;  // the same lane solved alone
    single.c = vec(std::vector<double>(c.begin() + l * 4, c.begin() + (l + 1) * 4));
    worst = std::max(worst, std::abs(lp::solve(single, simplex).objective.item<double>() - result.objective.host<double>()[static_cast<std::size_t>(l)]));
  }
  lp::Options chunked = simplex;
  chunked.max_lanes = 2;
  const auto again = lp::solve(batch, chunked);
  const auto a = again.objective.host<double>(), b = result.objective.host<double>();
  double diff = 0.0;
  for (std::size_t l = 0; l < a.size(); ++l) diff = std::max(diff, std::abs(a[l] - b[l]));
  std::cout << std::scientific << std::setprecision(1) << "\nlargest difference between the batch and separate solves: " << worst
            << "\nlargest difference with max_lanes = 2: " << diff << "\n";
  return 0;
}
```

Output

```text
t     status    plan                      cost    emission
0.00  Optimal   6 4 0 0                   14.00   78.00
0.25  Optimal   6 4 0 0                   14.00   78.00
0.50  Optimal   0 0 6 4                   38.00   22.00
0.75  Optimal   0 0 4 6                   42.00   18.00
1.00  Optimal   0 0 4 6                   42.00   18.00

largest difference between the batch and separate solves: 0.0e+00
largest difference with max_lanes = 2: 0.0e+00
```
The table traces the trade-off. For `t` of 0 and 0.25 the cheapest plan wins: 6 units of activity 0 and 4 of activity 1, at cost 14 and emission 78. At `t = 0.5` the plan switches to activities 2 and 3, at cost 38 and emission 22, and at `t = 0.75` and 1 to the cleanest, 4 and 6 units, at cost 42 and emission 18. The plan changes only at certain weights and is constant between them, which is how a linear program behaves: the optimal corner changes at a breakpoint of the weight, not smoothly.

The batch gives exactly the answers of five separate solves (the largest difference is 0), and limiting `max_lanes` to 2 gives exactly the same answers again: it changes how much memory the call needs, not what it returns. The bound *values* may differ between lanes, but the pattern of which bounds are finite must be the same in every lane.

## Decisions and pitfalls

- **Ties are resolved differently.** If you need a vertex solution (few non-zero variables), use the simplex; if you want the symmetric centre of the optimal set, the interior-point method.
- **A warm start is for the simplex method only.** Use it when you re-solve the same model many times with small changes: a scenario study, a rolling plan, a branch-and-bound search.
- **A warm start from a basis that is no longer feasible costs iterations.** Large changes in the data can invalidate the old basis; the benefit grows with how close the new problem is.
- **Batches give exactly the answers of separate solves.** `max_lanes` changes the memory the call needs, not what it returns.

Where next: [Chapter 4](../linprog_04_large_sparse_models.md) is for large, sparse models.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->

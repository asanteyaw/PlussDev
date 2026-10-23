<!-- generated from many_programs_in_one_call.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Many programs in one call

[Back to Methods, Warm Starts and Sweeps](../linprog_03_methods_warm_starts_and_sweeps.md)


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
  const auto x = result.x.to_vec<double>();      // [5, 4], row-major
  const auto status = result.status.to_vec<double>();

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
    worst = std::max(worst, std::abs(lp::solve(single, simplex).objective.item<double>() - result.objective.to_vec<double>()[static_cast<std::size_t>(l)]));
  }
  lp::Options chunked = simplex;
  chunked.max_lanes = 2;
  const auto again = lp::solve(batch, chunked);
  const auto a = again.objective.to_vec<double>(), b = result.objective.to_vec<double>();
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

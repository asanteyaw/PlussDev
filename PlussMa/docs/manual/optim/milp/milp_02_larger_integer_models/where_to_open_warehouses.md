<!-- generated from where_to_open_warehouses.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Where to open warehouses

[Back to Larger Models and Worked Problems](../milp_02_larger_integer_models.md)


Four sites could be opened, each with a fixed cost and a capacity. Six customers need known amounts, and shipping costs a given amount per unit from each site to each customer. Choose which sites to open and how to ship, at least total cost. Each site's open or closed decision is a 0/1 variable `y`; the shipments `x` are continuous; and a row per site says that what it ships out is at most its capacity times `y`, which forces a closed site to ship nothing. This mix of a few integer variables and many continuous ones is typical.

```cpp
#include <pluss/ma.h>

#include <algorithm>
#include <iomanip>
#include <iostream>

namespace lp = pluss::subsp::math::lp;
namespace dense = pluss::subsp::math::linprog;
namespace milp = pluss::subsp::math::milp;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

int main() {
  const int F = 4, C = 6;
  const std::vector<double> fixed_cost = {60, 45, 70, 50}, capacity = {55, 40, 70, 45};
  const std::vector<double> demand = {12, 18, 15, 20, 10, 16};
  const std::vector<double> ship = {2, 4, 5, 6, 3, 7,   // per unit, site 0 to each customer
                                    6, 2, 3, 5, 7, 4,
                                    5, 5, 2, 3, 4, 2,
                                    7, 6, 4, 2, 5, 3};
  // variables: y[0..F-1] (open), then x[f * C + c] (units shipped from site f to customer c)
  const int n = F + F * C;
  std::vector<double> cost(static_cast<std::size_t>(n), 0.0);
  for (int f = 0; f < F; ++f) cost[static_cast<std::size_t>(f)] = fixed_cost[static_cast<std::size_t>(f)];
  for (int k = 0; k < F * C; ++k) cost[static_cast<std::size_t>(F + k)] = ship[static_cast<std::size_t>(k)];

  const auto build = [&](const std::vector<int>& forced_open /* empty = y free */) {
    milp::Problem p;
    p.lp.c = vec(cost);
    // equality rows: each customer's demand is met
    std::vector<double> eq(static_cast<std::size_t>(C * n), 0.0), b_eq = demand;
    for (int c = 0; c < C; ++c)
      for (int f = 0; f < F; ++f) eq[static_cast<std::size_t>(c * n + F + f * C + c)] = 1.0;
    p.lp.a_eq = mat(eq, C, n);
    p.lp.b_eq = vec(b_eq);
    // inequality rows: shipments out of a site <= capacity * open
    std::vector<double> ub(static_cast<std::size_t>(F * n), 0.0);
    for (int f = 0; f < F; ++f) {
      for (int c = 0; c < C; ++c) ub[static_cast<std::size_t>(f * n + F + f * C + c)] = 1.0;
      ub[static_cast<std::size_t>(f * n + f)] = -capacity[static_cast<std::size_t>(f)];
    }
    p.lp.a_ub = mat(ub, F, n);
    p.lp.b_ub = vec(std::vector<double>(static_cast<std::size_t>(F), 0.0));
    std::vector<double> lower(static_cast<std::size_t>(n), 0.0), upper(static_cast<std::size_t>(n), 100.0);
    for (int f = 0; f < F; ++f) upper[static_cast<std::size_t>(f)] = 1.0;
    p.lp.lower = vec(lower);
    p.lp.upper = vec(upper);
    std::vector<double> integer(static_cast<std::size_t>(n), 0.0);
    for (int f = 0; f < F; ++f) integer[static_cast<std::size_t>(f)] = 1.0;
    p.integer = vec(integer);
    (void)forced_open;
    return p;
  };

  std::cout << std::fixed << std::setprecision(2);
  const auto report = [&](const char* label, const std::vector<double>& x, double objective, long long nodes) {
    std::cout << std::left << std::setw(26) << label << "cost " << std::setw(9) << objective << "open sites:";
    for (int f = 0; f < F; ++f) if (x[static_cast<std::size_t>(f)] > 0.5) std::cout << " " << f;
    std::cout << "   nodes " << nodes << "\n";
  };

  const auto problem = build({});
  const auto d = milp::solve(problem);
  report("dense branch and bound", d.x.to_vec<double>(), d.objective.item<double>(), d.nodes);
  const auto s = lp::solve_sparse(problem);
  report("sparse branch and cut", s.x, s.objective, s.nodes);

  // verify: every combination of open sites, with the shipping solved as a linear program
  double best = 1e300;
  int best_mask = -1;
  for (int mask = 1; mask < (1 << F); ++mask) {
    auto p = build({});
    std::vector<double> upper(static_cast<std::size_t>(n), 100.0), lower(static_cast<std::size_t>(n), 0.0);
    for (int f = 0; f < F; ++f) {
      const double y = static_cast<double>((mask >> f) & 1);
      lower[static_cast<std::size_t>(f)] = y;
      upper[static_cast<std::size_t>(f)] = y;
    }
    p.lp.lower = vec(lower);
    p.lp.upper = vec(upper);
    const auto r = dense::solve(p.lp);
    if (dense::success(static_cast<dense::Status>(static_cast<int>(r.status.item<double>()))) && r.objective.item<double>() < best) {
      best = r.objective.item<double>();
      best_mask = mask;
    }
  }
  std::cout << std::left << std::setw(26) << "all 15 combinations" << "cost " << std::setw(9) << best << "open sites:";
  for (int f = 0; f < F; ++f) if ((best_mask >> f) & 1) std::cout << " " << f;
  std::cout << "\n";
  return 0;
}
```

Output

```text
dense branch and bound    cost 376.00   open sites: 1 2   nodes 21
sparse branch and cut     cost 376.00   open sites: 1 2   nodes 5
all 15 combinations       cost 376.00   open sites: 1 2
```
Both engines find the cost 376 with sites 1 and 2 open: the dense branch-and-bound in 21 nodes, the sparse branch-and-cut in 5. The last line is the independent check. With four sites there are only 15 non-empty sets of sites to open; for each, the program fixes the `y` values and solves the remaining shipping problem as a plain linear program, and takes the best. It also finds 376 with sites 1 and 2. Three separate routes to the same number is as good a verification as an integer program allows.

This is the general way to verify a mixed model whose integer part is small: enumerate the integer part, and solve the linear part exactly.

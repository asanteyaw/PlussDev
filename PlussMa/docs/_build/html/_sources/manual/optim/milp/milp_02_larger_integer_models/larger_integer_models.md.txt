<!-- generated from larger_integer_models.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Larger integer models

[Back to Integer Programming 2: Larger Models and Worked Problems](../milp_02_larger_integer_models.md)

Real integer models are bigger, mix whole and continuous variables, and benefit from a stronger search. The sparse **branch-and-cut** solver adds *cuts* (extra constraints that remove fractional points without removing whole ones), presolve and heuristics to the branching, and often finishes at the root where the dense branch-and-bound needs dozens or hundreds of nodes. This page uses it on two complete problems, with the dense engine alongside for comparison.

The first is **facility location**: choose which of four sites to open (each with a fixed cost and a capacity) and how to ship to six customers, at least total cost. The open-or-closed decision is a 0/1 variable and shipments are continuous. The second is a **weekly staff schedule**: each employee works five consecutive days then has two off, and the number needed each day is known; how many should start on each day? Both are verified independently of the solver: by enumerating the integer part and solving the linear part exactly.

## Program 1: Where to open warehouses

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
  report("dense branch and bound", d.x.host<double>(), d.objective.item<double>(), d.nodes);
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

## Program 2: A weekly staff schedule

Each employee works five consecutive days and then has two days off, and a shift can start on any day of the week. The number of people needed each day is known: 17, 13, 15, 19, 14, 16 and 11 from Monday to Sunday. How many should start on each day so that every day is covered, with the fewest employees? The starts are non-negative integers, and day `d` is covered by everyone who started in the five days ending at `d`.

```cpp
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

  const auto starts = dense.x.host<double>();
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
```

Output

```text
dense branch and bound: 23 employees, 103 nodes
sparse branch and cut:  23 employees, 0 nodes

day   starts  working   needed
Mon   1       17        17
Tue   5       14        13
Wed   1       15        15
Thu   8       21        19
Fri   0       15        14
Sat   2       16        16
Sun   6       17        11

every day covered: true
with 22 employees: 376740 splits tried, 0 cover the week, so 23 is optimal
```
Both engines find that **23** employees are needed. The dense search takes 103 nodes; the sparse solver reports 0 nodes, meaning its presolve, cuts and heuristics finished at the root. The table checks the plan day by day: starts of 1, 5, 1, 8, 0, 2 and 6, and every day has at least the people it needs. Tuesday has 14 for a need of 13, Thursday 21 for 19, Friday 15 for 14 and Sunday 17 for 11, so there is slack, which is normal: the five-day pattern cannot match the daily needs exactly.

The final lines prove the answer optimal without either solver. If 22 employees could cover the week, then some assignment of exactly 22 would, because an extra employee never hurts. There are 376,740 ways to split 22 employees over the seven start days, and the program tries all of them: none covers the week. So 23 is optimal. Counting arguments like this one are not available for most models, but where they are, they are the strongest check.

## Decisions and pitfalls

- **Verify a mixed model by enumerating its integer part.** With four sites there are 15 non-empty sets to open; for each, solve the linear part exactly and compare.
- **The sparse solver can finish at the root.** Presolve, cuts and heuristics may leave 0 nodes to explore where the dense search needs 100.
- **A tight bound is a proof.** The staff schedule is proved optimal without either solver by showing no assignment of 22 employees covers the week.
- **Model with the smallest integer part you can.** Every 0/1 variable multiplies the search; formulate cuts and bounds tightly.

Where next: That completes integer programming. [Root finding](../../roots.md) is next.

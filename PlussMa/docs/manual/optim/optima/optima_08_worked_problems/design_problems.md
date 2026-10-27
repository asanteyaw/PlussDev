<!-- generated from design_problems.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Design problems

[Back to Optima 8: Worked Problems](../optima_08_worked_problems.md)

Design problems have a natural objective (use the least material, make the most profit) and a list of physical or business limits, which is exactly the shape `optima` is built for. Each program here follows the same checklist: state the problem, solve it, check the flag and the violation, check the answer by something the solver did not use, and read the multipliers as prices.

Three problems are covered. A can that holds a given volume with the least metal has a closed form that the solver must reproduce, which makes it a test of the whole method. The same can with catalogue limits that make it impossible shows what a failed solve looks like and how to tell it from a success. A production plant with every kind of constraint (shared resources, an exact-split rule, a pollution cap, bounds) is the realistic case, where the multipliers say what each limit is worth.

## Program 2: A design with a closed-form answer

The least metal for a cylindrical can that holds 1000 cubic centimetres. The unknowns are the radius `r` and the height `h`, named blocks. The surface area is the objective, and the volume requirement `pi r^2 h >= 1000` is a curved inequality, written `1000 - pi r^2 h <= 0`.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> rh(double r, double h) { return optim::pack_scalars({{"r", scalar(r)}, {"h", scalar(h)}}); }

int main() {
  const double pi = 3.14159265358979323846, volume = 1000.0;
  auto area = [&](const Pack& p) { return 2.0 * pi * p.at("r").square() + 2.0 * pi * p.at("r") * p.at("h"); };
  const auto can = optim::constraints()
                       .lbs(rh(0.1, 0.1))
                       .ubs(rh(50, 50))
                       .nl_ineq([&](const Pack& p) { return volume - pi * p.at("r").square() * p.at("h"); });
  const auto r = optim::optima(area, rh(3, 10), {}, can);

  const double radius = as_f64(r.params.at("r")), height = as_f64(r.params.at("h"));
  const double best_radius = std::cbrt(volume / (2.0 * pi));
  std::cout << std::fixed << std::setprecision(4) << "exit flag " << static_cast<int>(r.exit_flag) << ", " << r.iteration << " iterations\n";
  std::cout << "radius " << radius << " (closed form " << best_radius << "), height " << height << " (closed form " << 2.0 * best_radius << ")\n";
  std::cout << "area " << r.evaluation.value.item<double>() << ", volume " << pi * radius * radius * height << " (required " << volume << ")\n";
  const double price = r.multipliers.nonlinear_inequalities.host<double>()[0];
  std::cout << "multiplier " << price << " (closed form " << 2.0 * std::cbrt(2.0 * pi) / std::cbrt(volume) << ")\n";
  return 0;
}
```

Output

```text
exit flag 1, 9 iterations
radius 5.4193 (closed form 5.4193), height 10.8385 (closed form 10.8385)
area 553.5810, volume 1000.0000 (required 1000.0000)
multiplier 0.3691 (closed form 0.3691)
```
The answer is a radius of 5.4193 and a height of 10.8385, found in 9 iterations. For this problem calculus gives the answer in closed form: the height is twice the radius and `r = (V / 2 pi)^(1/3)`. The solver agrees with it to the four printed digits, the volume constraint is met with equality, and the multiplier, 0.3691, equals the closed-form value `2 (2 pi)^(1/3) V^(-1/3)`: it is the area saved per cubic centimetre of capacity given up. Problems with a known answer are worth keeping as tests of your own formulation: if your model of a larger problem reduces to one like this, solve that case first.

## Program 3: A design that cannot be built

The same can, now with a catalogue limit: the radius may not exceed 2 and the height may not exceed 20. The largest can the limits allow holds `pi * 2^2 * 20 = 251` cubic centimetres, far short of 1000. No point meets every requirement.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> rh(double r, double h) { return optim::pack_scalars({{"r", scalar(r)}, {"h", scalar(h)}}); }

void solve(const char* label, double max_height) {
  const double pi = 3.14159265358979323846, volume = 1000.0;
  auto area = [&](const Pack& p) { return 2.0 * pi * p.at("r").square() + 2.0 * pi * p.at("r") * p.at("h"); };
  const auto can = optim::constraints()
                       .lbs(rh(0.1, 0.1))
                       .ubs(rh(2.0, max_height))
                       .nl_ineq([&](const Pack& p) { return volume - pi * p.at("r").square() * p.at("h"); });
  const auto r = optim::optima(area, rh(1, 10), {}, can);
  std::cout << label << ": exit flag " << static_cast<int>(r.exit_flag) << ", max violation " << std::scientific << std::setprecision(2)
            << r.evaluation.max_constraint_violation << ", radius " << std::fixed << std::setprecision(4) << as_f64(r.params.at("r"))
            << ", height " << as_f64(r.params.at("h")) << "\n  " << r.message.substr(0, r.message.find(" diagnostic")) << "\n";
}

int main() {
  solve("height limit 20 ", 20.0);
  solve("height limit 400", 400.0);
  return 0;
}
```

Output

```text
height limit 20 : exit flag -6, max violation 7.49e+02, radius 2.0000, height 20.0000
  restoration failed to reduce constraint violation
height limit 400: exit flag 1, max violation 0.00e+00, radius 2.0000, height 79.5775
  optimality and feasibility tolerances satisfied
```
The first solve does not return an answer, and it says so in three ways: exit flag `-6`, a maximum violation of 749 (the 1000 required minus the 251 reached), and the message that restoration, the phase that tries to reduce the violation alone, could not reduce it. The point it returns, radius 2 and height 20, is the best the bounds permit and is not a solution. This is the case chapter 2 warned about: the returned parameters look like an answer. Only the flag and the violation tell you otherwise.

The second solve relaxes the height limit to 400 and succeeds, and it shows something worth noticing. The unrestricted can of Program 2 had a radius of 5.42, but the radius limit of 2 is still in force, so the radius sits at its bound and the height rises to 79.5775 to supply the volume. Relaxing one limit does not remove the others; the answer moves to wherever the remaining limits put it.

## Program 5: A plant with every kind of constraint

A plant makes five products. Profit from each has diminishing returns, so the objective, the negative of a weighted sum of square roots, is convex. Two shared resources limit production (linear inequalities: machine hours and labour hours), products 1 to 3 must together make exactly 30 units (a linear equality), a pollution measure is capped (a nonlinear inequality), and each quantity has a capacity (bounds). After the solve the program prints the checks of the routine.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(f64); }

int main() {
  const std::vector<double> value = {9, 7, 8, 6, 5};
  auto negative_profit = [&](const pluss::Series& q) {
    return -1.0 * (vec(value) * (q + 1.0).pow(0.5)).sum();  // diminishing returns
  };
  const auto plant =
      optim::constraints()
          .lbs(vec({0, 0, 0, 0, 0}))
          .ubs(vec({40, 40, 40, 40, 40}))
          .ln_ineq(mat({2, 1, 3, 1, 2,      // machine hours
                        1, 3, 1, 2, 1},     // labour hours
                       2, 5),
                   vec({100, 90}))
          .ln_eq(mat({1, 1, 1, 0, 0}, 1, 5), vec({30}))  // products 1-3 together make exactly 30 units
          .nl_ineq([](const Pack& p) { return p.at("x").square().sum() / 100.0 - 12.0; });  // pollution cap

  const auto r = optim::optima(negative_profit, vec({5, 5, 5, 5, 5}), {}, plant);
  const auto q = r.params.at("x").host<double>();

  std::cout << std::fixed << std::setprecision(4);
  std::cout << "quantities:";
  for (double v : q) std::cout << " " << v;
  std::cout << "\nprofit measure " << -r.evaluation.value.item<double>() << "\n\n";

  std::cout << "checks before trusting the result\n";
  std::cout << "  exit flag " << static_cast<int>(r.exit_flag) << " (1 means converged)\n";
  std::cout << std::scientific << std::setprecision(1) << "  max constraint violation " << r.evaluation.max_constraint_violation << "\n";
  std::cout << "  first-order optimality " << r.output.first_order_optimality << "\n" << std::fixed << std::setprecision(4);
  const auto machine = r.multipliers.linear_inequalities.host<double>();
  const auto pollution = r.multipliers.nonlinear_inequalities.host<double>();
  std::cout << "  multipliers: machine hours " << machine[0] << ", labour hours " << machine[1] << ", pollution cap " << pollution[0]
            << "\n  a multiplier of zero means that limit does not bind\n";
  return 0;
}
```

Output

```text
quantities: 15.7105 6.6804 7.6091 18.0691 10.5011
profit measure 122.8206

checks before trusting the result
  exit flag 1 (1 means converged)
  max constraint violation 1.1e-12
  first-order optimality 3.4e-06
  multipliers: machine hours 0.2624, labour hours 0.2123, pollution cap 0.0000
  a multiplier of zero means that limit does not bind
```
The solve converges with the quantities 15.71, 6.68, 7.61, 18.07 and 10.50, a violation of 1.1e-12 and an optimality measure of 3.4e-06, so steps 4 and 5 pass. The multipliers, step 6, tell the story of the plan. Both resource limits bind and have positive multipliers: machine hours 0.2624 and labour hours 0.2123, the extra profit measure from one more hour of each. The pollution cap has a multiplier of zero: the plant is well inside it, and loosening it would change nothing. You can confirm all three without the multipliers: the machine hours used are `2(15.7105) + 6.6804 + 3(7.6091) + 18.0691 + 2(10.5011) = 100`, the labour hours are 90, and the pollution measure, the sum of squares divided by 100, is 7.86 against a cap of 12. Products 1 to 3 sum to 30.0000. The multipliers say which limits are worth relaxing, and here they point at the two resources, not the cap.

## Decisions and pitfalls

- **Check against a closed form when you have one.** Agreement with calculus to the printed digits is the strongest check of a model.
- **An infeasible design is reported, not hidden.** The solve returns a flag, a violation and a message; the point it returns is the best the bounds permit and is not a solution.
- **Relaxing one limit does not remove the others.** Raising the height limit left the radius limit in force, and the answer moved to wherever the remaining limits allowed.
- **Multipliers are the prices of the limits.** In the plant, the resource multipliers are the extra profit from one more hour; a limit with multiplier zero is not binding and costs nothing.
- **Pass the whole checklist.** Flag, violation, optimality, a check the solver did not use, and the multipliers: a result that skips any of these is not yet an answer.

Where next: [Fitting and portfolio problems](fitting_and_portfolio_problems.md) apply the same checklist to estimation and finance.

<!-- generated from a_design_with_a_closed_form_answer.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A design with a closed-form answer

[Back to Worked Problems](../optima_08_worked_problems.md)


The least metal for a cylindrical can that holds 1000 cubic centimetres. The unknowns are the radius `r` and the height `h`, named blocks. The surface area is the objective, and the volume requirement `pi r^2 h >= 1000` is a curved inequality, written `1000 - pi r^2 h <= 0`.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> rh(double r, double h) { return optim::pack_scalars({{"r", scalar(r)}, {"h", scalar(h)}}); }

int main() {
  const double pi = 3.14159265358979323846, volume = 1000.0;
  auto area = [&](const Pack& p) { return 2.0 * pi * optim::square(p.at("r")) + 2.0 * pi * p.at("r") * p.at("h"); };
  const auto can = optim::constraints()
                       .lbs(rh(0.1, 0.1))
                       .ubs(rh(50, 50))
                       .nl_ineq([&](const Pack& p) { return volume - pi * optim::square(p.at("r")) * p.at("h"); });
  const auto r = optim::optima(area, rh(3, 10), {}, can);

  const double radius = r.params.at("r").item<double>(), height = r.params.at("h").item<double>();
  const double best_radius = std::cbrt(volume / (2.0 * pi));
  std::cout << std::fixed << std::setprecision(4) << "exit flag " << static_cast<int>(r.exit_flag) << ", " << r.iteration << " iterations\n";
  std::cout << "radius " << radius << " (closed form " << best_radius << "), height " << height << " (closed form " << 2.0 * best_radius << ")\n";
  std::cout << "area " << r.evaluation.value.item<double>() << ", volume " << pi * radius * radius * height << " (required " << volume << ")\n";
  const double price = r.multipliers.nonlinear_inequalities.to_vec<double>()[0];
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

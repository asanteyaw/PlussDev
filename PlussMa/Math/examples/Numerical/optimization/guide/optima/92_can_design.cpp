// Design a cylindrical can: the least metal that holds a given volume. The unknowns are the radius `r` and height `h`; the
// metal is the surface area 2 pi r^2 + 2 pi r h; the can must hold at least V = 1000 cubic centimetres, a curved
// inequality pi r^2 h >= V. This problem has a closed-form answer (the height is twice the radius, and r = (V / 2 pi)^(1/3)),
// so the program can say how close the solver gets, and the multiplier of the volume constraint has a closed form too:
// it is the area saved per unit of volume given up, dA/dV = 2 (2 pi)^(1/3) V^(-1/3).
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

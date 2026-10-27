// Ito's product rule is Ito's lemma for g(x, y) = x y, and it carries the covariation of the two processes:
// d(XY) = X dY + Y dX + d[X, Y]. Two assets driven by correlated Brownian motions share two factors, so the
// covariation is the dot product of their loading rows. `ito_product` and `ito_quotient` are the two named
// special cases; any other function of two processes goes through `ito` with a two-argument function.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

using ItoState = pluss::Series::ItoState;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  // Asset X loads on factor 1 only; asset Y has correlation rho with X.
  const double rho = 0.6, sx = 0.3, sy = 0.4;
  const double xv = 2.0, yv = 1.5, mx = 0.05, my = 0.08;
  const pluss::Series x = vec({xv});
  const pluss::Series mu_x = vec({mx});
  const pluss::Series sigma_x = pluss::make_series(std::vector<double>{sx, 0.0}, {1, 2}).to(f64);
  const ItoState y{vec({yv}), vec({my}),
                         pluss::make_series(std::vector<double>{rho * sy, std::sqrt(1 - rho * rho) * sy}, {1, 2}).to(f64)};

  const double cov = rho * sx * sy;  // d[X, Y] / dt
  std::cout << std::fixed << std::setprecision(5);
  const auto product = x.calc.diff.ito_product(mu_x, sigma_x, y);
  std::cout << "product:   drift " << product.drift.item<double>() << "   (X mu_y + Y mu_x + cov = "
            << xv * my + yv * mx + cov << ")\n";
  std::cout << "           diffusion (" << product.diffusion.host<double>()[0] << ", "
            << product.diffusion.host<double>()[1] << ")   (X sigma_y + Y sigma_x = (" << xv * rho * sy + yv * sx << ", "
            << xv * std::sqrt(1 - rho * rho) * sy << "))\n";

  const auto quotient = x.calc.diff.ito_quotient(mu_x, sigma_x, y);
  const double syy = sy * sy;
  std::cout << "quotient:  drift " << quotient.drift.item<double>() << "   (mu_x/Y - X mu_y/Y^2 - cov/Y^2 + X sigma_y^2/Y^3 = "
            << mx / yv - xv * my / (yv * yv) - cov / (yv * yv) + xv * syy / (yv * yv * yv) << ")\n";

  // Any other function of the two processes: the squared spread (X - Y)^2.
  const auto spread = x.calc.diff.ito(
      [](const pluss::Series& a, const pluss::Series& b) { return a.sub(b).pow(2.0); }, mu_x, sigma_x, y);
  const double var_spread = sx * sx + syy - 2 * cov;
  std::cout << "(X - Y)^2: drift " << spread.drift.item<double>() << "   (2 (X - Y)(mu_x - mu_y) + var(X - Y) = "
            << 2 * (xv - yv) * (mx - my) + var_spread << ")\n";
  return 0;
}

<!-- generated from products_and_quotients.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Products and quotients

[Back to Differentials and Ito's Lemma](../stochastic_02_ito_lemma.md)


For two processes driven by the same noise, `d(XY) = X dY + Y dX + d[X, Y]`, where `d[X, Y] = σ_x · σ_y dt` is the covariation. Quotients and any function of two processes follow from Ito's lemma in two variables. The convenience methods `ito_product` and `ito_quotient` name the two common cases; the general two-state form takes any `g(x, y)`.

```cpp
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
  std::cout << "product:   drift " << product.drift.squeeze().item<double>() << "   (X mu_y + Y mu_x + cov = "
            << xv * my + yv * mx + cov << ")\n";
  std::cout << "           diffusion (" << product.diffusion.host<double>()[0] << ", "
            << product.diffusion.host<double>()[1] << ")   (X sigma_y + Y sigma_x = (" << xv * rho * sy + yv * sx << ", "
            << xv * std::sqrt(1 - rho * rho) * sy << "))\n";

  const auto quotient = x.calc.diff.ito_quotient(mu_x, sigma_x, y);
  const double syy = sy * sy;
  std::cout << "quotient:  drift " << quotient.drift.squeeze().item<double>() << "   (mu_x/Y - X mu_y/Y^2 - cov/Y^2 + X sigma_y^2/Y^3 = "
            << mx / yv - xv * my / (yv * yv) - cov / (yv * yv) + xv * syy / (yv * yv * yv) << ")\n";

  // Any other function of the two processes: the squared spread (X - Y)^2.
  const auto spread = x.calc.diff.ito(
      [](const pluss::Series& a, const pluss::Series& b) { return a.sub(b).pow(2.0); }, mu_x, sigma_x, y);
  const double var_spread = sx * sx + syy - 2 * cov;
  std::cout << "(X - Y)^2: drift " << spread.drift.squeeze().item<double>() << "   (2 (X - Y)(mu_x - mu_y) + var(X - Y) = "
            << 2 * (xv - yv) * (mx - my) + var_spread << ")\n";
  return 0;
}
```

Output

```text
product:   drift 0.30700   (X mu_y + Y mu_x + cov = 0.30700)
           diffusion (0.93000, 0.64000)   (X sigma_y + Y sigma_x = (0.93000, 0.64000))
quotient:  drift 0.02504   (mu_x/Y - X mu_y/Y^2 - cov/Y^2 + X sigma_y^2/Y^3 = 0.02504)
(X - Y)^2: drift 0.07600   (2 (X - Y)(mu_x - mu_y) + var(X - Y) = 0.07600)
```
Every drift equals its closed form: 0.30700 for the product, 0.02504 for the quotient, 0.07600 for `(X − Y)²`. The last one shows the general form: the second-order term is the variance of `X − Y`, which depends on how correlated the noises are.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="several_sources_of_noise.html">
    <span class="label">Previous</span>
    <span class="title">Several sources of noise</span>
  </a>
  <a class="next-link" href="explicit_time_dependence.html">
    <span class="label">Next</span>
    <span class="title">Explicit time dependence</span>
  </a>
</div>
<!-- apiref:nav:end -->

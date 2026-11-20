// Fit a straight line to noisy measurements by least squares, then judge the fit: coefficients, uncertainty and R squared.
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  using pluss::ScalarType;
  const std::int64_t n = 200;
  // Synthetic truth: y = 3 + 2 x plus noise of standard deviation 1.5. A fixed seed makes the run repeatable.
  auto x = pluss::series::linspace(0.0, 10.0, n, ScalarType::Float64);
  auto noise = pluss::series::randn({n}, ScalarType::Float64, 7) * 1.5;
  auto y = x * 2.0 + 3.0 + noise;

  // 1. Design matrix [1, x]: a column of ones for the intercept next to the column of x. Shape [n, 2].
  auto design = pluss::series::ones({n, 1}, ScalarType::Float64).unite({x.unsqueeze(1)}, 1);
  std::cout << "design matrix shape [" << design.size(0) << ", " << design.size(1) << "]\n";

  // 2. Normal equations: beta = (X'X)^-1 X'y.
  auto xtx_inv = design.t().matmul(design).linearalg().inverse();
  auto beta = xtx_inv.matmul(design.t().matmul(y.unsqueeze(1)));
  show("beta (intercept, slope)", beta.squeeze(1));

  // 3. Residuals, noise level and standard errors of the coefficients.
  auto residual = y.unsqueeze(1) - design.matmul(beta);
  const double dof = static_cast<double>(n - 2);
  const double sigma2 = residual.square().sum().item<double>() / dof;
  std::cout << "residual standard deviation " << std::sqrt(sigma2) << " (true 1.5)\n";
  show("standard errors", (xtx_inv.diagonal() * sigma2).sqrt());

  // 4. R squared: the share of the variance of y that the line explains.
  const double ss_res = residual.square().sum().item<double>();
  const double ss_tot = (y - y.mean()).square().sum().item<double>();
  std::cout << "R squared " << 1.0 - ss_res / ss_tot << "\n";

  // 5. Predict at new points.
  auto fresh = pluss::make_series({0.0, 5.0, 12.0});
  auto newx = pluss::series::ones({3, 1}, ScalarType::Float64).unite({fresh.unsqueeze(1)}, 1);
  show("prediction at x = 0, 5, 12", newx.matmul(beta).squeeze(1));
}

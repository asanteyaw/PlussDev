// Integrate by random sampling: a one-dimensional integral with a known answer, a five-dimensional volume where grids fail,
// and a variance-reduction trick, each with its error estimate.
#include <pluss/pluss.h>

#include <cmath>
#include <iostream>

int main() {
  using pluss::ScalarType;
  const double pi = 3.14159265358979323846;

  // 1. The integral of 4 / (1 + x^2) over [0, 1] is pi. Average the integrand at uniform points: the mean times the interval
  //    length estimates the integral, and the standard deviation of the integrand over sqrt(N) is the error.
  for (const std::int64_t n : {100, 10000, 1000000}) {
    auto x = pluss::series::rand({n}, ScalarType::Float64, 17);
    auto f = (x.square() + 1.0).reciprocal() * 4.0;
    std::cout << "N = " << n << ": " << f.mean().item<double>() << " +/- " << f.std().item<double>() / std::sqrt(static_cast<double>(n)) << "   (pi = " << pi << ")\n";
  }

  // 2. Volume of the unit ball in five dimensions: sample the cube [-1, 1]^5 and count the points inside the ball.
  //    The cube has volume 32; the exact ball volume is 8 pi^2 / 15.
  const std::int64_t n = 2000000;
  auto points = pluss::series::rand({n, 5}, ScalarType::Float64, 99) * 2.0 - 1.0;
  auto inside = points.square().sum(1).le(1.0).to(ScalarType::Float64);
  const double fraction = inside.mean().item<double>();
  const double volume = 32.0 * fraction;
  const double error = 32.0 * std::sqrt(fraction * (1.0 - fraction) / static_cast<double>(n));
  std::cout << "5-ball volume " << volume << " +/- " << error << "   (exact " << 8.0 * pi * pi / 15.0 << ")\n";

  // 3. Antithetic variates: pair every point x with 1 - x. For a monotone integrand the two values are negatively correlated,
  //    so their average varies less than a single value. Integrate exp(x) over [0, 1]; the exact answer is e - 1.
  const std::int64_t m = 20000;
  auto u = pluss::series::rand({m}, ScalarType::Float64, 5);
  auto plain = u.exp();
  auto paired = (u.exp() + (-u + 1.0).exp()) * 0.5;
  std::cout << "plain      " << plain.mean().item<double>() << " +/- " << plain.std().item<double>() / std::sqrt(static_cast<double>(m)) << "\n";
  std::cout << "antithetic " << paired.mean().item<double>() << " +/- " << paired.std().item<double>() / std::sqrt(static_cast<double>(m)) << "   (exact " << std::exp(1.0) - 1.0 << ")\n";
}

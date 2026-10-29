#include <pluss/ma.h>

#include <iostream>
#include <sstream>

int main() {
  namespace math = pluss::math;
  namespace operators = pluss::operators;

  const math::Function f =
      math::function({"x", "y"}, [](math::Function x, math::Function y) {
        return x.pow(2) * (x * y).sin() * x.cos().exp();
      });
  const math::Function dfdx = operators::differential("x")(f);
  const pluss::Series x = pluss::make_series(1.0);
  const pluss::Series y = pluss::make_series(0.5);

  std::cout << "== structural differentiation ==\n";
  std::cout << "f(1, 0.5) = " << f({x, y}).item<double>() << "\n";
  std::cout << "df/dx(1, 0.5) = " << dfdx({x, y}).item<double>() << "\n\n";

  const math::Function gaussian = math::function(
      {"x"}, [](math::Function value) { return (-value.pow(2)).exp(); });
  const math::Function area = operators::integral("x", 0.0, 1.0)(gaussian);
  std::cout << "== numerical integral ==\n";
  std::cout << "integral_0^1 exp(-x^2) dx = " << area().item<double>() << "\n";
  std::cout << "provenance = " << static_cast<int>(area.provenance()) << "\n\n";

  const math::Function cgf = math::function(
      {"t"}, [](math::Function t) { return t * 1.5 + t.pow(2) * 2.0; });
  const math::Function mean = operators::differential("t")(cgf);
  const math::Function variance = operators::differential("t", 2)(cgf);
  std::cout << "== cumulants from a generic CGF ==\n";
  std::cout << "mean = " << mean(0.0).item<double>() << "\n";
  std::cout << "variance = " << variance(0.0).item<double>() << "\n\n";

  std::ostringstream artifact(std::ios::binary);
  math::io::save(dfdx, artifact);
  std::istringstream restored_artifact(artifact.str(), std::ios::binary);
  const math::Function restored = math::io::load_function(restored_artifact);
  std::cout << "== persistence ==\n";
  std::cout << "restored df/dx(1, 0.5) = " << restored({x, y}).item<double>()
            << "\n";
}

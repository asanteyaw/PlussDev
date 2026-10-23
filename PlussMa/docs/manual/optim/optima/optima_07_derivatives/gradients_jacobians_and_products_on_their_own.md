<!-- generated from gradients_jacobians_and_products_on_their_own.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Gradients, Jacobians and products on their own

[Back to Derivatives](../optima_07_derivatives.md)


The machinery that the algorithms use is available directly on the same parameter packs: `grad` and `value_and_grad` for a scalar function, `jacrev` and `jacfwd` for the Jacobian of a vector function by reverse and by forward mode, `jvp` for a derivative along a direction, and `hvp` for the Hessian times a direction without ever forming the Hessian. The program evaluates each at a point and compares it with the formula worked out by hand.

```cpp
#include <pluss/ma.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> pair(double x, double y) { return optim::pack_scalars({{"x", scalar(x)}, {"y", scalar(y)}}); }
double at(const optim::ParameterPack<pluss::Series>& p, const char* name) { return p.at(name).item<double>(); }

// One text, two number types: Trace for reverse mode, DualTrace for the forward-over-reverse products.
template <class AD>
AD f_of(const optim::ParameterPack<AD>& p) {
  return optim::square(p.at("x")) * p.at("y") + optim::sin(p.at("y"));
}
template <class AD>
AD big_f_of(const optim::ParameterPack<AD>& p) {
  return pluss::subsp::math::diff::unite({p.at("x") * p.at("y"), p.at("x") + optim::square(p.at("y"))}, 0);
}

int main() {
  const double x = 1.5, y = 0.7;
  const auto point = pair(x, y);
  std::cout << std::fixed << std::setprecision(6);
  double worst = 0.0;
  const auto check = [&](const char* label, std::initializer_list<std::pair<double, double>> pairs) {
    double err = 0.0;
    for (const auto& [got, want] : pairs) err = std::max(err, std::abs(got - want));
    worst = std::max(worst, err);
    std::cout << std::left << std::setw(28) << label << "max error " << std::scientific << std::setprecision(1) << err << std::fixed << std::setprecision(6) << "\n";
  };

  const auto [value, g] = optim::value_and_grad([](const optim::ParameterPack<optim::Trace>& p) { return f_of<optim::Trace>(p); }, point);
  std::cout << "f = " << value.item<double>() << ", df/dx = " << at(g, "x") << ", df/dy = " << at(g, "y") << "\n";
  check("value_and_grad", {{value.item<double>(), x * x * y + std::sin(y)}, {at(g, "x"), 2 * x * y}, {at(g, "y"), x * x + std::cos(y)}});

  const auto [fv, jr] = optim::jacrev([](const optim::ParameterPack<optim::Trace>& p) { return big_f_of<optim::Trace>(p); }, point);
  check("jacrev (reverse mode)", {{jr.at("x").select(0, 0).item<double>(), y}, {jr.at("x").select(0, 1).item<double>(), 1.0}, {jr.at("y").select(0, 0).item<double>(), x}, {jr.at("y").select(0, 1).item<double>(), 2 * y}});
  const auto [fv2, jf] = optim::jacfwd([](const optim::ParameterPack<optim::DualTrace>& p) { return big_f_of<optim::DualTrace>(p); }, point);
  check("jacfwd (forward mode)", {{jf.at("x").select(0, 0).item<double>(), y}, {jf.at("x").select(0, 1).item<double>(), 1.0}, {jf.at("y").select(0, 0).item<double>(), x}, {jf.at("y").select(0, 1).item<double>(), 2 * y}});

  // Directional derivative of f along (1, -2): df/dx - 2 df/dy.
  const auto [fj, directional] = optim::jvp([](const optim::ParameterPack<optim::DualTrace>& p) { return f_of<optim::DualTrace>(p); }, point, pair(1.0, -2.0));
  check("jvp along (1, -2)", {{directional.item<double>(), 2 * x * y - 2 * (x * x + std::cos(y))}});

  // Hessian of f times the direction (1, -2): [f_xx f_xy; f_xy f_yy] (1, -2) with f_xx = 2y, f_xy = 2x, f_yy = -sin y.
  const auto [fh, hv] = optim::hvp([](const optim::ParameterPack<optim::DualTrace>& p) { return f_of<optim::DualTrace>(p); }, point, pair(1.0, -2.0));
  check("hvp along (1, -2)", {{at(hv, "x"), 2 * y * 1.0 + 2 * x * -2.0}, {at(hv, "y"), 2 * x * 1.0 - std::sin(y) * -2.0}});

  std::cout << "\nall five agree with the hand-worked formulas to better than 1e-12: " << std::boolalpha << (worst < 1e-12) << "\n";
  return 0;
}
```

Output

```text
f = 2.219218, df/dx = 2.100000, df/dy = 3.014842
value_and_grad              max error 0.0e+00
jacrev (reverse mode)       max error 0.0e+00
jacfwd (forward mode)       max error 0.0e+00
jvp along (1, -2)           max error 0.0e+00
hvp along (1, -2)           max error 0.0e+00

all five agree with the hand-worked formulas to better than 1e-12: true
```
All five agree with the formulas to better than 1e-12. Reverse mode (`jacrev`) costs one pass per output and suits functions with few outputs and many inputs, such as a loss; forward mode (`jacfwd`, `jvp`) costs one pass per input and suits few inputs. `hvp` is forward mode applied to a reverse-mode gradient, which is why it asks for the dual form of the function: one product gives one column of the Hessian, so the Hessian of a function of `n` variables costs `n` products. The Jacobian of a vector function comes back as one block per parameter, each of shape `[outputs, block size]`, which is the shape the analytic constraints of Program 3 return.

<!-- generated from gradients_through_a_solve.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Gradients through a solve

[Back to Events, Stiffness and Gradients](../ode_02_events_stiffness_and_gradients.md)


`ode::fluxa_adjoint(f, y0, t0, t1, theta, grad_y_final, options)` returns the final state together with the gradient of a scalar loss with respect to the initial state (`grad_y0`) and to the parameters (`grad_theta`), given the seed `dL/dy(t1)`. It runs fixed-step RK4 forward, stores the trajectory at each step boundary, then walks it backward computing one step's vector–Jacobian product at a time. It never builds a single graph over the whole trajectory, so the memory is the stored trajectory, not a stack of intermediate operations.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  const ode::OdeFn decay = [](double, const Series& y, const Series& theta) { return y * theta * -1.0; };
  const double T = 2.0, y0 = 1.5, th = 0.7;
  const double dy0 = std::exp(-th * T), dth = -T * y0 * std::exp(-th * T);
  std::cout << std::left << std::setw(8) << "steps" << std::setw(18) << "dL/dy0" << std::setw(18) << "dL/dtheta" << "error in dL/dtheta\n";
  for (std::int64_t steps : {4, 8, 16, 32}) {
    ode::OdeOptions o;
    o.steps = steps;
    const auto g = ode::fluxa_adjoint(decay, vec({y0}), 0.0, T, vec({th}), vec({1.0}), o);
    std::cout << std::left << std::fixed << std::setprecision(8) << std::setw(8) << steps << std::setw(18) << g.grad_y0.item<double>() << std::setw(18)
              << g.grad_theta.item<double>() << std::scientific << std::setprecision(1) << std::abs(g.grad_theta.item<double>() - dth) << "\n";
  }
  std::cout << "exact   " << std::fixed << std::setprecision(8) << std::setw(18) << dy0 << std::setw(18) << dth << "\n";
}
```

Output

```text
steps   dL/dy0            dL/dtheta         error in dL/dtheta
4       0.24665483        -0.73930797       4.8e-04
8       0.24660009        -0.73976582       2.5e-05
16      0.24659715        -0.73978946       1.4e-06
32      0.24659697        -0.73979081       8.5e-08
exact   0.24659696        -0.73979089       
```
The adjoint is the exact gradient of the *discrete* RK4 map. It differs from the gradient of the true solution only by the method's own error, which falls by about 16 per halving of the step, as in Chapter 1. The method is defined for fixed-step RK4; asking for it with the adaptive method is an error, because differentiating through accepted and rejected step decisions is a separate problem. Choose `steps` so the discretisation error is below the accuracy you need in the gradient.

<!-- generated from a_design_that_cannot_be_built.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A design that cannot be built

[Back to Worked Problems](../optima_08_worked_problems.md)


The same can, now with a catalogue limit: the radius may not exceed 2 and the height may not exceed 20. The largest can the limits allow holds `pi * 2^2 * 20 = 251` cubic centimetres, far short of 1000. No point meets every requirement.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> rh(double r, double h) { return optim::pack_scalars({{"r", scalar(r)}, {"h", scalar(h)}}); }

void solve(const char* label, double max_height) {
  const double pi = 3.14159265358979323846, volume = 1000.0;
  auto area = [&](const Pack& p) { return 2.0 * pi * optim::square(p.at("r")) + 2.0 * pi * p.at("r") * p.at("h"); };
  const auto can = optim::constraints()
                       .lbs(rh(0.1, 0.1))
                       .ubs(rh(2.0, max_height))
                       .nl_ineq([&](const Pack& p) { return volume - pi * optim::square(p.at("r")) * p.at("h"); });
  const auto r = optim::optima(area, rh(1, 10), {}, can);
  std::cout << label << ": exit flag " << static_cast<int>(r.exit_flag) << ", max violation " << std::scientific << std::setprecision(2)
            << r.evaluation.max_constraint_violation << ", radius " << std::fixed << std::setprecision(4) << r.params.at("r").item<double>()
            << ", height " << r.params.at("h").item<double>() << "\n  " << r.message.substr(0, r.message.find(" diagnostic")) << "\n";
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

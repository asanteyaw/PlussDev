<!-- generated from three_ways_to_hand_over_the_same_objective.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Three ways to hand over the same objective

[Back to First Problems](../septima_01_first_problems.md)


Septima takes the objective in the form that suits your model. A single block of parameters is a `Series` and the objective is a function of one `Trace`. A model with several parts uses a `ParameterPack` whose blocks have names, and the objective reads them with `p.at("name")`. If you would rather not use `Trace` at all, the objective can be a function of a list of ordinary `Series` and the library still differentiates it. The program minimizes `(a - 1)^2 + (b + 2)^2` all three ways and then once more in single precision.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using optim::Trace;
using Pack = optim::ParameterPack<Trace>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  optim::SeptimaSolveOptions options;
  options.step.adam.learning_rate = 0.1;
  options.max_iterations = 1000;
  options.gradient_tolerance = 1e-4;  // looser than the default, so that each run can end by converging
  options.step_tolerance = 1e-4;

  std::cout << std::fixed << std::setprecision(4) << std::left;
  const auto report = [](const char* label, double a, double b, const optim::SeptimaResult& r) {
    std::cout << std::setw(26) << label << "a = " << a << ", b = " << b << "   iterations " << r.iteration
              << (r.converged ? "   converged" : "   did not converge") << "\n";
  };

  // 1. One block: a vector (a, b).
  const auto one = optim::septima(
      [](const Trace& v) -> Trace {
        const Trace target = optim::fixed(vec({1.0, -2.0}));
        return optim::sum(optim::square(v - target));
      },
      vec({0.0, 0.0}), options);
  report("one block", one.params[0].select(0, 0).item<double>(), one.params[0].select(0, 1).item<double>(), one);

  // 2. Named blocks.
  std::function<Trace(const Pack&)> named = [](const Pack& p) {
    return optim::square(p.at("a") - 1.0) + optim::square(p.at("b") + 2.0);
  };
  const auto two = optim::septima(named, optim::pack_scalars({{"a", scalar(0.0)}, {"b", scalar(0.0)}}), options);
  report("named blocks", two.params[0].item<double>(), two.params[1].item<double>(), two);

  // 3. A Series objective over a list of blocks.
  std::function<pluss::Series(const std::vector<pluss::Series>&)> plain = [](const std::vector<pluss::Series>& p) {
    return (p[0] - 1.0).square() + (p[1] + 2.0).square();
  };
  const auto three = optim::septima(plain, std::vector<pluss::Series>{scalar(0.0), scalar(0.0)}, options);
  report("Series objective", three.params[0].item<double>(), three.params[1].item<double>(), three);

  // The first form again, in single precision.
  const auto single = optim::septima(
      [](const Trace& v) -> Trace {
        const Trace target = optim::fixed(vec({1.0, -2.0}).to(pluss::ScalarType::Float32));
        return optim::sum(optim::square(v - target));
      },
      vec({0.0, 0.0}).to(pluss::ScalarType::Float32), options);
  report("one block, Float32", single.params[0].select(0, 0).item<double>(), single.params[0].select(0, 1).item<double>(), single);
  return 0;
}
```

Output

```text
one block                 a = 1.0000, b = -2.0000   iterations 191   converged
named blocks              a = 1.0000, b = -2.0000   iterations 191   converged
Series objective          a = 1.0000, b = -2.0000   iterations 191   converged
one block, Float32        a = 1.0000, b = -2.0000   iterations 191   converged
```
The three forms find the same point, `a = 1`, `b = -2`, in the same 191 iterations, because they describe one function: the form changes what you write, not what the solver does. Single precision does too, to four digits. Use named blocks when the parameters mean different things (weights, biases, scales), because the objective then reads like the model; use one vector block when the parameters are interchangeable. Parameters can be real or complex, in single or double precision, on the CPU or on a device; with constraints (chapter 2) they must all be on one platform.

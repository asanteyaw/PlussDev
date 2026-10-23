<!-- generated from several_blocks_and_a_complex_parameter.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Several blocks, and a complex parameter

[Back to The L-SR1 Trust-Region Optimizer](../septima_04_lsr1_trust_region.md)


The block form takes the objective as a function of a list of `Trace` blocks and returns the blocks in their original shapes and dtypes along with the result.

```cpp
#include <pluss/ma.h>

#include <complex>
#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using optim::Trace;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  std::cout << std::fixed << std::setprecision(4);
  {
    const auto target = vec({1.0, 2.0, 3.0});
    auto objective = [&](const std::vector<Trace>& blocks) -> Trace {
      const Trace& w = blocks[0];
      const Trace& b = blocks[1];
      return optim::sum(optim::square(w - optim::fixed(target))) + optim::square(b + 1.0) + optim::square(optim::sum(w) - b);
    };
    const auto [params, r] = optim::l_sr1_trust_region(objective, std::vector<pluss::Series>{vec({0.0, 0.0, 0.0}), vec({0.0})});
    const auto w = params[0].to_vec<double>();
    std::cout << "blocks: w = (" << w[0] << ", " << w[1] << ", " << w[2] << "), b = " << params[1].item<double>() << "   "
              << (r.converged ? "Converged" : "not converged") << " in " << r.iteration << " iterations\n";
  }
  {
    using C = std::complex<double>;
    const auto target = pluss::make_series(std::vector<C>{{1.0, 2.0}}, {1});
    auto distance = [&](const Trace& z) -> Trace {
      const Trace difference = z - optim::fixed(target);
      return optim::sum(pluss::subsp::math::diff::pow(pluss::subsp::math::diff::abs(difference), 2.0));
    };
    const auto r = optim::l_sr1_trust_region(distance, pluss::make_series(std::vector<C>{{0.0, 0.0}}, {1}));
    const C z = r.params.to_vec<C>()[0];
    std::cout << "complex: z = " << z.real() << " + " << z.imag() << "i   " << (r.converged ? "Converged" : "not converged") << " in " << r.iteration << " iterations\n";
  }
  return 0;
}
```

Output

```text
blocks: w = (-0.4000, 0.6000, 1.6000), b = 0.4000   Converged in 4 iterations
complex: z = 1.0000 + 2.0000i   Converged in 2 iterations
```
The first problem fits a vector `w` and a scalar `b`; the answer, `w = (-0.4, 0.6, 1.6)` and `b = 0.4`, satisfies the optimality conditions of the objective, and is found in 4 iterations. The second fits a complex number to the target `1 + 2i` with a real loss and finds it in 2. As with the other optimizers, the real and imaginary parts are treated as two independent coordinates; the loss must be real.

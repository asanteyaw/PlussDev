// Several parameter blocks, and a complex one. The block form of `l_sr1_trust_region` takes the objective as a function of a
// list of `Trace` blocks and returns the blocks (in their original shapes and dtypes) together with the result. The first
// problem fits a vector `w` and a scalar `b`: minimize |w - (1, 2, 3)|^2 + (b + 1)^2 + (sum(w) - b)^2. The second fits a
// complex number to a target with a real loss, which the solver handles by treating the real and imaginary parts as two
// coordinates.
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

// Three ways to hand Septima the same objective, f = (a - 1)^2 + (b + 2)^2. (1) One block: the parameters are a single
// vector and the objective is a function of one `pluss::Series`. (2) Named blocks: the parameters are a `ParameterPack` and the
// objective looks them up by name, which keeps a model with many parts readable. (3) A plain `Series` objective over
// a list of blocks: the function is written with ordinary `Series` operations and no `pluss::Series` at all. All three describe
// one function, so they must find the same point; the table shows the result of each, and the last row repeats the
// first in single precision.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;
using Pack = optim::ParameterPack<pluss::Series>;

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
      [](const pluss::Series& v) -> pluss::Series {
        const pluss::Series target = vec({1.0, -2.0});
        return (v - target).square().sum();
      },
      vec({0.0, 0.0}), options);
  report("one block", one.params[0].select(0, 0).item<double>(), one.params[0].select(0, 1).item<double>(), one);

  // 2. Named blocks.
  std::function<pluss::Series(const Pack&)> named = [](const Pack& p) {
    return (p.at("a") - 1.0).square() + (p.at("b") + 2.0).square();
  };
  const auto two = optim::septima(named, optim::pack_scalars({{"a", scalar(0.0)}, {"b", scalar(0.0)}}), options);
  report("named blocks", as_f64(two.params[0]), as_f64(two.params[1]), two);

  // 3. A Series objective over a list of blocks.
  std::function<pluss::Series(const std::vector<pluss::Series>&)> plain = [](const std::vector<pluss::Series>& p) {
    return (p[0] - 1.0).square() + (p[1] + 2.0).square();
  };
  const auto three = optim::septima(plain, std::vector<pluss::Series>{scalar(0.0), scalar(0.0)}, options);
  report("Series objective", as_f64(three.params[0]), as_f64(three.params[1]), three);

  // The first form again, in single precision.
  const auto single = optim::septima(
      [](const pluss::Series& v) -> pluss::Series {
        const pluss::Series target = vec({1.0, -2.0}).to(pluss::ScalarType::Float32);
        return (v - target).square().sum();
      },
      vec({0.0, 0.0}).to(pluss::ScalarType::Float32), options);
  report("one block, Float32", single.params[0].select(0, 0).item<double>(), single.params[0].select(0, 1).item<double>(), single);
  return 0;
}

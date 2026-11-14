// Complex parameters in Optima. A real objective of complex parameters is optimized over the real and imaginary
// parts as independent real coordinates: the algorithms see a real problem (every complex block replaced by its
// real view, a trailing axis of size 2) and the answer is mapped back. Before this support the real part was
// optimized and the imaginary part left at its start, with the solver reporting convergence.
//
// Every case has an exact answer worked out by hand, and the primary check is a self-consistency one: a run on the
// complex variable must equal a run on the same problem written with split real parameters.
#include <pluss/ma.h>

#include <cmath>
#include <complex>
#include <iostream>
#include <string>
#include <vector>

namespace {

namespace optim = pluss::subsp::math::optim;
using pluss::ScalarType;
using pluss::Series;
using Complex = std::complex<double>;
using Pack = optim::ParameterPack<Series>;

int failures = 0;
void expect(bool ok, const std::string& what) {
  if (!ok) {
    ++failures;
    std::cerr << "[FAIL] " << what << '\n';
  }
}
void near(Complex got, Complex want, double tol, const std::string& what) {
  if (std::abs(got - want) > tol) {
    ++failures;
    std::cerr << "[FAIL] " << what << ": got " << got << " want " << want << '\n';
  }
}

Series cvec(const std::vector<Complex>& v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(ScalarType::Complex128); }
std::vector<Complex> values(const Series& s) { return s.to(pluss::pCPU, ScalarType::Complex128).reshape({s.numel()}).host<Complex>(); }

const std::vector<optim::OptimaAlgorithm> kAlgorithms{optim::OptimaAlgorithm::SequentialQP, optim::OptimaAlgorithm::InternalProtocol,
                                                      optim::OptimaAlgorithm::WorkingSet, optim::OptimaAlgorithm::MeritMethod};

template <class Fn>
bool throws_with(Fn&& fn, const std::string& fragment) {
  try {
    fn();
  } catch (const std::exception& e) {
    return std::string(e.what()).find(fragment) != std::string::npos;
  }
  return false;
}

}  // namespace

int main() {
  const Series target = cvec({{1.0, 2.0}, {-1.0, 0.5}});
  const Series start = cvec({{0.0, 0.0}, {0.0, 0.0}});
  const auto loss = [&](const Series& z) { return (z - target).abs().square().sum(); };

  // 1) Unconstrained: every algorithm reaches the exact complex minimizer, imaginary parts included.
  for (const auto algorithm : kAlgorithms) {
    optim::OptimaOptions options;
    options.algorithm = algorithm;
    const auto r = optim::optima(loss, start, options);
    const auto z = values(r.params.at("x"));
    const std::string name(optim::optima_algorithm_name(algorithm));
    expect(r.converged, name + ": converged");
    near(z[0], {1.0, 2.0}, 1e-5, name + ": z0");
    near(z[1], {-1.0, 0.5}, 1e-5, name + ": z1");
    expect(r.params.at("x").dtype().is_complex(), name + ": result stays complex");
    expect(r.evaluation.gradient.at("x").dtype().is_complex(), name + ": gradient stays complex");
  }

  // 2) A genuinely complex nonconvex objective, |z^2 - 1|^2 from a start with both parts nonzero: minimum 0 at z = +-1.
  {
    const auto r = optim::optima([](const Series& z) { return (z.square() - 1.0).abs().square().sum(); }, cvec({{0.6, 0.4}}));
    const Complex z = values(r.params.at("x"))[0];
    expect(std::min(std::abs(z - Complex(1, 0)), std::abs(z + Complex(1, 0))) < 1e-3, "z^2 = 1 reached");
  }

  // 3) The complex run equals the same problem written with split real parameters.
  {
    const auto split_loss = [&](const Pack& p) {
      const Series re = p.at("re"), im = p.at("im");
      return (re - target.real()).square().sum() + (im - target.imag()).square().sum();
    };
    const Pack split_start({{"re", Series(start.real()).reshape({2})}, {"im", Series(start.imag()).reshape({2})}});
    const auto split = optim::optima(split_loss, split_start);
    const auto complex_run = optim::optima(loss, start);
    const auto z = values(complex_run.params.at("x"));
    const auto re = split.params.at("re").host<double>();
    const auto im = split.params.at("im").host<double>();
    near(z[0], {re[0], im[0]}, 1e-9, "complex run equals the split-real run (0)");
    near(z[1], {re[1], im[1]}, 1e-9, "complex run equals the split-real run (1)");
    expect(complex_run.iteration == split.iteration, "same number of iterations as the split-real run");
  }

  // 4) A complex equality constraint z0 + z1 = c: minimize |z0|^2 + |z1|^2, whose minimum is z0 = z1 = c / 2.
  {
    const Complex c{3.0, -2.0};
    const auto constraints = optim::constraints().nl_eq([&](const Pack& p) {
      const Series z = p.at("x");
      return z.select(0, 0) + z.select(0, 1) - cvec({c}).select(0, 0);
    });
    for (const auto algorithm : {optim::OptimaAlgorithm::SequentialQP, optim::OptimaAlgorithm::InternalProtocol}) {
      optim::OptimaOptions options;
      options.algorithm = algorithm;
      const auto r = optim::optima([](const Series& z) { return z.abs().square().sum(); }, cvec({{1.0, 1.0}, {-1.0, 0.5}}), options, constraints);
      const auto z = values(r.params.at("x"));
      near(z[0], c / 2.0, 1e-5, "equality: z0 = c / 2");
      near(z[1], c / 2.0, 1e-5, "equality: z1 = c / 2");
    }
  }

  // 5) A real inequality on a complex variable: the projection of 3 + 4i onto |z| <= 1 is 0.6 + 0.8i.
  {
    const auto disc = optim::constraints().nl_ineq([](const Pack& p) { return p.at("x").abs().square().sum() - 1.0; });
    const auto r = optim::optima([](const Series& z) { return (z - cvec({{3.0, 4.0}}).select(0, 0)).abs().square().sum(); }, cvec({{0.0, 0.0}}), {}, disc);
    near(values(r.params.at("x"))[0], {0.6, 0.8}, 1e-4, "projection onto the unit disc");
  }

  // 6) A mixed pack, one real block and one complex block, with an analytic gradient (dL/dRe + i dL/dIm).
  {
    const Pack mixed_start({{"a", pluss::make_series(std::vector<double>{0.0}, {1}).to(ScalarType::Float64)}, {"z", cvec({{0.0, 0.0}})}});
    const auto r = optim::optima(
        std::function<std::pair<Series, Pack>(const Pack&)>([](const Pack& p) -> std::pair<Series, Pack> {
          const Series a = p.at("a"), z = p.at("z");
          const Series value = (a - 2.0).square().sum() + (z - cvec({{1.0, -1.0}})).abs().square().sum();
          return {value, Pack({{"a", (a - 2.0) * 2.0}, {"z", (z - cvec({{1.0, -1.0}})) * 2.0}})};
        }),
        mixed_start);
    expect(std::abs(r.params.at("a").host<double>()[0] - 2.0) < 1e-5, "mixed pack: real block");
    near(values(r.params.at("z"))[0], {1.0, -1.0}, 1e-5, "mixed pack: complex block");
    expect(!r.params.at("a").dtype().is_complex(), "mixed pack: the real block stays real");
  }

  // 7) The progress hook sees complex parameters.
  {
    bool complex_seen = false;
    optim::OptimaOptions options;
    options.progress_hook = [&](const optim::OptimaProgressSnapshot& s) {
      complex_seen = complex_seen || s.params.at("x").dtype().is_complex();
      return false;
    };
    (void)optim::optima(loss, start, options);
    expect(complex_seen, "the progress hook receives complex parameters");
  }

  // 8) A real problem is untouched by all of this.
  {
    const Series real_target = pluss::make_series(std::vector<double>{1.0, 2.0}, {2}).to(ScalarType::Float64);
    const auto r = optim::optima([&](const Series& x) { return (x - real_target).square().sum(); }, pluss::make_series(std::vector<double>{0.0, 0.0}, {2}).to(ScalarType::Float64));
    expect(std::abs(r.params.at("x").host<double>()[1] - 2.0) < 1e-6 && !r.params.at("x").dtype().is_complex(), "real problems are unchanged");
  }

  // 9) What is refused, and why.
  expect(throws_with([&] { (void)optim::optima([](const Series& z) { return z.sum(); }, start); }, "must be real-valued"), "a complex objective is refused");
  expect(throws_with([&] {
           const auto c = optim::constraints().nl_ineq([](const Pack& p) { return p.at("x").sum(); });
           (void)optim::optima(loss, start, {}, c);
         }, "inequality constraint must be real-valued"), "a complex inequality residual is refused");
  expect(throws_with([&] {
           const auto c = optim::constraints().ln_eq(pluss::make_series(std::vector<double>{1, 1}, {1, 2}).to(ScalarType::Float64), pluss::make_series(std::vector<double>{1}, {1}).to(ScalarType::Float64));
           (void)optim::optima(loss, start, {}, c);
         }, "linear constraints are not supported"), "linear constraints on complex parameters are refused");
  expect(throws_with([&] {
           optim::Bounds b;
           b.lower = Pack({{"x", pluss::make_series(std::vector<double>{-1, -1}, {2}).to(ScalarType::Float64)}});
           optim::ConstrainedProblem problem;
           problem.objective.autodiff = optim::detail::trace_pack_callback_([&](const Series& z) { return loss(z); });
           problem.bounds = b;
           (void)optim::optima(problem, Pack({{"x", start}}));
         }, "bounds on the complex parameter"), "bounds on a complex parameter are refused");

  if (failures != 0) {
    std::cerr << failures << " failure(s)\n";
    return 1;
  }
  std::cout << "optima_complex_test passed\n";
  return 0;
}

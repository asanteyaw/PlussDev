#include <pluss/ma.h>

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using namespace pluss::subsp::stats::distr;

std::vector<double> as_f64_vec(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
}

std::string preview(const Series& x, std::size_t limit = 5) {
  const auto values = as_f64_vec(x);
  std::ostringstream oss;
  oss << "[";
  for (std::size_t i = 0; i < std::min(values.size(), limit); ++i) {
    if (i != 0) {
      oss << ", ";
    }
    oss << std::fixed << std::setprecision(6) << values[i];
  }
  if (values.size() > limit) {
    oss << ", ...";
  }
  oss << "]";
  return oss.str();
}

template <typename Dist>
void show_object(std::string_view name, const Dist& dist, const Series& x, const Series& t, const Series& like) {
  const Series sample_like_draws = dist.sample_like(like, 20260303ULL);
  std::cout << "\n" << name << "\n";
  std::cout << "  pdf(x):      " << preview(dist.pdf(x)) << "\n";
  std::cout << "  logpdf(x):   " << preview(dist.logpdf(x)) << "\n";
  std::cout << "  cdf(x):      " << preview(dist.cdf(x)) << "\n";
  std::cout << "  logcdf(x):   " << preview(dist.logcdf(x)) << "\n";
  std::cout << "  cgf(t):      " << preview(dist.cgf(t)) << "\n";
  std::cout << "  cumulants:   ["
            << dist.cumulant(1).template item<double>() << ", "
            << dist.cumulant(2).template item<double>() << ", "
            << dist.cumulant(3).template item<double>() << ", "
            << dist.cumulant(4).template item<double>() << "]\n";
  std::cout << "  sample({4}): " << preview(dist.sample({4}, Config{}.with_dtype(ScalarType::Float64), 20260303ULL)) << "\n";
  std::cout << "  sample_like: " << preview(sample_like_draws)
            << " dtype=" << static_cast<int>(sample_like_draws.scalar_type()) << "\n";
}

} // namespace

int main() {
  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series x_cont = pluss::make_series(std::vector<double>{-1.0, 0.0, 0.5, 1.0}, {4}, f64);
  const Series x_disc = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0}, {4}, f64);
  const Series t = pluss::make_series(std::vector<double>{-0.25, 0.0, 0.25}, {3}, f64);
  const Series t_nonpositive = pluss::make_series(std::vector<double>{-0.5, -0.25, 0.0}, {3}, f64);
  const Series t_zero_only = pluss::make_series(std::vector<double>{0.0}, {1}, f64);
  const Series like = pluss::series::zeros({4}, Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU));
  const Series mix_weights = pluss::make_series(std::vector<double>{0.7, 0.3}, {2}, f64);
  const Series mix_means = pluss::make_series(std::vector<double>{-1.0, 2.0}, {2}, f64);
  const Series mix_stddevs = pluss::make_series(std::vector<double>{0.5, 1.25}, {2}, f64);

  std::cout << "Object-oriented distribution wrappers over the functional catalog API\n";
  std::cout << "Continuous evaluation grid: " << preview(x_cont) << "\n";
  std::cout << "Discrete evaluation grid:   " << preview(x_disc) << "\n";
  std::cout << "CGF grid:                   " << preview(t) << "\n";

  show_object("Normal(mean=0, stddev=1)", Normal{0.0, 1.0}, x_cont, t, like);
  show_object("Gamma(shape=2, scale=1, location=0)", Gamma{2.0, 1.0, 0.0}, x_cont.add(1.5), t, like);
  show_object("ChiSquare(df=5)", ChiSquare{5}, x_cont.add(2.0), t, like);
  show_object("InverseGaussian(theta=1.5, chi=3)", InverseGaussian{1.5, 3.0}, x_cont.add(1.5), t, like);
  show_object("Lognormal(mean_log=0.1, stddev_log=0.4)", Lognormal{0.1, 0.4}, x_cont.add(1.5), t_nonpositive, like);
  show_object("Uniform(low=0, high=1)", Uniform{0.0, 1.0}, x_cont.add(1.0).div(2.0), t, like);
  show_object("Exponential(rate=2)", Exponential{2.0}, x_cont.add(1.5), t, like);
  show_object("Laplace(location=0, scale=0.5)", Laplace{0.0, 0.5}, x_cont, t, like);
  show_object("Logistic(location=0, scale=0.75)", Logistic{0.0, 0.75}, x_cont, t, like);
  show_object("Rayleigh(sigma=1)", Rayleigh{1.0}, x_cont.add(1.5), t, like);
  show_object("NormalTrunc(lower=-1, upper=1, mean=0, stddev=1)", NormalTrunc{-1.0, 1.0, 0.0, 1.0}, x_cont, t, like);
  show_object("StudentT(df=9, location=0, scale=1)", StudentT{9.0, 0.0, 1.0}, x_cont, t_zero_only, like);
  show_object("FDistribution(d1=5, d2=12)", FDistribution{5.0, 12.0}, x_cont.add(1.5), t_zero_only, like);
  show_object("Weibull(shape=2, scale=1.5)", Weibull{2.0, 1.5}, x_cont.add(1.5), t, like);
  show_object("Beta(low=0, high=1, alpha=2, beta=3)", Beta{0.0, 1.0, 2.0, 3.0}, x_cont.add(1.0).div(2.5), t, like);
  show_object("Bernoulli(p=0.25)", Bernoulli{0.25}, x_disc, t, like);
  show_object("Binomial(count=4, p=0.25)", Binomial{4, 0.25}, x_disc, t, like);
  show_object("Poisson(lambda=2.5)", Poisson{2.5}, x_disc, t, like);
  show_object("Geometric(p=0.25)", Geometric{0.25}, x_disc.add(1.0), t, like);
  show_object("NegativeBinomial(r=3, p=0.4)", NegativeBinomial{3.0, 0.4}, x_disc, t, like);
  show_object("Skellam(lambda1=2, lambda2=1)", Skellam{2.0, 1.0}, x_disc.sub(1.0), t, like);
  show_object("Nig(alpha=2, beta=0.5, location=0, scale=1)", Nig{2.0, 0.5, 0.0, 1.0}, x_cont, t, like);
  show_object("Gh(lambda=1, alpha=2, beta=0.5, location=0, scale=1)", Gh{1.0, 2.0, 0.5, 0.0, 1.0}, x_cont, t, like);
  show_object("GaussianMixture(weights=[0.7,0.3], means=[-1,2], stddevs=[0.5,1.25])",
              GaussianMixture{mix_weights, mix_means, mix_stddevs},
              x_cont,
              t,
              like);

  std::cout << "\nClosed-form MLE object methods\n";
  const Series xn = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4}, f64);
  const Series xe = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0}, {4}, f64);
  const Series xp = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0, 4.0}, {5}, f64);
  const Series xb = pluss::make_series(std::vector<double>{0.0, 1.0, 1.0, 0.0, 1.0}, {5}, f64);
  const Series xbin = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0, 4.0}, {5}, f64);
  const Series xg = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4}, f64);
  const Series xl = pluss::make_series(std::vector<double>{1.0, 2.0, 4.0, 8.0}, {4}, f64);
  const Series xu = pluss::make_series(std::vector<double>{2.0, 3.0, 5.0, 7.0}, {4}, f64);
  const Series xr = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4}, f64);

  const auto nfit = Normal{0.0, 1.0}.MLE(xn);
  const auto efit = Exponential{1.0}.MLE(xe);
  const auto pfit = Poisson{1.0}.MLE(xp);
  const auto bfit = Bernoulli{0.5}.MLE(xb);
  const auto binfit = Binomial{4, 0.5}.MLE(xbin);
  const auto gfit = Geometric{0.5}.MLE(xg);
  const auto lfit = Lognormal{0.0, 1.0}.MLE(xl);
  const auto ufit = Uniform{0.0, 1.0}.MLE(xu);
  const auto rfit = Rayleigh{1.0}.MLE(xr);

  std::cout << "  Normal MLE: mean=" << nfit.mean << " stddev=" << nfit.stddev
            << " score=" << preview(nfit.score(xn))
            << " fisher=" << preview(nfit.fisher_inf(xn)) << "\n";
  std::cout << "  Exponential MLE: rate=" << efit.rate
            << " score=" << preview(efit.score(xe))
            << " fisher=" << preview(efit.fisher_inf(xe)) << "\n";
  std::cout << "  Poisson MLE: lambda=" << pfit.lambda
            << " score=" << preview(pfit.score(xp))
            << " fisher=" << preview(pfit.fisher_inf(xp)) << "\n";
  std::cout << "  Bernoulli MLE: p=" << bfit.p
            << " score=" << preview(bfit.score(xb))
            << " fisher=" << preview(bfit.fisher_inf(xb)) << "\n";
  std::cout << "  Binomial MLE: count=" << binfit.count << " p=" << binfit.p
            << " score=" << preview(binfit.score(xbin))
            << " fisher=" << preview(binfit.fisher_inf(xbin)) << "\n";
  std::cout << "  Geometric MLE: p=" << gfit.p
            << " score=" << preview(gfit.score(xg))
            << " fisher=" << preview(gfit.fisher_inf(xg)) << "\n";
  std::cout << "  Lognormal MLE: mean_log=" << lfit.mean_log << " stddev_log=" << lfit.stddev_log
            << " score=" << preview(lfit.score(xl))
            << " fisher=" << preview(lfit.fisher_inf(xl)) << "\n";
  std::cout << "  Uniform MLE: low=" << ufit.low << " high=" << ufit.high
            << " score=" << preview(ufit.score(xu))
            << " fisher=" << preview(ufit.fisher_inf(xu)) << "\n";
  std::cout << "  Rayleigh MLE: sigma=" << rfit.sigma
            << " score=" << preview(rfit.score(xr))
            << " fisher=" << preview(rfit.fisher_inf(xr)) << "\n";

  return 0;
}

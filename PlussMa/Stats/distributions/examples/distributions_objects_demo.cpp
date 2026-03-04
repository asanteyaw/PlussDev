#include <pluss/pluss.h>

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
using namespace pluss::subsp::stat::distr;

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
  const Series like = pluss::zeros({4}, Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU));
  const Series mix_weights = pluss::make_series(std::vector<double>{0.7, 0.3}, {2}, f64);
  const Series mix_means = pluss::make_series(std::vector<double>{-1.0, 2.0}, {2}, f64);
  const Series mix_stddevs = pluss::make_series(std::vector<double>{0.5, 1.25}, {2}, f64);

  std::cout << "Object-oriented distribution wrappers over the functional catalog API\n";
  std::cout << "Continuous evaluation grid: " << preview(x_cont) << "\n";
  std::cout << "Discrete evaluation grid:   " << preview(x_disc) << "\n";
  std::cout << "CGF grid:                   " << preview(t) << "\n";

  show_object("Normal(mean=0, stddev=1)", Normal{0.0, 1.0}, x_cont, t, like);
  show_object("Gamma(shape=2, scale=1, location=0)", Gamma{2.0, 1.0, 0.0}, x_cont.add(1.5), t, like);
  show_object("Uniform(low=0, high=1)", Uniform{0.0, 1.0}, x_cont.add(1.0).div(2.0), t, like);
  show_object("Exponential(rate=2)", Exponential{2.0}, x_cont.add(1.5), t, like);
  show_object("Bernoulli(p=0.25)", Bernoulli{0.25}, x_disc, t, like);
  show_object("Binomial(count=4, p=0.25)", Binomial{4, 0.25}, x_disc, t, like);
  show_object("Skellam(lambda1=2, lambda2=1)", Skellam{2.0, 1.0}, x_disc.sub(1.0), t, like);
  show_object("Nig(alpha=2, beta=0.5, location=0, scale=1)", Nig{2.0, 0.5, 0.0, 1.0}, x_cont, t, like);
  show_object("GaussianMixture(weights=[0.7,0.3], means=[-1,2], stddevs=[0.5,1.25])",
              GaussianMixture{mix_weights, mix_means, mix_stddevs},
              x_cont,
              t,
              like);

  return 0;
}

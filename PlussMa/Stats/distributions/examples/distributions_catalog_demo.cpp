#include <pluss/pluss.h>

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

struct DemoCase {
  const char* name;
  std::vector<double> params;
  double probe;
};

double scalar_f64(const Series& x) {
  if (!x.defined() || x.numel() != 1) {
    throw std::runtime_error("expected scalar Series");
  }
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

Series scalar_series(double v, const Config& cfg) {
  return pluss::make_series(v, cfg);
}

std::span<const double> params_span(const std::vector<double>& params) {
  return std::span<const double>(params.data(), params.size());
}

std::string preview(const Series& x, std::size_t limit = 4) {
  const auto values = x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
  std::ostringstream out;
  out << "[";
  const std::size_t count = std::min(limit, values.size());
  for (std::size_t i = 0; i < count; ++i) {
    if (i != 0) out << ", ";
    out << std::fixed << std::setprecision(4) << values[i];
  }
  if (values.size() > count) out << ", ...";
  out << "]";
  return out.str();
}

void run_case(const DemoCase& c, const Config& cfg, std::uint64_t seed) {
  const Series probe = scalar_series(c.probe, cfg);
  const std::span<const std::int64_t> sizes = std::array<std::int64_t, 1>{5};
  const std::span<const double> params = params_span(c.params);

  const Series sample = c.params.empty()
                            ? pluss::distr::sample(c.name, sizes, cfg, seed)
                            : pluss::distr::sample(c.name, params, sizes, cfg, seed);
  const Series pdf = pluss::distr::pdf(c.name, probe, params);
  const Series logpdf = pluss::distr::logpdf(c.name, probe, params);
  const Series cdf = pluss::distr::cdf(c.name, probe, params);

  std::cout << std::left << std::setw(12) << c.name
            << " sample=" << preview(sample)
            << " pdf=" << std::fixed << std::setprecision(6) << scalar_f64(pdf)
            << " logpdf=" << scalar_f64(logpdf)
            << " cdf=" << scalar_f64(cdf)
            << '\n';
}

} // namespace

int main() {
  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const std::vector<DemoCase> cases = {
      {"alpha", {2.0, 3.0}, 2.0},
      {"anglit", {}, 0.0},
      {"arcsin", {}, 0.5},
      {"bern", {0.35}, 1.0},
      {"bessel", {0.0, 2.5}, 1.0},
      {"beta", {2.0, 5.0}, 0.35},
      {"binom", {10.0, 0.30}, 3.0},
      {"bradford", {1.5}, 0.4},
      {"burr", {2.0, 3.0}, 1.25},
      {"cauchy", {0.0, 1.0}, 0.25},
      {"chi", {3.0}, 1.5},
      {"chisq", {4.0}, 2.0},
      {"chisqnc", {1.5, 4.0}, 3.0},
      {"cosine", {}, 0.25},
      {"erlang", {1.25, 3.0}, 2.0},
      {"exp", {1.5}, 0.75},
      {"extrval", {0.0, 1.0}, 0.25},
      {"f", {4.0, 6.0}, 1.0},
      {"fnc", {1.5, 4.0, 6.0}, 1.0},
      {"gamma", {2.5}, 1.5},
      {"geom", {0.35}, 3.0},
      {"gig", {0.5, 1.0, 2.0}, 1.0},
      {"gh", {1.0, 2.0, 0.5, 0.0, 1.0}, 0.25},
      {"gompertz", {1.0, 2.0}, 0.5},
      {"hcos", {}, 0.0},
      {"hsec", {}, 0.0},
      {"hypergeom", {20.0, 8.0, 5.0}, 2.0},
      {"ig", {1.5, 2.0}, 1.0},
      {"laplace", {0.0, 1.0}, 0.25},
      {"logistic", {0.0, 1.0}, 0.25},
      {"lognorm", {0.0, 0.5}, 1.0},
      {"maxwell", {1.0}, 1.0},
      {"nakagami", {1.5, 2.0}, 1.0},
      {"negbinom", {3.0, 0.45}, 2.0},
      {"norm", {}, 0.0},
      {"normaltrunc", {-1.0, 1.0, 0.0, 1.0}, 0.0},
      {"nig", {2.0, 0.5, 0.0, 1.0}, 0.25},
      {"pareto", {1.0, 3.0}, 2.0},
      {"lomax", {2.0, 3.0}, 1.0},
      {"planck", {2.0, 1.5}, 0.75},
      {"poisson", {2.0}, 2.0},
      {"quadr", {}, 0.0},
      {"rademacher", {}, 1.0},
      {"rayl", {1.0}, 1.0},
      {"rice", {1.0, 1.0}, 1.5},
      {"semicirc", {0.0, 1.0}, 0.0},
      {"skellam", {2.0, 1.0}, 1.0},
      {"t", {5.0}, 0.5},
      {"tri", {0.0, 0.5, 1.0}, 0.6},
      {"tukeylambda", {0.14}, 0.0},
      {"u", {}, 0.0},
      {"uniform", {0.0, 1.0}, 0.25},
      {"vonmises", {0.0, 2.0}, 0.0},
      {"wald", {2.0}, 1.0},
      {"weibull", {0.0, 2.0, 1.5}, 1.0},
      {"yule", {3.0}, 2.0},
      {"zeta", {2.5}, 2.0},
  };

  std::cout << "PlussMa distribution catalog demo\n";
  std::cout << "surface: pluss::distr via #include <pluss/pluss.h>\n\n";

  for (std::size_t i = 0; i < cases.size(); ++i) {
    run_case(cases[i], f64, 1000ULL + static_cast<std::uint64_t>(i));
  }

  std::cout << "\nShort-form patterns\n";
  std::cout << "supports(gauss)=" << pluss::distr::supports("gauss")
            << " supports(zipf)=" << pluss::distr::supports("zipf")
            << " supports(pascal)=" << pluss::distr::supports("pascal") << '\n';

  const Series like = pluss::zeros({2, 4}, Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU));
  const Series like_gamma = pluss::distr::sample_like("gamma", {2.0}, like, 77ULL);
  const Series like_sign = pluss::distr::sample_like("rademacher", like, 79ULL);
  std::cout << "sample_like(gamma) preview=" << preview(like_gamma)
            << " dtype=" << static_cast<int>(like_gamma.scalar_type()) << '\n';
  std::cout << "sample_like(rademacher) preview=" << preview(like_sign)
            << " dtype=" << static_cast<int>(like_sign.scalar_type()) << '\n';

  const Series weights = pluss::make_series(std::vector<double>{0.65, 0.35}, {2}, f64);
  const Series means = pluss::make_series(std::vector<double>{-1.0, 2.0}, {2}, f64);
  const Series stddevs = pluss::make_series(std::vector<double>{0.5, 1.25}, {2}, f64);
  const Series pseudo_mix = pluss::distr::mixture_normal(weights, means, stddevs, {8}, f64, 101ULL, false);
  const Series sobol_mix = pluss::distr::mixture_normal(weights, means, stddevs, {8}, f64, 103ULL, true);

  std::cout << "mixture_normal pseudo=" << preview(pseudo_mix) << '\n';
  std::cout << "mixture_normal sobol =" << preview(sobol_mix) << '\n';

  return 0;
}

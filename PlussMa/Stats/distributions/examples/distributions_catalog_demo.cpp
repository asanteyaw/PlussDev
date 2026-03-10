#include <pluss/ma.h>
#include "pluss/ma/aliases.h"

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
  ma::dst::Kind kind;
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
  const std::array<std::int64_t, 1> sizes_buf{5};
  const std::span<const std::int64_t> sizes = sizes_buf;
  const std::span<const double> params = params_span(c.params);
  const ma::dst::Kind kind = c.kind;

  const Series sample = c.params.empty()
                            ? pluss::distr::sample(kind, sizes, cfg, seed)
                            : pluss::distr::sample(kind, params, sizes, cfg, seed);
  const Series pdf = pluss::distr::pdf(kind, probe, params);
  const Series logpdf = pluss::distr::logpdf(kind, probe, params);
  const Series cdf = pluss::distr::cdf(kind, probe, params);

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
      {"alpha", ma::dst::Alpha, {2.0, 3.0}, 2.0},
      {"anglit", ma::dst::Anglit, {}, 0.0},
      {"arcsin", ma::dst::Arcsin, {}, 0.5},
      {"bern", ma::dst::Bernoulli, {0.35}, 1.0},
      {"bessel", ma::dst::Bessel, {0.0, 2.5}, 1.0},
      {"beta", ma::dst::Beta, {2.0, 5.0}, 0.35},
      {"binom", ma::dst::Binomial, {10.0, 0.30}, 3.0},
      {"bradford", ma::dst::Bradford, {1.5}, 0.4},
      {"burr", ma::dst::Burr, {2.0, 3.0}, 1.25},
      {"cauchy", ma::dst::Cauchy, {0.0, 1.0}, 0.25},
      {"chi", ma::dst::Chi, {3.0}, 1.5},
      {"chisq", ma::dst::ChiSquare, {4.0}, 2.0},
      {"chisqnc", ma::dst::ChiSquareNoncentral, {1.5, 4.0}, 3.0},
      {"cosine", ma::dst::Cosine, {}, 0.25},
      {"erlang", ma::dst::Erlang, {1.25, 3.0}, 2.0},
      {"exp", ma::dst::Exponential, {1.5}, 0.75},
      {"extrval", ma::dst::ExtremeValue, {0.0, 1.0}, 0.25},
      {"f", ma::dst::F, {4.0, 6.0}, 1.0},
      {"fnc", ma::dst::FNoncentral, {1.5, 4.0, 6.0}, 1.0},
      {"gamma", ma::dst::Gamma, {2.5}, 1.5},
      {"geom", ma::dst::Geometric, {0.35}, 3.0},
      {"gig", ma::dst::Gig, {0.5, 1.0, 2.0}, 1.0},
      {"gh", ma::dst::Gh, {1.0, 2.0, 0.5, 0.0, 1.0}, 0.25},
      {"gompertz", ma::dst::Gompertz, {1.0, 2.0}, 0.5},
      {"hcos", ma::dst::HalfCosine, {}, 0.0},
      {"hsec", ma::dst::HyperbolicSecant, {}, 0.0},
      {"hypergeom", ma::dst::Hypergeometric, {20.0, 8.0, 5.0}, 2.0},
      {"ig", ma::dst::InverseGaussian, {1.5, 2.0}, 1.0},
      {"laplace", ma::dst::Laplace, {0.0, 1.0}, 0.25},
      {"logistic", ma::dst::Logistic, {0.0, 1.0}, 0.25},
      {"lognorm", ma::dst::Lognormal, {0.0, 0.5}, 1.0},
      {"maxwell", ma::dst::Maxwell, {1.0}, 1.0},
      {"nakagami", ma::dst::Nakagami, {1.5, 2.0}, 1.0},
      {"negbinom", ma::dst::NegativeBinomial, {3.0, 0.45}, 2.0},
      {"norm", ma::dst::Normal, {}, 0.0},
      {"normaltrunc", ma::dst::NormalTrunc, {-1.0, 1.0, 0.0, 1.0}, 0.0},
      {"nig", ma::dst::Nig, {2.0, 0.5, 0.0, 1.0}, 0.25},
      {"pareto", ma::dst::Pareto, {1.0, 3.0}, 2.0},
      {"lomax", ma::dst::Lomax, {2.0, 3.0}, 1.0},
      {"planck", ma::dst::Planck, {2.0, 1.5}, 0.75},
      {"poisson", ma::dst::Poisson, {2.0}, 2.0},
      {"quadr", ma::dst::Quadratic, {}, 0.0},
      {"rademacher", ma::dst::Rademacher, {}, 1.0},
      {"rayl", ma::dst::Rayleigh, {1.0}, 1.0},
      {"rice", ma::dst::Rice, {1.0, 1.0}, 1.5},
      {"semicirc", ma::dst::Semicircle, {0.0, 1.0}, 0.0},
      {"skellam", ma::dst::Skellam, {2.0, 1.0}, 1.0},
      {"t", ma::dst::StudentT, {5.0}, 0.5},
      {"tri", ma::dst::Triangular, {0.0, 0.5, 1.0}, 0.6},
      {"tukeylambda", ma::dst::TukeyLambda, {0.14}, 0.0},
      {"u", ma::dst::UShape, {}, 0.0},
      {"uniform", ma::dst::Uniform, {0.0, 1.0}, 0.25},
      {"vonmises", ma::dst::VonMises, {0.0, 2.0}, 0.0},
      {"wald", ma::dst::Wald, {2.0}, 1.0},
      {"weibull", ma::dst::Weibull, {0.0, 2.0, 1.5}, 1.0},
      {"yule", ma::dst::Yule, {3.0}, 2.0},
      {"zeta", ma::dst::Zeta, {2.5}, 2.0},
  };

  std::cout << "PlussMa distribution catalog demo\n";
  std::cout << "surface: pluss::distr via #include <pluss/ma.h>\n\n";

  for (std::size_t i = 0; i < cases.size(); ++i) {
    run_case(cases[i], f64, 1000ULL + static_cast<std::uint64_t>(i));
  }

  std::cout << "\nShort-form patterns\n";
  std::cout << "supports(Normal)=" << pluss::distr::supports(ma::dst::Normal)
            << " supports(Zeta)=" << pluss::distr::supports(ma::dst::Zeta)
            << " supports(NegativeBinomial)=" << pluss::distr::supports(ma::dst::NegativeBinomial) << '\n';

  const Series like = pluss::zeros({2, 4}, Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU));
  const Series like_gamma = pluss::distr::sample_like(ma::dst::Gamma, {2.0}, like, 77ULL);
  const Series like_sign = pluss::distr::sample_like(ma::dst::Rademacher, like, 79ULL);
  const Series nig = pluss::distr::sample(ma::dst::Nig, {2.0, 0.5, 0.0, 1.0}, {2, 4}, f64, 83ULL);
  std::cout << "sample_like(gamma) preview=" << preview(like_gamma)
            << " dtype=" << static_cast<int>(like_gamma.scalar_type()) << '\n';
  std::cout << "sample_like(rademacher) preview=" << preview(like_sign)
            << " dtype=" << static_cast<int>(like_sign.scalar_type()) << '\n';
  std::cout << "sample(nig) preview=" << preview(nig) << " dtype=" << static_cast<int>(nig.scalar_type()) << '\n';
  const Series weights = pluss::make_series(std::vector<double>{0.65, 0.35}, {2}, f64);
  const Series means = pluss::make_series(std::vector<double>{-1.0, 2.0}, {2}, f64);
  const Series stddevs = pluss::make_series(std::vector<double>{0.5, 1.25}, {2}, f64);
  const Series pseudo_mix = pluss::distr::mixture_normal(weights, means, stddevs, {8}, f64, 101ULL, false);
  const Series sobol_mix = pluss::distr::mixture_normal(weights, means, stddevs, {8}, f64, 103ULL, true);
  const Series mix_x = pluss::make_series(std::vector<double>{-2.0, -0.5, 1.0, 3.0}, {4}, f64);
  const Series mix_pdf = pluss::distr::mixture_normal_pdf(mix_x, weights, means, stddevs);
  const Series mix_logpdf = pluss::distr::mixture_normal_logpdf(mix_x, weights, means, stddevs);
  const Series mix_cdf = pluss::distr::mixture_normal_cdf(mix_x, weights, means, stddevs);

  std::cout << "mixture_normal pseudo=" << preview(pseudo_mix) << '\n';
  std::cout << "mixture_normal sobol =" << preview(sobol_mix) << '\n';
  std::cout << "mixture_normal pdf   =" << preview(mix_pdf) << '\n';
  std::cout << "mixture_normal cdf   =" << preview(mix_cdf) << '\n';
  std::cout << "log likelihood = " << mix_logpdf.sum() << "\n";

  return 0;
}

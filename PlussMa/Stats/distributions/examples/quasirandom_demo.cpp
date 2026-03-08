#include <pluss/pluss.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

namespace {

std::string preview(const Series& s, std::size_t k = 8) {
  const auto v = s.to_vec<double>();
  const std::size_t n = std::min(k, v.size());
  std::string out = "[";
  for (std::size_t i = 0; i < n; ++i) {
    out += std::to_string(v[i]);
    if (i + 1 < n) out += ", ";
  }
  if (v.size() > n) out += ", ...";
  out += "]";
  return out;
}

void show(const char* name, const Series& s) {
  std::cout << name << " sizes=";
  const auto sz = s.sizes();
  std::cout << "{";
  for (std::size_t i = 0; i < sz.size(); ++i) {
    std::cout << sz[i];
    if (i + 1 < sz.size()) std::cout << ",";
  }
  std::cout << "} preview=" << preview(s) << '\n';
}

} // namespace

int main() {
  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // 1) Direct low-discrepancy draws
  const Series sob = pluss::rng::sobol({8, 3}, f64, 1ULL);
  const Series hal = pluss::rng::halton({8, 3}, f64, 1ULL);
  const Series tor = pluss::rng::torus({8, 3}, f64, 1ULL);

  show("sobol", sob);
  show("halton", hal);
  show("torus", tor);

  // 2) randtoolbox-style options
  pluss::rng::SobolOptions sob_opts{};
  sob_opts.start = 1;
  sob_opts.scrambling = 3;
  sob_opts.mixed = true;
  sob_opts.seed = 42;
  sob_opts.seed_set = true;
  const Series sob_scr = pluss::rng::sobol({8, 3}, sob_opts, f64);
  show("sobol(scrambling=3,mixed=true)", sob_scr);

  pluss::rng::TorusOptions tor_opts{};
  tor_opts.prime = {2, 3, 5, 7};
  const Series tor_custom = pluss::rng::torus({6}, tor_opts, f64); // becomes {6,4}
  show("torus(prime={2,3,5,7})", tor_custom);

  // 3) Distribution sampling using low-discrepancy methods
  const Series normal_sob = pluss::distr::sample(
      "normal", {0.0, 1.0}, {10}, f64, 123ULL, pluss::rng::Method::Sobol);

  const Series like = pluss::zeros({2, 5}, f64);
  const Series unif_like_tor = pluss::distr::sample_like(
      "uniform", {0.0, 1.0}, like, 999ULL, pluss::rng::Method::Torus);

  show("distr::sample(normal, Sobol)", normal_sob);
  show("distr::sample_like(uniform, Torus)", unif_like_tor);

  return 0;
}

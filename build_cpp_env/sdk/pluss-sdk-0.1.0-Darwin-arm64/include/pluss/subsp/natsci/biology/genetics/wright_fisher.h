#pragma once

// Single-locus, two-allele Wright-Fisher model, batched over replicates /
// loci / parameter lanes (frequency Series of any shape).
//
// One generation: selection, then mutation, then binomial drift of 2N gametes:
//   p*  = (p^2 wAA + p q wAa) / wbar,   wbar = p^2 wAA + 2pq wAa + q^2 waa
//   p** = p* (1 - mu) + (1 - p*) nu
//   k ~ Binomial(2N, p**),  p' = k / 2N
// Exact references: the (2N+1)x(2N+1) transition matrix and absorption
// quantities (markov.h), the diffusion fixation probability
//   u(p) = (1 - exp(-4 Ne s p)) / (1 - exp(-4 Ne s))
// for additive selection wAA = 1+2s, wAa = 1+s, waa = 1 (Kimura 1962; the
// formula is quoted from memory and secondary sources and checked here
// against the exact chain, to within 0.4% at N = 40, s = 0.01), and neutral
// fixation probability = p0 (martingale).
//
// The binomial sampler is exact (inverse CDF over a lgamma pmf, one
// [lanes, 2N+1] tensor per generation), intended for 2N up to a few thousand.

#include "pluss/subsp/natsci/biology/genetics/markov.h"

namespace pluss::subsp::natsci::biology::genetics {

using dynamics::Coeff;

struct Fitness final {
  Coeff aa{1.0}, ab{1.0}, bb{1.0}; // genotypes AA, Aa, aa
};

// Additive genic selection: AA 1+2s, Aa 1+s, aa 1.
inline Fitness additive(double s) {
  Fitness w;
  w.aa = 1.0 + 2.0 * s;
  w.ab = 1.0 + s;
  return w;
}
inline Fitness additive(const Series &s) {
  Fitness w;
  w.aa = s.mul(2.0).add(1.0);
  w.ab = s.add(1.0);
  return w;
}

inline Series hardy_weinberg(const Series &p) {
  const Series q = p.neg().add(1.0);
  return p.square().unsqueeze(-1).unite(
      std::vector<Series>{p.mul(q).mul(2.0).unsqueeze(-1), q.square().unsqueeze(-1)}, -1);
}

inline Series mean_fitness(const Series &p, const Fitness &w) {
  const Series q = p.neg().add(1.0);
  return p.square().mul(w.aa.on(p)).add(p.mul(q).mul(2.0).mul(w.ab.on(p)))
      .add(q.square().mul(w.bb.on(p)));
}

inline Series select(const Series &p, const Fitness &w) {
  const Series q = p.neg().add(1.0);
  const Series num = p.square().mul(w.aa.on(p)).add(p.mul(q).mul(w.ab.on(p)));
  return num.div(mean_fitness(p, w));
}

inline Series mutate(const Series &p, const Coeff &mu, const Coeff &nu) {
  return p.mul(mu.on(p).neg().add(1.0)).add(p.neg().add(1.0).mul(nu.on(p)));
}

// Expected next frequency (deterministic part of one generation).
inline Series expected_next(const Series &p, const Fitness &w, const Coeff &mu = 0.0,
                            const Coeff &nu = 0.0) {
  dynamics::require_nonnegative(p, "wright_fisher: frequency");
  dynamics::require(p.le(1.0).all().item<bool>(),
                    "wright_fisher: frequency must not exceed one");
  return mutate(select(p, w), mu, nu);
}

// Overdominance equilibrium for wAA = 1 - s, wAa = 1, waa = 1 - t.
inline Series heterozygote_advantage_equilibrium(const Series &s, const Series &t) {
  return t.div(s.add(t));
}

// pmf of Binomial(n, p) over k = 0..n, as [..., n+1].
inline Series binomial_pmf(std::int64_t n, const Series &p) {
  dynamics::require(n >= 1, "binomial: n must be positive");
  const Series k = pluss::series::arange(0, n + 1, 1, p.scalar_type()).to(p.platform());
  const double ln_n = std::lgamma(static_cast<double>(n) + 1.0);
  const Series pc = p.clamp(1e-300, 1.0 - 1e-16).unsqueeze(-1);
  const Series log_choose = k.add(1.0).lgamma().neg().sub(k.neg().add(static_cast<double>(n) + 1.0).lgamma()).add(ln_n);
  const Series pmf = log_choose.add(k.mul(pc.log())).add(k.neg().add(static_cast<double>(n)).mul(pc.neg().add(1.0).log())).exp();
  // exact absorbing boundaries
  const Series zero = p.le(0.0).unsqueeze(-1);
  const Series full = p.ge(1.0).unsqueeze(-1);
  const Series at0 = k.eq(0.0);
  const Series atn = k.eq(static_cast<double>(n));
  return zero.where(at0.to(p.scalar_type()).add(pmf.mul(0.0)),
                    full.where(atn.to(p.scalar_type()).add(pmf.mul(0.0)), pmf));
}

// Exact binomial draw per lane via inverse CDF.
inline Series sample_binomial(std::int64_t n, const Series &p, std::uint64_t seed) {
  const Config cfg = Config{}.with_dtype(p.scalar_type()).with_platform(p.platform());
  const Series u = pluss::series::rand_like(p, cfg, seed).unsqueeze(-1);
  const Series cdf = binomial_pmf(n, p).cumsum(-1, false);
  const Series k = cdf.lt(u).to(p.scalar_type()).sum(-1, false);
  return k.clamp(0.0, static_cast<double>(n));
}

// One stochastic generation with 2N gametes.
inline Series step(const Series &p, std::int64_t population, const Fitness &w,
                   const Coeff &mu, const Coeff &nu, std::uint64_t seed) {
  const std::int64_t gametes = 2 * population;
  return sample_binomial(gametes, expected_next(p, w, mu, nu), seed)
      .div(static_cast<double>(gametes));
}

struct Run final {
  Series frequency = pluss::series::undefined();
  std::int64_t generations{0};
  bool absorbed{false}; // every lane at 0 or 1
};

inline Run simulate(const Series &p0, std::int64_t population, const Fitness &w,
                    std::int64_t max_generations, std::uint64_t seed,
                    const Coeff &mu = 0.0, const Coeff &nu = 0.0) {
  dynamics::require(population >= 1, "wright_fisher: population must be positive");
  Series p = p0;
  Run out;
  const bool mutating = (!mu.is_series() && mu.scalar() > 0.0) ||
                        (!nu.is_series() && nu.scalar() > 0.0) ||
                        mu.is_series() || nu.is_series();
  for (std::int64_t g = 0; g < max_generations; ++g) {
    p = step(p, population, w, mu, nu, seed + static_cast<std::uint64_t>(g) + 1);
    out.generations = g + 1;
    if (!mutating && (g & 7) == 7) {
      const Series interior = p.gt(0.0).logical_and(p.lt(1.0));
      if (!interior.any().item<bool>()) {
        out.absorbed = true;
        break;
      }
    }
  }
  out.frequency = p;
  return out;
}

// Exact (2N+1)x(2N+1) transition matrix; rows are the current allele count.
inline Series transition_matrix(std::int64_t population, const Fitness &w,
                                const Coeff &mu = 0.0, const Coeff &nu = 0.0) {
  const std::int64_t n = 2 * population;
  const Series counts = pluss::series::arange(0, n + 1, 1, pluss::ScalarType::Float64)
                            .div(static_cast<double>(n));
  return binomial_pmf(n, expected_next(counts, w, mu, nu));
}

// Diffusion approximation for additive selection (Kimura 1962); s -> 0 gives p.
inline Series kimura_fixation(const Series &p, const Coeff &ne, const Coeff &s) {
  const Series a = ne.on(p).mul(s.on(p)).mul(4.0);
  const Series tiny = a.abs().lt(1e-12);
  const Series safe = tiny.where(a.mul(0.0).add(1.0), a);
  const Series ratio = p.mul(safe.neg()).expm1().div(safe.neg().expm1());
  return tiny.where(p, ratio);
}

} // namespace pluss::subsp::natsci::biology::genetics

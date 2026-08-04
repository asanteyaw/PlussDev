#include "pluss/subsp/stats/rng.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"

#include "sobol1111_data.h"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace pluss::subsp::stats::rng {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

constexpr double kNormalEps = 1e-12;

thread_local std::unordered_map<std::int64_t, std::uint64_t> g_sobol_next;
thread_local std::unordered_map<std::int64_t, std::uint64_t> g_halton_next;
thread_local std::unordered_map<std::int64_t, std::uint64_t> g_torus_next;

struct ShapeSpec {
  std::vector<std::int64_t> sizes;
  std::int64_t points = 0;
  std::int64_t dims = 0;
  std::int64_t total = 0;
};

ScalarType resolve_float_dtype_(ScalarType requested, const char* op) {
  if (requested == ScalarType::Float32 || requested == ScalarType::Float64) {
    return requested;
  }
  PLUSS_CHECK(false, std::string(op) + ": dtype must be Float32 or Float64");
  return ScalarType::Float64;
}

Config float64_cfg_(Config cfg) {
  return Config{}
      .with_dtype(ScalarType::Float64)
      .with_platform(cfg.platform)
      .with_pinned(cfg.pinned);
}

Config f64_cpu_cfg_() {
  return Config{}.with_dtype(ScalarType::Float64);
}

std::int64_t checked_mul_(std::int64_t a, std::int64_t b, const char* op) {
  if (a == 0 || b == 0) return 0;
  PLUSS_CHECK(a <= std::numeric_limits<std::int64_t>::max() / b,
              std::string(op) + ": size product overflow");
  return a * b;
}

void validate_mexp_(int mexp, const char* op) {
  static constexpr std::array<int, 10> kAuthorized = {
      607, 1279, 2281, 4253, 11213, 19937, 44497, 86243, 132049, 216091};
  PLUSS_CHECK(std::find(kAuthorized.begin(), kAuthorized.end(), mexp) != kAuthorized.end(),
              std::string(op) + ": mexp must be in {607,1279,2281,4253,11213,19937,44497,86243,132049,216091}");
}

std::uint64_t now_seed_() {
  const auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
  return static_cast<std::uint64_t>(now.count());
}

std::uint64_t choose_seed_(bool seed_set, std::uint64_t seed, int mexp) {
  std::uint64_t out = seed_set ? seed : now_seed_();
  out ^= (static_cast<std::uint64_t>(static_cast<std::uint32_t>(mexp)) << 32U);
  out ^= 0x9E3779B97F4A7C15ULL;
  return out;
}

ShapeSpec decode_shape_(std::span<const std::int64_t> sizes, const char* op) {
  ShapeSpec spec;
  spec.sizes.assign(sizes.begin(), sizes.end());

  if (sizes.empty()) {
    spec.points = 1;
    spec.dims = 1;
    spec.total = 1;
    return spec;
  }

  for (const std::int64_t s : sizes) {
    PLUSS_CHECK(s >= 0, std::string(op) + ": sizes must be >= 0");
  }

  if (sizes.size() == 1) {
    spec.points = sizes.front();
    spec.dims = 1;
    spec.total = sizes.front();
    return spec;
  }

  spec.dims = sizes.back();
  PLUSS_CHECK(spec.dims > 0, std::string(op) + ": last axis (dimension) must be > 0");

  spec.points = 1;
  for (std::size_t i = 0; i + 1 < sizes.size(); ++i) {
    spec.points = checked_mul_(spec.points, sizes[i], op);
  }
  spec.total = checked_mul_(spec.points, spec.dims, op);
  return spec;
}

void force_dims_(ShapeSpec& spec, std::int64_t forced_dims, const char* op) {
  if (forced_dims <= 0) return;

  if (spec.sizes.empty()) {
    spec.sizes = {1, forced_dims};
    spec.points = 1;
    spec.dims = forced_dims;
    spec.total = forced_dims;
    return;
  }

  if (spec.sizes.size() == 1) {
    spec.points = spec.sizes.front();
    spec.dims = forced_dims;
    spec.total = checked_mul_(spec.points, spec.dims, op);
    spec.sizes.push_back(forced_dims);
    return;
  }

  PLUSS_CHECK(spec.sizes.back() == forced_dims,
              std::string(op) + ": sizes last axis must match provided dimension/primes length");
  spec.dims = forced_dims;
  spec.total = checked_mul_(spec.points, spec.dims, op);
}

Series to_standard_normal_(const Series& uniform_u, Config cfg, const char* op) {
  const ScalarType out_dtype = resolve_float_dtype_(cfg.dtype, op);
  const Series u = uniform_u.to(cfg.platform, ScalarType::Float64)
                       .clamp_min(kNormalEps)
                       .clamp_max(1.0 - kNormalEps);
  return u.mul(2.0).sub(1.0).erfinv().mul(std::sqrt(2.0)).to(cfg.platform, out_dtype);
}

std::vector<std::uint32_t> first_primes_(std::int64_t n, const char* op) {
  if (n <= 0) return {};
  PLUSS_CHECK(n <= 100000, std::string(op) + ": dimension must be <= 100000");

  thread_local std::vector<std::uint32_t> prime_cache{2U};
  if (prime_cache.size() < static_cast<std::size_t>(n)) {
    std::uint32_t candidate = prime_cache.back() + 1U;
    if ((candidate & 1U) == 0U) ++candidate;

    while (prime_cache.size() < static_cast<std::size_t>(n)) {
      bool is_prime = true;
      for (const std::uint32_t p : prime_cache) {
        if (static_cast<std::uint64_t>(p) * static_cast<std::uint64_t>(p) > candidate) break;
        if (candidate % p == 0U) {
          is_prime = false;
          break;
        }
      }

      if (is_prime) {
        prime_cache.push_back(candidate);
      }

      PLUSS_CHECK(candidate < std::numeric_limits<std::uint32_t>::max() - 2U,
                  std::string(op) + ": unable to allocate requested dimensions");
      candidate += 2U;
    }
  }

  return std::vector<std::uint32_t>(prime_cache.begin(),
                                    prime_cache.begin() + static_cast<std::ptrdiff_t>(n));
}

double radical_inverse_(std::uint64_t index, std::uint32_t base) {
  const double inv_base = 1.0 / static_cast<double>(base);
  double factor = inv_base;
  double value = 0.0;
  std::uint64_t n = index;
  while (n != 0U) {
    value += factor * static_cast<double>(n % base);
    n /= base;
    factor *= inv_base;
  }
  return value;
}

Series make_f64_series_(const std::vector<double>& values, std::span<const std::int64_t> sizes) {
  return pluss::make_series(values, sizes, Config{}.with_dtype(ScalarType::Float64));
}

Series make_empty_f64_(std::span<const std::int64_t> sizes) {
  return pluss::make_series(std::vector<double>{}, sizes, Config{}.with_dtype(ScalarType::Float64));
}

std::uint64_t resolve_offset_(std::unordered_map<std::int64_t, std::uint64_t>& state,
                              std::int64_t dims,
                              bool init,
                              std::uint64_t start,
                              bool usetime,
                              std::int64_t points,
                              const char* op) {
  std::uint64_t base = 0;
  if (usetime) {
    base = now_seed_() >> 16U;
  } else if (init || state.find(dims) == state.end()) {
    base = start;
  } else {
    base = state[dims];
  }

  PLUSS_CHECK(points >= 0, std::string(op) + ": points must be >= 0");
  PLUSS_CHECK(base <= std::numeric_limits<std::uint64_t>::max() - static_cast<std::uint64_t>(points),
              std::string(op) + ": offset overflow");
  state[dims] = base + static_cast<std::uint64_t>(points);
  return base;
}

std::array<std::uint64_t, 64> sobol_directions_for_dim_(std::int64_t dim_index) {
  std::array<std::uint64_t, 64> dirs{};
  if (dim_index == 0) {
    for (std::size_t bit = 0; bit < dirs.size(); ++bit) {
      dirs[bit] = std::uint64_t{1} << (63U - static_cast<unsigned>(bit));
    }
    return dirs;
  }

  const std::size_t d = static_cast<std::size_t>(dim_index);
  PLUSS_CHECK(d < detail::sobol1111::kMaxDim,
              "rng::sobol: internal direction lookup out of range");

  const std::uint32_t s = detail::sobol1111::kMjShift[d];
  const std::uint32_t a = detail::sobol1111::kAllA[d];
  const std::size_t row = d * detail::sobol1111::kMaxDeg;

  for (std::uint32_t i = 0; i < s; ++i) {
    const std::uint64_t mi = detail::sobol1111::kInitMj[row + i];
    dirs[i] = mi << (63U - i);
  }

  for (std::size_t i = static_cast<std::size_t>(s); i < dirs.size(); ++i) {
    std::uint64_t value = dirs[i - s] ^ (dirs[i - s] >> s);
    for (std::uint32_t k = 1; k < s; ++k) {
      if (((a >> (s - 1U - k)) & 1U) != 0U) {
        value ^= dirs[i - k];
      }
    }
    dirs[i] = value;
  }

  return dirs;
}

std::uint64_t sobol_uint_(std::uint64_t index, const std::array<std::uint64_t, 64>& dirs) {
  const std::uint64_t gray = index ^ (index >> 1U);
  std::uint64_t x = 0;
  std::uint64_t bits = gray;
  std::size_t bit = 0;
  while (bits != 0U && bit < dirs.size()) {
    if ((bits & 1U) != 0U) {
      x ^= dirs[bit];
    }
    bits >>= 1U;
    ++bit;
  }
  return x;
}

std::uint64_t parity64_(std::uint64_t x) {
  return static_cast<std::uint64_t>(std::popcount(x) & 1U);
}

std::uint64_t apply_linear_scramble_(std::uint64_t x, const std::array<std::uint64_t, 64>& rows) {
  std::uint64_t y = 0;
  for (std::uint32_t bit = 0; bit < 64; ++bit) {
    y |= (parity64_(x & rows[bit]) << bit);
  }
  return y;
}

std::array<std::uint64_t, 64> random_lower_triangular_matrix_(std::mt19937_64& rng) {
  std::array<std::uint64_t, 64> rows{};
  for (std::uint32_t bit = 0; bit < 64; ++bit) {
    std::uint64_t row = rng();
    const std::uint64_t mask = (bit == 63U)
                                   ? std::numeric_limits<std::uint64_t>::max()
                                   : ((std::uint64_t{1} << (bit + 1U)) - 1U);
    row &= mask;
    row |= (std::uint64_t{1} << bit);
    rows[bit] = row;
  }
  return rows;
}

struct SobolScrambleSpec {
  bool has_shift = false;
  bool has_linear = false;
  std::vector<std::uint64_t> shifts;
  std::vector<std::array<std::uint64_t, 64>> matrices;
};

SobolScrambleSpec make_sobol_scramble_(std::int64_t dims, int mode, std::uint64_t seed) {
  SobolScrambleSpec spec;
  if (mode <= 0) return spec;

  std::mt19937_64 scramble_rng(seed ^ 0xD1B54A32D192ED03ULL);
  spec.has_shift = (mode == 1 || mode == 3);
  spec.has_linear = (mode == 2 || mode == 3);

  if (spec.has_shift) {
    spec.shifts.resize(static_cast<std::size_t>(dims), 0U);
    std::generate(spec.shifts.begin(), spec.shifts.end(), [&] { return scramble_rng(); });
  }
  if (spec.has_linear) {
    spec.matrices.reserve(static_cast<std::size_t>(dims));
    for (std::int64_t d = 0; d < dims; ++d) {
      spec.matrices.push_back(random_lower_triangular_matrix_(scramble_rng));
    }
  }

  return spec;
}

inline std::uint64_t scramble_sobol_value_(std::uint64_t x, std::int64_t d, const SobolScrambleSpec& spec) {
  if (spec.has_linear) {
    x = apply_linear_scramble_(x, spec.matrices[static_cast<std::size_t>(d)]);
  }
  if (spec.has_shift) {
    x ^= spec.shifts[static_cast<std::size_t>(d)];
  }
  return x;
}

std::vector<double> sobol_values_(const ShapeSpec& shape, const SobolOptions& opts) {
  if (shape.total == 0) return {};

  PLUSS_CHECK(opts.scrambling >= 0 && opts.scrambling <= 3,
              "rng::sobol: scrambling must be in {0,1,2,3}");
  PLUSS_CHECK(opts.maxit > 0 && opts.maxit <= 1000,
              "rng::sobol: maxit must be in [1,1000]");
  validate_mexp_(opts.mexp, "rng::sobol");

  const bool use_mixed = opts.mixed;
  const std::uint64_t seed = choose_seed_(opts.seed_set, opts.seed, opts.mexp);
  const SobolScrambleSpec scramble = make_sobol_scramble_(shape.dims, opts.scrambling, seed);

  std::uint64_t base = 0;
  if (!use_mixed) {
    base = resolve_offset_(g_sobol_next, shape.dims, opts.init, opts.start, false, shape.points, "rng::sobol");
  }

  const std::int64_t sobol_dims =
      std::min<std::int64_t>(shape.dims, static_cast<std::int64_t>(detail::sobol1111::kMaxDim));

  std::vector<std::array<std::uint64_t, 64>> dirs;
  dirs.reserve(static_cast<std::size_t>(sobol_dims));
  for (std::int64_t d = 0; d < sobol_dims; ++d) {
    dirs.push_back(sobol_directions_for_dim_(d));
  }

  std::vector<std::uint32_t> fallback_primes;
  if (shape.dims > static_cast<std::int64_t>(detail::sobol1111::kMaxDim)) {
    fallback_primes = first_primes_(shape.dims, "rng::sobol");
  }

  std::vector<std::uint64_t> uint_values(static_cast<std::size_t>(shape.total), 0U);

  if (use_mixed) {
    std::mt19937_64 index_rng(seed);
    for (std::int64_t i = 0; i < shape.points; ++i) {
      const std::uint64_t index = index_rng();
      const std::size_t row = static_cast<std::size_t>(i * shape.dims);
      for (std::int64_t d = 0; d < shape.dims; ++d) {
        std::uint64_t x = 0U;
        if (d < sobol_dims) {
          x = sobol_uint_(index, dirs[static_cast<std::size_t>(d)]);
        } else {
          // Fallback beyond 1111 dimensions: continue with Halton-style radical inverse.
          const double u = radical_inverse_(index + 1U, fallback_primes[static_cast<std::size_t>(d)]);
          x = static_cast<std::uint64_t>(u * 18446744073709551616.0);
        }
        uint_values[row + static_cast<std::size_t>(d)] = scramble_sobol_value_(x, d, scramble);
      }
    }
  } else {
    std::vector<std::uint64_t> current(static_cast<std::size_t>(sobol_dims), 0U);
    for (std::int64_t d = 0; d < sobol_dims; ++d) {
      current[static_cast<std::size_t>(d)] = sobol_uint_(base, dirs[static_cast<std::size_t>(d)]);
    }

    for (std::int64_t i = 0; i < shape.points; ++i) {
      const std::uint64_t index = base + static_cast<std::uint64_t>(i);
      const std::size_t row = static_cast<std::size_t>(i * shape.dims);

      for (std::int64_t d = 0; d < sobol_dims; ++d) {
        const std::uint64_t x = current[static_cast<std::size_t>(d)];
        uint_values[row + static_cast<std::size_t>(d)] = scramble_sobol_value_(x, d, scramble);
      }

      for (std::int64_t d = sobol_dims; d < shape.dims; ++d) {
        // Fallback beyond 1111 dimensions: continue with Halton-style radical inverse.
        const double u = radical_inverse_(index + 1U, fallback_primes[static_cast<std::size_t>(d)]);
        const std::uint64_t x = static_cast<std::uint64_t>(u * 18446744073709551616.0);
        uint_values[row + static_cast<std::size_t>(d)] = scramble_sobol_value_(x, d, scramble);
      }

      if (i + 1 < shape.points) {
        const unsigned changed_bit = std::countr_zero(index + 1U);
        for (std::int64_t d = 0; d < sobol_dims; ++d) {
          current[static_cast<std::size_t>(d)] ^= dirs[static_cast<std::size_t>(d)][changed_bit];
        }
      }
    }
  }

  std::vector<double> values(uint_values.size(), 0.0);
  std::transform(uint_values.begin(), uint_values.end(), values.begin(), [](std::uint64_t x) {
    return std::ldexp(static_cast<double>(x), -64);
  });

  return values;
}

std::vector<double> halton_values_(const ShapeSpec& shape, const HaltonOptions& opts) {
  if (shape.total == 0) return {};
  validate_mexp_(opts.mexp, "rng::halton");

  const std::vector<std::uint32_t> primes = first_primes_(shape.dims, "rng::halton");
  std::vector<double> values(static_cast<std::size_t>(shape.total), 0.0);

  if (opts.mixed) {
    std::mt19937_64 rng(choose_seed_(opts.seed_set, opts.seed, opts.mexp));
    for (std::int64_t i = 0; i < shape.points; ++i) {
      const std::uint64_t index = rng() >> 32U;
      for (std::int64_t d = 0; d < shape.dims; ++d) {
        values[static_cast<std::size_t>(i * shape.dims + d)] =
            radical_inverse_(index, primes[static_cast<std::size_t>(d)]);
      }
    }
    return values;
  }

  const std::uint64_t base = resolve_offset_(
      g_halton_next,
      shape.dims,
      opts.init,
      opts.start,
      opts.usetime,
      shape.points,
      "rng::halton");

  for (std::int64_t i = 0; i < shape.points; ++i) {
    const std::uint64_t index = base + static_cast<std::uint64_t>(i);
    for (std::int64_t d = 0; d < shape.dims; ++d) {
      values[static_cast<std::size_t>(i * shape.dims + d)] =
          radical_inverse_(index, primes[static_cast<std::size_t>(d)]);
    }
  }

  return values;
}

std::vector<double> torus_values_(ShapeSpec shape, const TorusOptions& opts) {
  if (shape.total == 0) return {};
  validate_mexp_(opts.mexp, "rng::torus");

  std::vector<double> prime_values;
  if (!opts.prime.empty()) {
    prime_values.reserve(opts.prime.size());
    for (const std::int64_t p : opts.prime) {
      PLUSS_CHECK(p > 0, "rng::torus: prime values must be > 0");
      prime_values.push_back(static_cast<double>(p));
    }
    force_dims_(shape, static_cast<std::int64_t>(prime_values.size()), "rng::torus");
  } else {
    const auto primes_u32 = first_primes_(shape.dims, "rng::torus");
    prime_values.assign(primes_u32.begin(), primes_u32.end());
  }

  std::vector<double> values(static_cast<std::size_t>(shape.total), 0.0);
  std::vector<double> root_prime(static_cast<std::size_t>(shape.dims), 0.0);
  std::transform(prime_values.begin(), prime_values.begin() + static_cast<std::ptrdiff_t>(shape.dims), root_prime.begin(),
                 [](double p) { return std::sqrt(p); });

  if (opts.mixed) {
    std::mt19937_64 rng(choose_seed_(opts.seed_set, opts.seed, opts.mexp));
    for (std::int64_t i = 0; i < shape.points; ++i) {
      const std::uint64_t index = rng() >> 32U;
      const double idx = static_cast<double>(index);
      for (std::int64_t d = 0; d < shape.dims; ++d) {
        const double x = idx * root_prime[static_cast<std::size_t>(d)];
        values[static_cast<std::size_t>(i * shape.dims + d)] = x - std::floor(x);
      }
    }
    return values;
  }

  const std::uint64_t base = resolve_offset_(
      g_torus_next,
      shape.dims,
      opts.init,
      opts.start,
      opts.usetime,
      shape.points,
      "rng::torus");

  for (std::int64_t i = 0; i < shape.points; ++i) {
    const double idx = static_cast<double>(base + static_cast<std::uint64_t>(i));
    for (std::int64_t d = 0; d < shape.dims; ++d) {
      const double x = idx * root_prime[static_cast<std::size_t>(d)];
      values[static_cast<std::size_t>(i * shape.dims + d)] = x - std::floor(x);
    }
  }

  return values;
}

} // namespace

const char* method_name(Method method) noexcept {
  switch (method) {
    case Method::Pseudo:
      return "pseudo";
    case Method::Sobol:
      return "sobol";
    case Method::Halton:
      return "halton";
    case Method::Torus:
      return "torus";
    default:
      return "unknown";
  }
}

bool is_low_discrepancy(Method method) noexcept {
  return method == Method::Sobol || method == Method::Halton || method == Method::Torus;
}

Series sobol(std::span<const std::int64_t> sizes, const SobolOptions& opts, Config cfg) {
  const ScalarType out_dtype = resolve_float_dtype_(cfg.dtype, "rng::sobol");
  const ShapeSpec shape = decode_shape_(sizes, "rng::sobol");

  if (shape.total == 0) {
    return make_empty_f64_(shape.sizes).to(cfg.platform, out_dtype);
  }

  const std::vector<double> values = sobol_values_(shape, opts);
  Series out = make_f64_series_(values, shape.sizes);
  if (opts.normal) {
    out = to_standard_normal_(out, cfg, "rng::sobol");
    return out;
  }
  return out.to(cfg.platform, out_dtype);
}

Series sobol(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t offset) {
  SobolOptions opts;
  opts.init = true;
  opts.start = offset;
  return sobol(sizes, opts, cfg);
}

Series halton(std::span<const std::int64_t> sizes, const HaltonOptions& opts, Config cfg) {
  const ScalarType out_dtype = resolve_float_dtype_(cfg.dtype, "rng::halton");
  const ShapeSpec shape = decode_shape_(sizes, "rng::halton");

  if (shape.total == 0) {
    return make_empty_f64_(shape.sizes).to(cfg.platform, out_dtype);
  }

  const std::vector<double> values = halton_values_(shape, opts);
  Series out = make_f64_series_(values, shape.sizes);
  if (opts.normal) {
    out = to_standard_normal_(out, cfg, "rng::halton");
    return out;
  }
  return out.to(cfg.platform, out_dtype);
}

Series halton(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t offset) {
  HaltonOptions opts;
  opts.init = true;
  opts.start = offset + 1U;
  return halton(sizes, opts, cfg);
}

Series torus(std::span<const std::int64_t> sizes, const TorusOptions& opts, Config cfg) {
  const ScalarType out_dtype = resolve_float_dtype_(cfg.dtype, "rng::torus");
  ShapeSpec shape = decode_shape_(sizes, "rng::torus");
  if (!opts.prime.empty()) {
    force_dims_(shape, static_cast<std::int64_t>(opts.prime.size()), "rng::torus");
  }

  if (shape.total == 0) {
    return make_empty_f64_(shape.sizes).to(cfg.platform, out_dtype);
  }

  const std::vector<double> values = torus_values_(shape, opts);
  Series out = make_f64_series_(values, shape.sizes);
  if (opts.normal) {
    out = to_standard_normal_(out, cfg, "rng::torus");
    return out;
  }
  return out.to(cfg.platform, out_dtype);
}

Series torus(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t offset) {
  TorusOptions opts;
  opts.init = true;
  opts.start = offset + 1U;
  return torus(sizes, opts, cfg);
}

Series sobol_normal(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t offset) {
  SobolOptions opts;
  opts.init = true;
  opts.start = offset;
  opts.normal = true;
  return sobol(sizes, opts, cfg);
}

Series halton_normal(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t offset) {
  HaltonOptions opts;
  opts.init = true;
  opts.start = offset + 1U;
  opts.normal = true;
  return halton(sizes, opts, cfg);
}

Series torus_normal(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t offset) {
  TorusOptions opts;
  opts.init = true;
  opts.start = offset + 1U;
  opts.normal = true;
  return torus(sizes, opts, cfg);
}

Series uniform(std::span<const std::int64_t> sizes,
               Method method,
               Config cfg,
               std::uint64_t seed_or_offset) {
  const ScalarType out_dtype = resolve_float_dtype_(cfg.dtype, "rng::uniform");
  const Config out_cfg = Config{}
                             .with_dtype(out_dtype)
                             .with_platform(cfg.platform)
                             .with_pinned(cfg.pinned);

  switch (method) {
    case Method::Pseudo:
      return pluss::series::rand(sizes, out_cfg, seed_or_offset);
    case Method::Sobol:
      return sobol(sizes, out_cfg, seed_or_offset);
    case Method::Halton:
      return halton(sizes, out_cfg, seed_or_offset);
    case Method::Torus:
      return torus(sizes, out_cfg, seed_or_offset);
    default:
      PLUSS_CHECK(false, "rng::uniform: unsupported method");
      return pluss::series::rand(sizes, out_cfg, seed_or_offset);
  }
}

Series standard_normal(std::span<const std::int64_t> sizes,
                       Method method,
                       Config cfg,
                       std::uint64_t seed_or_offset) {
  const ScalarType out_dtype = resolve_float_dtype_(cfg.dtype, "rng::standard_normal");
  const Config out_cfg = Config{}
                             .with_dtype(out_dtype)
                             .with_platform(cfg.platform)
                             .with_pinned(cfg.pinned);

  switch (method) {
    case Method::Pseudo:
      return pluss::series::randn(sizes, out_cfg, seed_or_offset);
    case Method::Sobol:
      return sobol_normal(sizes, out_cfg, seed_or_offset);
    case Method::Halton:
      return halton_normal(sizes, out_cfg, seed_or_offset);
    case Method::Torus:
      return torus_normal(sizes, out_cfg, seed_or_offset);
    default:
      PLUSS_CHECK(false, "rng::standard_normal: unsupported method");
      return pluss::series::randn(sizes, out_cfg, seed_or_offset);
  }
}

} // namespace pluss::subsp::stats::rng

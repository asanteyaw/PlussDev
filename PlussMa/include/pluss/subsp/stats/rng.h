#pragma once

#include "pluss/codem/codem.h"
#include "pluss/config.h"
#include "pluss/export.h"

#include <cstdint>
#include <initializer_list>
#include <span>
#include <vector>

namespace pluss::subsp::stats::rng {

enum class Method : std::uint8_t {
  Pseudo = 0,
  Sobol = 1,
  Halton = 2,
  Torus = 3,
};

struct SobolOptions {
  bool init = true;
  int scrambling = 0;       // 0:none, 1:Owen-style digital shift, 2:Faure-Tezuka-style linear scramble, 3:both
  bool mixed = false;       // randomized index stream
  bool normal = false;      // qnorm transform
  std::uint64_t start = 1;  // first index when init=true
  std::uint64_t seed = 4711;
  bool seed_set = false;
  int maxit = 10;           // compatibility argument
  int mexp = 19937;         // compatibility argument for mixed mode
};

struct HaltonOptions {
  bool init = true;
  bool normal = false;      // qnorm transform
  bool usetime = false;
  bool mixed = false;       // randomized index stream
  std::uint64_t start = 1;
  std::uint64_t seed = 0;
  bool seed_set = false;
  int mexp = 19937;         // compatibility argument
};

struct TorusOptions {
  bool init = true;
  bool mixed = false;       // randomized index stream
  bool usetime = false;
  bool normal = false;      // qnorm transform
  std::uint64_t start = 1;
  std::uint64_t seed = 0;
  bool seed_set = false;
  int mexp = 19937;         // compatibility argument
  std::vector<std::int64_t> prime; // optional custom primes; dim inferred from prime.size()
};

CODEM_API const char* method_name(Method method) noexcept;
CODEM_API bool is_low_discrepancy(Method method) noexcept;

CODEM_API pluss::codem::Series sobol(std::span<const std::int64_t> sizes,
                                     pluss::codem::Config cfg = {},
                                     std::uint64_t offset = 0);
CODEM_API pluss::codem::Series sobol(std::span<const std::int64_t> sizes,
                                     const SobolOptions& opts,
                                     pluss::codem::Config cfg = {});
CODEM_API pluss::codem::Series halton(std::span<const std::int64_t> sizes,
                                      pluss::codem::Config cfg = {},
                                      std::uint64_t offset = 0);
CODEM_API pluss::codem::Series halton(std::span<const std::int64_t> sizes,
                                      const HaltonOptions& opts,
                                      pluss::codem::Config cfg = {});
CODEM_API pluss::codem::Series torus(std::span<const std::int64_t> sizes,
                                     pluss::codem::Config cfg = {},
                                     std::uint64_t offset = 0);
CODEM_API pluss::codem::Series torus(std::span<const std::int64_t> sizes,
                                     const TorusOptions& opts,
                                     pluss::codem::Config cfg = {});

CODEM_API pluss::codem::Series sobol_normal(std::span<const std::int64_t> sizes,
                                            pluss::codem::Config cfg = {},
                                            std::uint64_t offset = 0);
CODEM_API pluss::codem::Series halton_normal(std::span<const std::int64_t> sizes,
                                             pluss::codem::Config cfg = {},
                                             std::uint64_t offset = 0);
CODEM_API pluss::codem::Series torus_normal(std::span<const std::int64_t> sizes,
                                            pluss::codem::Config cfg = {},
                                            std::uint64_t offset = 0);

CODEM_API pluss::codem::Series uniform(std::span<const std::int64_t> sizes,
                                       Method method = Method::Pseudo,
                                       pluss::codem::Config cfg = {},
                                       std::uint64_t seed_or_offset = 0);
CODEM_API pluss::codem::Series standard_normal(std::span<const std::int64_t> sizes,
                                               Method method = Method::Pseudo,
                                               pluss::codem::Config cfg = {},
                                               std::uint64_t seed_or_offset = 0);

inline pluss::codem::Series sobol(std::initializer_list<std::int64_t> sizes,
                                  pluss::codem::Config cfg = {},
                                  std::uint64_t offset = 0) {
  return sobol(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, offset);
}

inline pluss::codem::Series sobol(std::initializer_list<std::int64_t> sizes,
                                  const SobolOptions& opts,
                                  pluss::codem::Config cfg = {}) {
  return sobol(std::span<const std::int64_t>(sizes.begin(), sizes.size()), opts, cfg);
}

inline pluss::codem::Series halton(std::initializer_list<std::int64_t> sizes,
                                   pluss::codem::Config cfg = {},
                                   std::uint64_t offset = 0) {
  return halton(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, offset);
}

inline pluss::codem::Series halton(std::initializer_list<std::int64_t> sizes,
                                   const HaltonOptions& opts,
                                   pluss::codem::Config cfg = {}) {
  return halton(std::span<const std::int64_t>(sizes.begin(), sizes.size()), opts, cfg);
}

inline pluss::codem::Series torus(std::initializer_list<std::int64_t> sizes,
                                  pluss::codem::Config cfg = {},
                                  std::uint64_t offset = 0) {
  return torus(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, offset);
}

inline pluss::codem::Series torus(std::initializer_list<std::int64_t> sizes,
                                  const TorusOptions& opts,
                                  pluss::codem::Config cfg = {}) {
  return torus(std::span<const std::int64_t>(sizes.begin(), sizes.size()), opts, cfg);
}

inline pluss::codem::Series sobol_normal(std::initializer_list<std::int64_t> sizes,
                                         pluss::codem::Config cfg = {},
                                         std::uint64_t offset = 0) {
  return sobol_normal(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, offset);
}

inline pluss::codem::Series halton_normal(std::initializer_list<std::int64_t> sizes,
                                          pluss::codem::Config cfg = {},
                                          std::uint64_t offset = 0) {
  return halton_normal(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, offset);
}

inline pluss::codem::Series torus_normal(std::initializer_list<std::int64_t> sizes,
                                         pluss::codem::Config cfg = {},
                                         std::uint64_t offset = 0) {
  return torus_normal(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, offset);
}

inline pluss::codem::Series uniform(std::initializer_list<std::int64_t> sizes,
                                    Method method = Method::Pseudo,
                                    pluss::codem::Config cfg = {},
                                    std::uint64_t seed_or_offset = 0) {
  return uniform(std::span<const std::int64_t>(sizes.begin(), sizes.size()), method, cfg, seed_or_offset);
}

inline pluss::codem::Series standard_normal(std::initializer_list<std::int64_t> sizes,
                                            Method method = Method::Pseudo,
                                            pluss::codem::Config cfg = {},
                                            std::uint64_t seed_or_offset = 0) {
  return standard_normal(std::span<const std::int64_t>(sizes.begin(), sizes.size()), method, cfg, seed_or_offset);
}

} // namespace pluss::subsp::stats::rng

namespace pluss::ma {
namespace rng = ::pluss::subsp::stats::rng;
}

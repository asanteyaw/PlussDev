#pragma once

#include "pluss/codem/codem.h"

#include <cstdint>
#include <utility>

namespace pluss::subsp::math::qp {

struct Options {
  std::int64_t max_iters{4000};
  double rho{0.1};
  double sigma{1e-6};
  double alpha{1.6};
  double abs_tol{1e-5};
  double rel_tol{1e-5};
  std::int64_t check_every{10};
  // Residual balancing: at each check the penalty rho is rescaled so the primal and dual residuals stay comparable, and the
  // system is refactorised when it changes by more than `adaptive_rho_tolerance` times. `rho` is then only the starting value.
  bool adaptive_rho{true};
  double adaptive_rho_tolerance{5.0};
};

struct WarmStart {
  Series x{};
  Series z{};
  Series y{};
};

struct Result {
  Series x;
  Series objective;
  Series z;
  Series y;
  double primal_residual{};
  double dual_residual{};
  std::int64_t iterations{};
  bool converged{};

  WarmStart warm() const { return {x, z, y}; }
};

class Solver final {
public:
  Result solve(const Series &q, const Series &lower, const Series &upper,
               WarmStart warm = {}) const;

  std::int64_t variables() const noexcept { return variables_; }
  std::int64_t constraints() const noexcept { return constraints_; }
  ScalarType dtype() const noexcept { return dtype_; }
  Platform platform() const noexcept { return p_.platform(); }

private:
  friend Solver prepare(const Series &, const Series &, Options);

  Solver(Series p, Series a, Series at, Series factor, Options options,
         ScalarType dtype, std::int64_t variables, std::int64_t constraints)
      : p_(std::move(p)), a_(std::move(a)), at_(std::move(at)),
        factor_(std::move(factor)), options_(options), dtype_(dtype),
        variables_(variables), constraints_(constraints) {}

  Series p_;
  Series a_;
  Series at_;
  Series factor_;
  Options options_;
  ScalarType dtype_{};
  std::int64_t variables_{};
  std::int64_t constraints_{};
};

// Validates and factorizes the invariant P/A portion once. Reuse the returned
// solver when q or bounds change but dimensions and quadratic terms do not.
Solver prepare(const Series &p, const Series &a, Options options = {});

// Solves 0.5*x^T*P*x + q^T*x subject to lower <= A*x <= upper.
// Leading dimensions are batch dimensions and follow Series broadcasting.
Result solve(const Series &p, const Series &q, const Series &a,
             const Series &lower, const Series &upper, Options options = {},
             WarmStart warm = {});

} // namespace pluss::subsp::math::qp

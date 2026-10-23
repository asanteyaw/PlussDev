#pragma once

#include "pluss/codem/codem.h"
#include "pluss/subsp/math/krylov.h"
#include "pluss/subsp/math/linear_operator.h"

#include <cstdint>
#include <functional>
#include <limits>
#include <utility>
#include <vector>

namespace pluss::subsp::natsci::common::quantum {

namespace detail {
struct Access;
}

class Ket final {
public:
  [[nodiscard]] const Series &value() const noexcept { return value_; }
  [[nodiscard]] const std::vector<std::int64_t> &spaces() const noexcept {
    return spaces_;
  }
  [[nodiscard]] std::int64_t size() const noexcept { return value_.size(-1); }
  [[nodiscard]] std::vector<std::int64_t> batch_shape() const;

private:
  Ket(Series value, std::vector<std::int64_t> spaces)
      : value_(std::move(value)), spaces_(std::move(spaces)) {}

  Series value_;
  std::vector<std::int64_t> spaces_;

  friend struct detail::Access;
};

class Op final {
public:
  [[nodiscard]] const Series &value() const noexcept { return value_; }
  [[nodiscard]] const std::vector<std::int64_t> &spaces() const noexcept {
    return spaces_;
  }
  [[nodiscard]] std::int64_t size() const noexcept { return value_.size(-1); }
  [[nodiscard]] std::vector<std::int64_t> batch_shape() const;

private:
  Op(Series value, std::vector<std::int64_t> spaces)
      : value_(std::move(value)), spaces_(std::move(spaces)) {}

  Series value_;
  std::vector<std::int64_t> spaces_;

  friend struct detail::Access;
};

class Channel final {
public:
  [[nodiscard]] const std::vector<Op> &kraus() const noexcept { return kraus_; }
  [[nodiscard]] const std::vector<std::int64_t> &spaces() const noexcept {
    return spaces_;
  }

private:
  Channel(std::vector<Op> kraus, std::vector<std::int64_t> spaces)
      : kraus_(std::move(kraus)), spaces_(std::move(spaces)) {}

  std::vector<Op> kraus_;
  std::vector<std::int64_t> spaces_;

  friend struct detail::Access;
};

class SparseOp final {
public:
  [[nodiscard]] const pluss::subsp::math::linear_operator::CooMap &
  map() const noexcept {
    return map_;
  }
  [[nodiscard]] const std::vector<std::int64_t> &spaces() const noexcept {
    return spaces_;
  }
  [[nodiscard]] std::int64_t size() const noexcept { return map_.rows(); }
  [[nodiscard]] bool hermitian() const noexcept { return hermitian_; }

private:
  SparseOp(pluss::subsp::math::linear_operator::CooMap map,
           std::vector<std::int64_t> spaces, bool hermitian)
      : map_(std::move(map)), spaces_(std::move(spaces)),
        hermitian_(hermitian) {}

  pluss::subsp::math::linear_operator::CooMap map_;
  std::vector<std::int64_t> spaces_;
  bool hermitian_{};

  friend struct detail::Access;
};

struct Spectrum {
  Series values = pluss::series::undefined();
  Op vectors;
};

enum class SolveMethod : std::uint8_t {
  RK4 = 0,
  RK45 = 1,
  ImplicitMidpoint = 2,
  Krylov = 3,
};

struct SolveOptions {
  SolveMethod method{SolveMethod::RK4};
  std::int64_t steps_per_span{16};
  double hbar{1.0};
  double relative_tolerance{1e-7};
  double absolute_tolerance{1e-10};
  double initial_step{0.0};
  double minimum_step{0.0};
  double maximum_step{std::numeric_limits<double>::infinity()};
  std::int64_t max_steps{100000};
  pluss::subsp::math::krylov::GmresOptions linear{};
};

using OpFn = std::function<Op(double)>;
using CollapseFn = std::function<std::vector<Op>(double)>;

[[nodiscard]] Ket ket(Series amplitudes, std::vector<std::int64_t> spaces = {});
[[nodiscard]] Op op(Series values, std::vector<std::int64_t> spaces = {});
[[nodiscard]] SparseOp sparse_op(std::int64_t size, Series row_index,
                                 Series column_index, Series values,
                                 std::vector<std::int64_t> spaces = {},
                                 bool hermitian = false);

[[nodiscard]] Series norm(const Ket &state);
[[nodiscard]] Ket normalize(const Ket &state, double tolerance = 0.0);
[[nodiscard]] Series inner(const Ket &left, const Ket &right);
[[nodiscard]] Series prob(const Ket &state);

[[nodiscard]] Op dag(const Op &value);
[[nodiscard]] Series trace(const Op &value);
[[nodiscard]] Series is_herm(const Op &value, double rtol = 1e-7,
                             double atol = 1e-9);

[[nodiscard]] Op density(const Ket &state);
[[nodiscard]] Series expect(const Op &observable, const Ket &state);
[[nodiscard]] Series expect(const Op &observable, const Op &state);
[[nodiscard]] Ket apply(const SparseOp &value, const Ket &state);
[[nodiscard]] Series purity(const Op &state);
[[nodiscard]] Spectrum spectrum(const Op &value, double tolerance = 1e-9);
[[nodiscard]] Ket ground(const Op &value, double tolerance = 1e-9);
[[nodiscard]] Series entropy(const Op &state, double base = 2.0,
                             double tolerance = 1e-9);
[[nodiscard]] Series root_fidelity(const Ket &left, const Ket &right);
[[nodiscard]] Series root_fidelity(const Ket &left, const Op &right,
                                   double tolerance = 1e-9);
[[nodiscard]] Series root_fidelity(const Op &left, const Op &right,
                                   double tolerance = 1e-9);
[[nodiscard]] Series fidelity(const Ket &left, const Ket &right);
[[nodiscard]] Series fidelity(const Ket &left, const Op &right,
                              double tolerance = 1e-9);
[[nodiscard]] Series fidelity(const Op &left, const Op &right,
                              double tolerance = 1e-9);

[[nodiscard]] Ket tensor(const Ket &left, const Ket &right);
[[nodiscard]] Op tensor(const Op &left, const Op &right);
[[nodiscard]] Op partial_trace(const Op &state, std::vector<std::int64_t> keep);

[[nodiscard]] Op unitary(const Op &hamiltonian, double time, double hbar = 1.0);
[[nodiscard]] Ket schrod(const Op &hamiltonian, const Ket &initial,
                         const Series &times, double hbar = 1.0);
[[nodiscard]] Ket schrod(const OpFn &hamiltonian, const Ket &initial,
                         const Series &times, const SolveOptions &options = {});
[[nodiscard]] Ket schrod(const SparseOp &hamiltonian, const Ket &initial,
                         const Series &times, const SolveOptions &options = {});

[[nodiscard]] Channel channel(std::vector<Op> kraus, double tolerance = 1e-9);
[[nodiscard]] Op apply(const Channel &map, const Ket &state);
[[nodiscard]] Op apply(const Channel &map, const Op &state);

[[nodiscard]] Op lindblad(const Op &hamiltonian, const Op &initial,
                          const Series &times,
                          const std::vector<Op> &collapse = {},
                          const SolveOptions &options = {});
[[nodiscard]] Op lindblad(const OpFn &hamiltonian, const Op &initial,
                          const Series &times, const CollapseFn &collapse = {},
                          const SolveOptions &options = {});
[[nodiscard]] Op lindblad(const SparseOp &hamiltonian, const Op &initial,
                          const Series &times,
                          const std::vector<SparseOp> &collapse = {},
                          const SolveOptions &options = {});

} // namespace pluss::subsp::natsci::common::quantum

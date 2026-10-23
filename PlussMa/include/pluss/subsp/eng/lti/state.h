#pragma once

#include "pluss/codem/codem.h"
#include "pluss/subsp/linearalg/linearalg_fwd.h"

#include <cstdint>
#include <utility>

namespace pluss::subsp::eng::lti {

enum class Gram : std::uint8_t {
  Reach = 0,
  Observe = 1,
};

class StateSpace final {
public:
  const Series &a() const noexcept { return a_; }
  const Series &b() const noexcept { return b_; }
  const Series &c() const noexcept { return c_; }
  const Series &d() const noexcept { return d_; }

  double sample_time() const noexcept { return sample_time_; }
  bool is_cont() const noexcept { return sample_time_ == 0.0; }
  bool is_disc() const noexcept { return sample_time_ > 0.0; }

  std::int64_t states() const { return a_.size(-1); }
  std::int64_t inputs() const { return b_.size(-1); }
  std::int64_t outputs() const { return c_.size(-2); }

private:
  friend StateSpace state(const Series &, const Series &, const Series &,
                          const Series &, double);

  StateSpace(Series a, Series b, Series c, Series d, double sample_time)
      : a_(std::move(a)), b_(std::move(b)), c_(std::move(c)), d_(std::move(d)),
        sample_time_(sample_time) {}

  Series a_;
  Series b_;
  Series c_;
  Series d_;
  double sample_time_{};
};

StateSpace state(const Series &a, const Series &b, const Series &c,
                 const Series &d, double sample_time = 0.0);

Series ctrb(const StateSpace &system);
Series obsv(const StateSpace &system);
Series gram(const StateSpace &system, Gram side,
            pluss::subsp::linearalg::MatrixEqOptions options = {});

} // namespace pluss::subsp::eng::lti

#pragma once

#include "pluss/subsp/eng/control/mpc.h"

#include "pluss/codem/codem.h"
#include "pluss/subsp/eng/lti/pole_zero.h"
#include "pluss/subsp/eng/lti/response.h"
#include "pluss/subsp/eng/lti/state.h"
#include "pluss/subsp/linearalg/linearalg_fwd.h"
#include "pluss/subsp/math/state.h"

#include <cstdint>
#include <utility>

namespace pluss::subsp::eng::control {

struct RiccatiResult {
  Series value = pluss::series::undefined();
  Series gain = pluss::series::undefined();
  Series closed_loop = pluss::series::undefined();
};

struct LqeResult {
  Series covariance = pluss::series::undefined();
  Series gain = pluss::series::undefined();
  Series closed_loop = pluss::series::undefined();
};

struct LoopResult {
  Series response = pluss::series::undefined();
  Series sens = pluss::series::undefined();
  Series comp = pluss::series::undefined();
};

struct MarginResult {
  Series gain = pluss::series::undefined();
  // Phase is measured in degrees; all frequency fields are angular.
  Series phase = pluss::series::undefined();
  // Minimum sampled distance from the Nyquist response to -1.
  Series stability = pluss::series::undefined();
  Series gain_freq = pluss::series::undefined();
  Series phase_freq = pluss::series::undefined();
  Series stability_freq = pluss::series::undefined();
};

struct PlaceOptions {
  std::int64_t sweeps = 5;
  double tolerance = 1e-10;
  double condition_limit = 1e12;
};

struct PlaceResult {
  Series gain = pluss::series::undefined();
  Series closed_loop = pluss::series::undefined();
  Series basis = pluss::series::undefined();
  Series residual = pluss::series::undefined();
  Series condition = pluss::series::undefined();
  std::int64_t sweeps = 0;
};

enum class PidRule : std::uint8_t {
  Forward = 0,
  Backward = 1,
  Trapezoid = 2,
};

class Pid final {
public:
  const Series &kp() const noexcept { return kp_; }
  const Series &ki() const noexcept { return ki_; }
  const Series &kd() const noexcept { return kd_; }
  const Series &filter() const noexcept { return filter_; }
  double sample_time() const noexcept { return sample_time_; }
  bool is_cont() const noexcept { return sample_time_ == 0.0; }
  bool is_disc() const noexcept { return sample_time_ > 0.0; }
  PidRule int_rule() const noexcept { return int_rule_; }
  PidRule diff_rule() const noexcept { return diff_rule_; }

private:
  friend Pid pid(const Series &, const Series &, const Series &, const Series &,
                 double, PidRule, PidRule);

  Pid(Series kp, Series ki, Series kd, Series filter, double sample_time,
      PidRule int_rule, PidRule diff_rule)
      : kp_(std::move(kp)), ki_(std::move(ki)), kd_(std::move(kd)),
        filter_(std::move(filter)), sample_time_(sample_time),
        int_rule_(int_rule), diff_rule_(diff_rule) {}

  Series kp_;
  Series ki_;
  Series kd_;
  Series filter_;
  double sample_time_{};
  PidRule int_rule_{PidRule::Forward};
  PidRule diff_rule_{PidRule::Forward};
};

using KalmanOptions = pluss::subsp::math::state::FilterOptions;
using KalmanResult = pluss::subsp::math::state::FilterResult;
using SmoothResult = pluss::subsp::math::state::SmoothResult;

RiccatiResult care(const Series &a, const Series &b, const Series &q,
                   const Series &r,
                   pluss::subsp::linearalg::MatrixEqOptions options = {});
RiccatiResult care(const Series &a, const Series &b, const Series &q,
                   const Series &r, const Series &cross,
                   pluss::subsp::linearalg::MatrixEqOptions options = {});

RiccatiResult dare(const Series &a, const Series &b, const Series &q,
                   const Series &r,
                   pluss::subsp::linearalg::MatrixEqOptions options = {});
RiccatiResult dare(const Series &a, const Series &b, const Series &q,
                   const Series &r, const Series &cross,
                   pluss::subsp::linearalg::MatrixEqOptions options = {});

RiccatiResult lqr(const lti::StateSpace &system, const Series &q,
                  const Series &r,
                  pluss::subsp::linearalg::MatrixEqOptions options = {});
RiccatiResult lqr(const lti::StateSpace &system, const Series &q,
                  const Series &r, const Series &cross,
                  pluss::subsp::linearalg::MatrixEqOptions options = {});

LqeResult lqe(const lti::StateSpace &system, const Series &process_cov,
              const Series &noise_cov,
              pluss::subsp::linearalg::MatrixEqOptions options = {});
LqeResult lqe(const lti::StateSpace &system, const Series &process_cov,
              const Series &noise_cov, const Series &cross,
              pluss::subsp::linearalg::MatrixEqOptions options = {});

KalmanResult kalman(const lti::StateSpace &system, const Series &observations,
                    const Series &process_cov, const Series &noise_cov,
                    const Series &initial_mean, const Series &initial_cov,
                    KalmanOptions options = {});
KalmanResult kalman(const lti::StateSpace &system, const Series &observations,
                    const Series &input, const Series &process_cov,
                    const Series &noise_cov, const Series &initial_mean,
                    const Series &initial_cov, KalmanOptions options = {});

SmoothResult smooth(const lti::StateSpace &system, const Series &observations,
                    const Series &process_cov, const Series &noise_cov,
                    const Series &initial_mean, const Series &initial_cov,
                    KalmanOptions options = {});
SmoothResult smooth(const lti::StateSpace &system, const Series &observations,
                    const Series &input, const Series &process_cov,
                    const Series &noise_cov, const Series &initial_mean,
                    const Series &initial_cov, KalmanOptions options = {});

LoopResult loop(const lti::StateSpace &system, const Series &omega);
LoopResult loop(const lti::PoleZero &system, const Series &omega);

// Frequency-data margins interpolate crossings between adjacent samples.
MarginResult margin(const Series &response, const Series &omega);
MarginResult margin(const lti::StateSpace &system, const Series &omega);
MarginResult margin(const lti::PoleZero &system, const Series &omega);

// Assigns the poles of A - B K. Desired poles are shared by every leading
// system batch and must be conjugate-paired when A and B are real.
PlaceResult place(const Series &a, const Series &b, const Series &poles,
                  PlaceOptions options = {});
PlaceResult place(const lti::StateSpace &system, const Series &poles,
                  PlaceOptions options = {});

// Parallel form: Kp + Ki/s + Kd*s/(filter*s + 1). For discrete controllers,
// the selected rules replace 1/s in the integral and filtered derivative.
Pid pid(const Series &kp, const Series &ki, const Series &kd,
        const Series &filter, double sample_time = 0.0,
        PidRule int_rule = PidRule::Forward,
        PidRule diff_rule = PidRule::Forward);
Series freqresp(const Pid &controller, const Series &omega);

} // namespace pluss::subsp::eng::control

#include "pluss/subsp/eng/control.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"

#include <cmath>
#include <limits>
#include <string_view>

namespace pluss::subsp::eng::control {
namespace {

constexpr double kPi = 3.141592653589793238462643383279502884;

bool finite_complex(const Series &value) {
  return value.real()
      .isfinite()
      .logical_and(value.imag().isfinite())
      .all()
      .item<bool>();
}

void require_grid(const Series &omega, Platform platform,
                  std::string_view operation) {
  const std::string prefix = "eng::control::" + std::string(operation) + ": ";
  PLUSS_CHECK(omega.defined(), prefix + "omega must be defined");
  PLUSS_CHECK(omega.dim() == 1 && omega.numel() >= 2,
              prefix + "omega must be a vector with at least two samples");
  PLUSS_CHECK(omega.dtype().is_floating() && !omega.dtype().is_complex(),
              prefix + "omega must have a real floating dtype");
  PLUSS_CHECK(omega.platform() == platform,
              prefix + "omega and the response must share one platform");
  PLUSS_CHECK(omega.isfinite().all().item<bool>(),
              prefix + "omega must contain only finite values");
  PLUSS_CHECK(omega.ge(0.0).all().item<bool>(),
              prefix + "omega must be non-negative");
  PLUSS_CHECK(omega.slice(0, 1, omega.numel(), 1)
                  .gt(omega.slice(0, 0, omega.numel() - 1, 1))
                  .all()
                  .item<bool>(),
              prefix + "omega must be strictly increasing");
}

void require_response(const Series &response, const Series &omega) {
  require_grid(omega, response.platform(), "margin");
  PLUSS_CHECK(response.defined(),
              "eng::control::margin: response must be defined");
  PLUSS_CHECK(
      response.dim() >= 1 && response.size(-1) == omega.numel(),
      "eng::control::margin: response must have shape [..., frequency]");
  PLUSS_CHECK(response.dtype().is_complex(),
              "eng::control::margin: response must have a complex dtype");
  PLUSS_CHECK(finite_complex(response),
              "eng::control::margin: response must contain only finite values");
}

void require_nyquist(const Series &omega, double sample_time) {
  if (sample_time == 0.0)
    return;
  const double limit = kPi / sample_time;
  const double tolerance = 32.0 * std::numeric_limits<double>::epsilon() *
                           std::max(1.0, std::abs(limit));
  PLUSS_CHECK(
      omega.le(limit + tolerance).all().item<bool>(),
      "eng::control::margin: a discrete frequency grid cannot exceed Nyquist");
}

struct Picked {
  Series value;
  Series frequency;
};

Picked pick_margin(const Series &candidate, const Series &frequency,
                   const Series &mask, const Series &score,
                   double absent_value) {
  const Series ranked = mask.where(
      score,
      pluss::series::full_like(score, std::numeric_limits<double>::infinity()));
  const Series index = ranked.argmin(-1, true);
  const Series present = mask.any(-1, false);
  const Series selected = candidate.gather(-1, index).squeeze(-1);
  const Series selected_frequency = frequency.gather(-1, index).squeeze(-1);
  return {
      present.where(selected, pluss::series::full_like(selected, absent_value)),
      present.where(
          selected_frequency,
          pluss::series::full_like(selected_frequency,
                                   std::numeric_limits<double>::quiet_NaN())),
  };
}

Series safe_fraction(const Series &numerator, const Series &denominator,
                     double tolerance) {
  const Series usable = denominator.abs().gt(tolerance);
  const Series safe_denominator =
      usable.where(denominator, pluss::series::ones_like(denominator));
  return usable.where(numerator.div(safe_denominator),
                      pluss::series::zeros_like(numerator));
}

MarginResult margin_impl(const Series &response, const Series &omega) {
  require_response(response, omega);
  const std::int64_t samples = omega.numel();
  const double tolerance =
      response.scalar_type() == ScalarType::Complex128 ? 1e-14 : 2e-6;

  const Series left = response.slice(-1, 0, samples - 1, 1);
  const Series right = response.slice(-1, 1, samples, 1);
  const Series omega_left = omega.slice(0, 0, samples - 1, 1);
  const Series omega_step = omega.slice(0, 1, samples, 1).sub(omega_left);

  const Series gain_left = left.abs().sub(1.0);
  const Series gain_right = right.abs().sub(1.0);
  const Series gain_alpha =
      safe_fraction(gain_left.mul(-1.0), gain_right.sub(gain_left), tolerance)
          .clamp(0.0, 1.0);
  const Series gain_cross = gain_left.mul(gain_right).le(0.0);
  const Series gain_response = left.add(right.sub(left).mul(gain_alpha));
  const Series phase_radians = gain_response.angle();
  const Series phase_degrees = phase_radians.mul(180.0 / kPi);
  const Series phase_margin = phase_degrees.le(0.0).where(
      phase_degrees.add(180.0), phase_degrees.sub(180.0));
  const Series phase_frequency = omega_left.add(omega_step.mul(gain_alpha));
  const Picked phase =
      pick_margin(phase_margin, phase_frequency, gain_cross, phase_margin.abs(),
                  std::numeric_limits<double>::infinity());

  const Series imag_left = left.imag();
  const Series imag_right = right.imag();
  const Series phase_alpha =
      safe_fraction(imag_left.mul(-1.0), imag_right.sub(imag_left), tolerance)
          .clamp(0.0, 1.0);
  const Series phase_response = left.add(right.sub(left).mul(phase_alpha));
  const Series phase_cross = imag_left.mul(imag_right)
                                 .le(0.0)
                                 .logical_and(phase_response.real().lt(0.0));
  const Series gain_margin = phase_response.abs().reciprocal();
  const Series gain_frequency = omega_left.add(omega_step.mul(phase_alpha));
  const Series finite_gain = gain_margin.isfinite().logical_and(phase_cross);
  const Picked gain = pick_margin(gain_margin, gain_frequency, finite_gain,
                                  gain_margin.log().abs(),
                                  std::numeric_limits<double>::infinity());

  const Series distance = response.add(1.0).abs();
  const Series stability_index = distance.argmin(-1, true);
  const Series stability = distance.gather(-1, stability_index).squeeze(-1);
  const Series frequency_grid = response.real().mul(0.0).add(omega);
  const Series stability_frequency =
      frequency_grid.gather(-1, stability_index).squeeze(-1);

  return {
      .gain = gain.value,
      .phase = phase.value,
      .stability = stability,
      .gain_freq = gain.frequency,
      .phase_freq = phase.frequency,
      .stability_freq = stability_frequency,
  };
}

Series siso_response(const lti::StateSpace &system, const Series &omega) {
  PLUSS_CHECK(system.inputs() == 1 && system.outputs() == 1,
              "eng::control::margin: classical margins require a SISO system");
  return lti::freqresp(system, omega).select(-3, 0).select(-2, 0);
}

} // namespace

LoopResult loop(const lti::StateSpace &system, const Series &omega) {
  PLUSS_CHECK(system.inputs() == system.outputs(),
              "eng::control::loop: the loop response must be square");
  const Series response = lti::freqresp(system, omega);
  PLUSS_CHECK(finite_complex(response),
              "eng::control::loop: response must contain only finite values");
  const Series matrix = response.moveaxis(-1, -3);
  const ScalarType real_dtype = response.scalar_type() == ScalarType::Complex128
                                    ? ScalarType::Float64
                                    : ScalarType::Float32;
  const Series identity =
      pluss::series::eye(
          system.inputs(),
          Config{}.with_dtype(real_dtype).with_platform(response.platform()))
          .to(response.scalar_type());
  const Series sens = matrix.add(identity).linearalg.lu_solve(identity);
  const Series comp = identity.sub(sens);
  return {
      .response = response,
      .sens = sens.moveaxis(-3, -1),
      .comp = comp.moveaxis(-3, -1),
  };
}

LoopResult loop(const lti::PoleZero &system, const Series &omega) {
  const Series response = lti::freqresp(system, omega);
  PLUSS_CHECK(finite_complex(response),
              "eng::control::loop: response must contain only finite values");
  const Series sens = response.add(1.0).reciprocal();
  return {
      .response = response,
      .sens = sens,
      .comp = response.mul(sens),
  };
}

MarginResult margin(const Series &response, const Series &omega) {
  return margin_impl(response, omega);
}

MarginResult margin(const lti::StateSpace &system, const Series &omega) {
  require_grid(omega, system.a().platform(), "margin");
  require_nyquist(omega, system.sample_time());
  return margin_impl(siso_response(system, omega), omega);
}

MarginResult margin(const lti::PoleZero &system, const Series &omega) {
  require_grid(omega, system.poles().platform(), "margin");
  require_nyquist(omega, system.sample_time());
  return margin_impl(lti::freqresp(system, omega), omega);
}

} // namespace pluss::subsp::eng::control

#include "pluss/subsp/eng/control.h"

#include "CodeM/core/Errors.h"
#include "CodeM/core/TypePromotion.h"
#include "pluss/codem/series_factory.h"

#include <cmath>
#include <complex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pluss::subsp::eng::control {
namespace {

using Complex = std::complex<double>;

bool is_narrow(ScalarType dtype) noexcept {
  return dtype == ScalarType::Float32 || dtype == ScalarType::Complex64;
}

ScalarType complex_dtype(ScalarType dtype) noexcept {
  return is_narrow(dtype) ? ScalarType::Complex64 : ScalarType::Complex128;
}

bool valid_rule(PidRule rule) noexcept {
  return rule == PidRule::Forward || rule == PidRule::Backward ||
         rule == PidRule::Trapezoid;
}

void validate_gain(const Series &value, std::string_view name) {
  PLUSS_CHECK(value.defined(), std::string("eng::control::pid: ") +
                                   std::string(name) + " must be defined");
  PLUSS_CHECK(value.dtype().is_floating(),
              std::string("eng::control::pid: ") + std::string(name) +
                  " must have a real floating dtype");
  PLUSS_CHECK(value.isfinite().all().item<bool>(),
              std::string("eng::control::pid: ") + std::string(name) +
                  " must contain only finite values");
}

Series integration_rule(const Series &z, double sample_time, PidRule rule) {
  const Series denominator = z.sub(1.0);
  switch (rule) {
  case PidRule::Forward:
    return denominator.reciprocal().mul(sample_time);
  case PidRule::Backward:
    return z.div(denominator).mul(sample_time);
  case PidRule::Trapezoid:
    return z.add(1.0).div(denominator).mul(0.5 * sample_time);
  }
  throw Error("eng::control::pid: invalid integration rule");
}

} // namespace

Pid pid(const Series &kp, const Series &ki, const Series &kd,
        const Series &filter, double sample_time, PidRule int_rule,
        PidRule diff_rule) {
  validate_gain(kp, "Kp");
  validate_gain(ki, "Ki");
  validate_gain(kd, "Kd");
  validate_gain(filter, "filter");
  PLUSS_CHECK(kp.platform() == ki.platform() &&
                  kp.platform() == kd.platform() &&
                  kp.platform() == filter.platform(),
              "eng::control::pid: gains must share one platform");
  PLUSS_CHECK(std::isfinite(sample_time) && sample_time >= 0.0,
              "eng::control::pid: sample_time must be finite and non-negative");
  PLUSS_CHECK(valid_rule(int_rule) && valid_rule(diff_rule),
              "eng::control::pid: integration rules are invalid");
  PLUSS_CHECK(filter.ge(0.0).all().item<bool>(),
              "eng::control::pid: filter must be non-negative");

  ScalarType dtype = codem::promote_types(kp.scalar_type(), ki.scalar_type());
  dtype = codem::promote_types(dtype, kd.scalar_type());
  dtype = codem::promote_types(dtype, filter.scalar_type());
  Series p = kp.to(dtype);
  Series i = ki.to(dtype);
  Series d = kd.to(dtype);
  Series f = filter.to(dtype);
  // Validate one common parameter batch at construction time.
  (void)p.add(i).add(d).add(f);

  if (sample_time > 0.0) {
    const Series active_d = d.ne(0.0);
    if (diff_rule == PidRule::Trapezoid)
      PLUSS_CHECK(!active_d.logical_and(f.eq(0.0)).any().item<bool>(),
                  "eng::control::pid: an unfiltered trapezoidal derivative is "
                  "unstable");
    if (diff_rule == PidRule::Forward) {
      const Series unstable = active_d.logical_and(f.gt(0.0)).logical_and(
          f.mul(2.0).le(sample_time));
      PLUSS_CHECK(!unstable.any().item<bool>(),
                  "eng::control::pid: forward derivative requires sample_time "
                  "< 2*filter");
    }
  }
  return Pid(std::move(p), std::move(i), std::move(d), std::move(f),
             sample_time, int_rule, diff_rule);
}

Series freqresp(const Pid &controller, const Series &omega) {
  PLUSS_CHECK(omega.defined() && omega.dim() == 1,
              "eng::control::freqresp(Pid): omega must be a defined vector");
  PLUSS_CHECK(omega.dtype().is_floating() &&
                  omega.isfinite().all().item<bool>(),
              "eng::control::freqresp(Pid): omega must be finite and floating");
  PLUSS_CHECK(omega.platform() == controller.kp().platform(),
              "eng::control::freqresp(Pid): omega and controller must share "
              "one platform");

  const ScalarType real_dtype =
      codem::promote_types(controller.kp().scalar_type(), omega.scalar_type());
  const ScalarType work_dtype = complex_dtype(real_dtype);
  const Series w = omega.to(work_dtype);
  const Series imaginary =
      pluss::series::make(
          std::vector<Complex>{{0.0, 1.0}}, {1},
          Config{}.with_dtype(work_dtype).with_platform(omega.platform()))
          .squeeze(0);
  const Series s = w.mul(imaginary);
  const Series kp = controller.kp().to(work_dtype).unsqueeze(-1);
  const Series ki = controller.ki().to(work_dtype).unsqueeze(-1);
  const Series kd = controller.kd().to(work_dtype).unsqueeze(-1);
  const Series filter = controller.filter().to(work_dtype).unsqueeze(-1);

  if (controller.is_cont()) {
    const Series integral = ki.eq(0.0).where(ki, ki.div(s));
    const Series derivative = kd.mul(s).div(filter.mul(s).add(1.0));
    return kp.add(integral).add(derivative);
  }

  const Series z = s.mul(controller.sample_time()).exp();
  const Series int_op =
      integration_rule(z, controller.sample_time(), controller.int_rule());
  const Series diff_op =
      integration_rule(z, controller.sample_time(), controller.diff_rule());
  const Series integral = ki.eq(0.0).where(ki, ki.mul(int_op));
  const Series derivative = kd.div(filter.add(diff_op));
  return kp.add(integral).add(derivative);
}

} // namespace pluss::subsp::eng::control

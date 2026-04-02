#pragma once

#include "CodeM/core/Errors.h"
#include "pluss/codem/codem.h"

#include <cmath>
#include <string>

namespace pluss {

enum class LossReduce : std::uint8_t {
  None = 0,
  Mean = 1,
  Sum = 2,
};

enum class RelMseDivisor : std::uint8_t {
  Target = 0,
  Prediction = 1,
};

namespace detail {

inline void validate_loss_inputs_(const Series& prediction, const Series& target, const char* op) {
  PLUSS_CHECK(prediction.defined(), std::string("pluss::") + op + ": prediction must be defined");
  PLUSS_CHECK(target.defined(), std::string("pluss::") + op + ": target must be defined");
  PLUSS_CHECK(prediction.platform() == target.platform(), std::string("pluss::") + op + ": platform mismatch");
}

inline Series reduce_loss_(const Series& per, LossReduce reduction) {
  switch (reduction) {
    case LossReduce::None:
      return per;
    case LossReduce::Sum:
      return per.sum();
    case LossReduce::Mean:
    default:
      return per.mean();
  }
}

} // namespace detail

inline Series mse(const Series& prediction, const Series& target, LossReduce reduction = LossReduce::Mean) {
  detail::validate_loss_inputs_(prediction, target, "mse");
  const Series per = prediction.sub(target).square();
  return detail::reduce_loss_(per, reduction);
}

inline Series rmse(const Series& prediction, const Series& target, LossReduce reduction = LossReduce::Mean) {
  if (reduction == LossReduce::None) {
    return prediction.sub(target).square().sqrt();
  }
  return mse(prediction, target, reduction).sqrt();
}

inline Series mae(const Series& prediction, const Series& target, LossReduce reduction = LossReduce::Mean) {
  detail::validate_loss_inputs_(prediction, target, "mae");
  const Series per = prediction.sub(target).abs();
  return detail::reduce_loss_(per, reduction);
}

inline Series huber_loss(const Series& prediction,
                         const Series& target,
                         double delta = 1.0,
                         LossReduce reduction = LossReduce::Mean) {
  detail::validate_loss_inputs_(prediction, target, "huber_loss");
  PLUSS_CHECK(std::isfinite(delta) && delta > 0.0, "pluss::huber_loss: delta must be finite and > 0");
  const Series err = prediction.sub(target);
  const Series abs_err = err.abs();
  const Series quadratic = abs_err.clamp(0.0, delta);
  const Series linear = abs_err.sub(quadratic);
  const Series per = quadratic.square().mul(0.5).add(linear.mul(delta));
  return detail::reduce_loss_(per, reduction);
}

inline Series rel_mse(const Series& prediction,
                      const Series& target,
                      const Series& divisor,
                      double epsilon = 1e-12,
                      bool use_absolute_divisor = true,
                      LossReduce reduction = LossReduce::Mean) {
  detail::validate_loss_inputs_(prediction, target, "rel_mse");
  PLUSS_CHECK(divisor.defined(), "pluss::rel_mse: divisor must be defined");
  PLUSS_CHECK(divisor.platform() == prediction.platform(), "pluss::rel_mse: divisor platform mismatch");
  PLUSS_CHECK(std::isfinite(epsilon) && epsilon > 0.0, "pluss::rel_mse: epsilon must be finite and > 0");

  Series denom = (divisor.scalar_type() == prediction.scalar_type()) ? divisor : divisor.to(prediction.scalar_type());
  if (use_absolute_divisor) {
    denom = denom.abs();
  }
  denom = denom.clamp_min(epsilon);
  const Series per = prediction.sub(target).square().div(denom);
  return detail::reduce_loss_(per, reduction);
}

inline Series rel_mse(const Series& prediction,
                      const Series& target,
                      RelMseDivisor divisor_source = RelMseDivisor::Target,
                      double epsilon = 1e-12,
                      bool use_absolute_divisor = true,
                      LossReduce reduction = LossReduce::Mean) {
  const Series& divisor = (divisor_source == RelMseDivisor::Prediction) ? prediction : target;
  return rel_mse(prediction, target, divisor, epsilon, use_absolute_divisor, reduction);
}

} // namespace pluss

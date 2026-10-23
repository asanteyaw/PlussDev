#include "pluss/subsp/eng/control.h"

#include "CodeM/core/Errors.h"
#include "CodeM/core/TypePromotion.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>

namespace pluss::subsp::eng::control {
namespace {

bool is_complex(ScalarType dtype) noexcept {
  return dtype == ScalarType::Complex64 || dtype == ScalarType::Complex128;
}

Series adjoint(const Series &value) {
  Series result = value.transpose(-2, -1);
  return is_complex(value.scalar_type()) ? result.conj() : result;
}

void validate_matrix(const Series &value, std::string_view operation,
                     std::string_view name) {
  const std::string prefix =
      std::string("eng::control::") + std::string(operation) + ": ";
  PLUSS_CHECK(value.defined(), prefix + std::string(name) + " must be defined");
  PLUSS_CHECK(value.dim() >= 2,
              prefix + std::string(name) + " must have matrix axes");
  PLUSS_CHECK(value.size(-2) > 0 && value.size(-1) > 0,
              prefix + std::string(name) + " must be non-empty");
  PLUSS_CHECK(value.dtype().is_floating() || value.dtype().is_complex(),
              prefix + std::string(name) +
                  " must have a floating or complex dtype");
}

void validate_problem(const Series &a, const Series &b, const Series &q,
                      const Series &r, const Series *cross,
                      std::string_view operation) {
  validate_matrix(a, operation, "A");
  validate_matrix(b, operation, "B");
  validate_matrix(q, operation, "Q");
  validate_matrix(r, operation, "R");
  const std::string prefix =
      std::string("eng::control::") + std::string(operation) + ": ";
  PLUSS_CHECK(a.size(-2) == a.size(-1), prefix + "A must be square");
  const std::int64_t states = a.size(-1);
  const std::int64_t inputs = b.size(-1);
  PLUSS_CHECK(b.size(-2) == states,
              prefix + "B rows must equal the state count");
  PLUSS_CHECK(q.size(-2) == states && q.size(-1) == states,
              prefix + "Q must have shape [..., states, states]");
  PLUSS_CHECK(r.size(-2) == inputs && r.size(-1) == inputs,
              prefix + "R must have shape [..., inputs, inputs]");
  PLUSS_CHECK(a.platform() == b.platform() && a.platform() == q.platform() &&
                  a.platform() == r.platform(),
              prefix + "A, B, Q, and R must share one platform");
  if (cross != nullptr) {
    validate_matrix(*cross, operation, "cross");
    PLUSS_CHECK(cross->size(-2) == states && cross->size(-1) == inputs,
                prefix + "cross must have shape [..., states, inputs]");
    PLUSS_CHECK(cross->platform() == a.platform(),
                prefix + "cross must share the system platform");
  }
}

ScalarType problem_dtype(const Series &a, const Series &b, const Series &q,
                         const Series &r, const Series *cross) {
  ScalarType dtype =
      pluss::codem::promote_types(a.scalar_type(), b.scalar_type());
  dtype = pluss::codem::promote_types(dtype, q.scalar_type());
  dtype = pluss::codem::promote_types(dtype, r.scalar_type());
  if (cross != nullptr)
    dtype = pluss::codem::promote_types(dtype, cross->scalar_type());
  return dtype;
}

double tolerance_for(ScalarType dtype, double requested) {
  const bool narrow =
      dtype == ScalarType::Float32 || dtype == ScalarType::Complex64;
  return std::max(requested, narrow ? 2e-5 : 5e-12);
}

void validate_weights(const Series &q, const Series &r,
                      std::string_view operation, double tolerance) {
  const std::string prefix =
      std::string("eng::control::") + std::string(operation) + ": ";
  PLUSS_CHECK(q.allclose(adjoint(q), tolerance, tolerance),
              prefix + "Q must be Hermitian");
  PLUSS_CHECK(r.allclose(adjoint(r), tolerance, tolerance),
              prefix + "R must be Hermitian");
  const double q_min = q.linearalg.eigvalsh().amin().item<double>();
  PLUSS_CHECK(std::isfinite(q_min) && q_min >= -tolerance,
              prefix + "Q must be positive semidefinite");
}

RiccatiResult solve_problem(const Series &a, const Series &b, const Series &q,
                            const Series &r, const Series *cross, bool discrete,
                            pluss::subsp::linearalg::MatrixEqOptions options,
                            std::string_view operation) {
  validate_problem(a, b, q, r, cross, operation);
  const ScalarType dtype = problem_dtype(a, b, q, r, cross);
  const Series aa = a.to(dtype);
  const Series bb = b.to(dtype);
  const Series qq = q.to(dtype);
  const Series rr = r.to(dtype);
  const Series nn = cross == nullptr ? bb.mul(0.0) : cross->to(dtype);
  const double tolerance = tolerance_for(dtype, options.tolerance);
  validate_weights(qq, rr, operation, tolerance);

  // Cholesky validates strict positive definiteness and is reused for every
  // R solve; no explicit inverse is formed.
  const Series r_factor = rr.linearalg.cholesky(false);
  const auto solve_r = [&](const Series &rhs) {
    return r_factor.linearalg.cholesky_solve(rhs, false).to(dtype);
  };
  const Series b_h = adjoint(bb);
  const Series n_h = adjoint(nn);
  const Series r_bh = solve_r(b_h);
  const Series g = bb.matmul(r_bh);
  const Series a_bar = aa.sub(bb.matmul(solve_r(n_h)));
  const Series q_bar = qq.sub(nn.matmul(solve_r(n_h)));
  const double reduced_q_min = q_bar.linearalg.eigvalsh().amin().item<double>();
  PLUSS_CHECK(std::isfinite(reduced_q_min) && reduced_q_min >= -tolerance,
              std::string("eng::control::") + std::string(operation) +
                  ": the joint [Q, cross; cross^H, R] cost must be "
                  "positive semidefinite");

  Series value = discrete ? a_bar.linearalg.dare(g, q_bar, options)
                          : a_bar.linearalg.care(g, q_bar, options);
  Series gain;
  if (discrete) {
    const Series gain_weight = rr.add(b_h.matmul(value).matmul(bb));
    const Series gain_factor = gain_weight.linearalg.cholesky(false);
    gain = gain_factor.linearalg
               .cholesky_solve(b_h.matmul(value).matmul(aa).add(n_h), false)
               .to(dtype);
  } else {
    gain = solve_r(b_h.matmul(value).add(n_h));
  }
  return {value, gain, aa.sub(bb.matmul(gain))};
}

LqeResult solve_estimator(const lti::StateSpace &system,
                          const Series &process_cov, const Series &noise_cov,
                          const Series *cross,
                          pluss::subsp::linearalg::MatrixEqOptions options) {
  const Series dual_a = adjoint(system.a());
  const Series dual_b = adjoint(system.c());
  const Series dual_cross = cross == nullptr ? dual_b.mul(0.0) : *cross;
  if (cross != nullptr) {
    PLUSS_CHECK(
        cross->dim() >= 2 && cross->size(-2) == system.states() &&
            cross->size(-1) == system.outputs(),
        "eng::control::lqe: cross must have shape [..., states, outputs]");
  }
  const RiccatiResult dual =
      solve_problem(dual_a, dual_b, process_cov, noise_cov, &dual_cross,
                    system.is_disc(), options, "lqe");
  const Series gain = adjoint(dual.gain);
  return {dual.value, gain, system.a().sub(gain.matmul(system.c()))};
}

pluss::subsp::math::state::LinearModel
filter_model(const lti::StateSpace &system, const Series &process_cov,
             const Series &noise_cov, const Series &initial_mean,
             const Series &initial_cov, const Series *input) {
  PLUSS_CHECK(system.is_disc(),
              "eng::control::kalman: the state-space model must be discrete");
  pluss::subsp::math::state::LinearModel model{
      .transition = system.a(),
      .observation = system.c(),
      .process_cov = process_cov,
      .observation_cov = noise_cov,
      .initial_mean = initial_mean,
      .initial_cov = initial_cov,
  };
  if (input != nullptr) {
    model.input = *input;
    model.state_input = system.b();
    model.obs_input = system.d();
  }
  return model;
}

} // namespace

RiccatiResult care(const Series &a, const Series &b, const Series &q,
                   const Series &r,
                   pluss::subsp::linearalg::MatrixEqOptions options) {
  return solve_problem(a, b, q, r, nullptr, false, options, "care");
}

RiccatiResult care(const Series &a, const Series &b, const Series &q,
                   const Series &r, const Series &cross,
                   pluss::subsp::linearalg::MatrixEqOptions options) {
  return solve_problem(a, b, q, r, &cross, false, options, "care");
}

RiccatiResult dare(const Series &a, const Series &b, const Series &q,
                   const Series &r,
                   pluss::subsp::linearalg::MatrixEqOptions options) {
  return solve_problem(a, b, q, r, nullptr, true, options, "dare");
}

RiccatiResult dare(const Series &a, const Series &b, const Series &q,
                   const Series &r, const Series &cross,
                   pluss::subsp::linearalg::MatrixEqOptions options) {
  return solve_problem(a, b, q, r, &cross, true, options, "dare");
}

RiccatiResult lqr(const lti::StateSpace &system, const Series &q,
                  const Series &r,
                  pluss::subsp::linearalg::MatrixEqOptions options) {
  return system.is_cont() ? care(system.a(), system.b(), q, r, options)
                          : dare(system.a(), system.b(), q, r, options);
}

RiccatiResult lqr(const lti::StateSpace &system, const Series &q,
                  const Series &r, const Series &cross,
                  pluss::subsp::linearalg::MatrixEqOptions options) {
  return system.is_cont() ? care(system.a(), system.b(), q, r, cross, options)
                          : dare(system.a(), system.b(), q, r, cross, options);
}

LqeResult lqe(const lti::StateSpace &system, const Series &process_cov,
              const Series &noise_cov,
              pluss::subsp::linearalg::MatrixEqOptions options) {
  return solve_estimator(system, process_cov, noise_cov, nullptr, options);
}

LqeResult lqe(const lti::StateSpace &system, const Series &process_cov,
              const Series &noise_cov, const Series &cross,
              pluss::subsp::linearalg::MatrixEqOptions options) {
  return solve_estimator(system, process_cov, noise_cov, &cross, options);
}

KalmanResult kalman(const lti::StateSpace &system, const Series &observations,
                    const Series &process_cov, const Series &noise_cov,
                    const Series &initial_mean, const Series &initial_cov,
                    KalmanOptions options) {
  return pluss::subsp::math::state::filter(observations,
                                           filter_model(system, process_cov,
                                                        noise_cov, initial_mean,
                                                        initial_cov, nullptr),
                                           options);
}

KalmanResult kalman(const lti::StateSpace &system, const Series &observations,
                    const Series &input, const Series &process_cov,
                    const Series &noise_cov, const Series &initial_mean,
                    const Series &initial_cov, KalmanOptions options) {
  return pluss::subsp::math::state::filter(observations,
                                           filter_model(system, process_cov,
                                                        noise_cov, initial_mean,
                                                        initial_cov, &input),
                                           options);
}

SmoothResult smooth(const lti::StateSpace &system, const Series &observations,
                    const Series &process_cov, const Series &noise_cov,
                    const Series &initial_mean, const Series &initial_cov,
                    KalmanOptions options) {
  return pluss::subsp::math::state::smooth(observations,
                                           filter_model(system, process_cov,
                                                        noise_cov, initial_mean,
                                                        initial_cov, nullptr),
                                           options);
}

SmoothResult smooth(const lti::StateSpace &system, const Series &observations,
                    const Series &input, const Series &process_cov,
                    const Series &noise_cov, const Series &initial_mean,
                    const Series &initial_cov, KalmanOptions options) {
  return pluss::subsp::math::state::smooth(observations,
                                           filter_model(system, process_cov,
                                                        noise_cov, initial_mean,
                                                        initial_cov, &input),
                                           options);
}

} // namespace pluss::subsp::eng::control

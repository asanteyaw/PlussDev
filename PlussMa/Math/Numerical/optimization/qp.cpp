#include "pluss/subsp/math/qp.h"

#include "CodeM/core/Errors.h"
#include "CodeM/core/TypePromotion.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/linearalg/linearalg_fwd.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace pluss::subsp::math::qp {
namespace {

constexpr const char *op = "math::qp::solve";

void require_vector(const Series &value, const char *name, std::int64_t width) {
  PLUSS_CHECK(value.defined(),
              std::string(op) + ": " + name + " must be defined");
  PLUSS_CHECK(value.dim() >= 1 && value.size(-1) == width,
              std::string(op) + ": " + name + " must end in " +
                  std::to_string(width));
}

void require_real_float(const Series &value, const char *name) {
  PLUSS_CHECK(value.dtype().is_floating(),
              std::string(op) + ": " + name +
                  " must have a real floating dtype");
}

double inf_norm(const Series &value) {
  return value.abs().amax().item<double>();
}

Series project(const Series &value, const Series &lower, const Series &upper) {
  return value.maximum(lower).minimum(upper);
}

} // namespace

Solver prepare(const Series &p, const Series &a, Options options) {
  PLUSS_CHECK(p.defined() && p.dim() >= 2 && p.size(-2) == p.size(-1) &&
                  p.size(-1) > 0,
              std::string(op) + ": P must end in a non-empty square matrix");
  PLUSS_CHECK(a.defined() && a.dim() >= 2 && a.size(-1) == p.size(-1),
              std::string(op) + ": A must end in [constraints, variables]");
  const std::int64_t variables = p.size(-1);
  const std::int64_t constraints = a.size(-2);
  PLUSS_CHECK(constraints > 0,
              std::string(op) + ": at least one constraint is required");
  require_real_float(p, "P");
  require_real_float(a, "A");
  PLUSS_CHECK(p.platform() == a.platform(),
              std::string(op) + ": P and A must share one platform");
  PLUSS_CHECK(options.max_iters > 0 && options.check_every > 0,
              std::string(op) + ": iteration counts must be positive");
  PLUSS_CHECK(std::isfinite(options.rho) && options.rho > 0.0 &&
                  std::isfinite(options.sigma) && options.sigma > 0.0,
              std::string(op) + ": rho and sigma must be finite and positive");
  PLUSS_CHECK(std::isfinite(options.alpha) && options.alpha > 0.0 &&
                  options.alpha < 2.0,
              std::string(op) + ": alpha must lie in (0, 2)");
  PLUSS_CHECK(std::isfinite(options.abs_tol) && options.abs_tol > 0.0 &&
                  std::isfinite(options.rel_tol) && options.rel_tol > 0.0,
              std::string(op) + ": tolerances must be finite and positive");

  const ScalarType dtype =
      codem::promote_types(p.scalar_type(), a.scalar_type());
  const Series pp = p.to(dtype);
  const Series aa = a.to(dtype);
  PLUSS_CHECK(pp.isfinite().all().item<bool>() &&
                  aa.isfinite().all().item<bool>(),
              std::string(op) + ": P and A must be finite");
  PLUSS_CHECK(pp.allclose(pp.transpose(-2, -1),
                          dtype == ScalarType::Float32 ? 2e-5 : 1e-11,
                          dtype == ScalarType::Float32 ? 2e-5 : 1e-11),
              std::string(op) + ": P must be symmetric");
  const double eig_floor = pp.linearalg.eigvalsh().amin().item<double>();
  const double psd_tol = dtype == ScalarType::Float32 ? 2e-5 : 1e-10;
  PLUSS_CHECK(std::isfinite(eig_floor) && eig_floor >= -psd_tol,
              std::string(op) + ": P must be positive semidefinite");

  const Series at = aa.transpose(-2, -1);
  const Series identity =
      pluss::series::eye(variables, dtype).to(pp.platform());
  const Series system =
      pp.add(identity.mul(options.sigma)).add(at.matmul(aa).mul(options.rho));
  const Series factor = system.linearalg.cholesky(false);

  return Solver(pp, aa, at, factor, options, dtype, variables, constraints);
}

Result Solver::solve(const Series &q, const Series &lower, const Series &upper,
                     WarmStart warm) const {
  require_vector(q, "q", variables_);
  require_vector(lower, "lower", constraints_);
  require_vector(upper, "upper", constraints_);
  require_real_float(q, "q");
  require_real_float(lower, "lower");
  require_real_float(upper, "upper");
  PLUSS_CHECK(
      q.platform() == p_.platform() && lower.platform() == p_.platform() &&
          upper.platform() == p_.platform(),
      std::string(op) + ": q and bounds must share the prepared platform");
  const Series qq = q.to(dtype_);
  const Series lo = lower.to(dtype_);
  const Series hi = upper.to(dtype_);
  PLUSS_CHECK(lo.le(hi).all().item<bool>(),
              std::string(op) + ": lower must not exceed upper");
  PLUSS_CHECK(qq.isfinite().all().item<bool>(),
              std::string(op) + ": q must be finite");

  Series x = warm.x.defined() ? warm.x.to(p_.platform(), dtype_) : qq.mul(0.0);
  require_vector(x, "warm.x", variables_);
  Series ax = a_.matmul(x.unsqueeze(-1)).squeeze(-1);
  Series z =
      warm.z.defined() ? warm.z.to(p_.platform(), dtype_) : project(ax, lo, hi);
  Series y = warm.y.defined() ? warm.y.to(p_.platform(), dtype_) : z.mul(0.0);
  require_vector(z, "warm.z", constraints_);
  require_vector(y, "warm.y", constraints_);
  PLUSS_CHECK(x.isfinite().all().item<bool>() &&
                  z.isfinite().all().item<bool>() &&
                  y.isfinite().all().item<bool>(),
              std::string(op) + ": warm-start values must be finite");

  double rho = options_.rho;
  Series factor = factor_;
  bool converged = false;
  double primal = INFINITY;
  double dual = INFINITY;
  std::int64_t iterations = 0;
  for (std::int64_t iteration = 1; iteration <= options_.max_iters;
       ++iteration) {
    const Series rhs =
        x.mul(options_.sigma)
            .sub(qq)
            .add(at_.matmul(z.mul(rho).sub(y).unsqueeze(-1))
                     .squeeze(-1));
    x = factor.linearalg.cholesky_solve(rhs.unsqueeze(-1), false).squeeze(-1);
    ax = a_.matmul(x.unsqueeze(-1)).squeeze(-1);
    const Series relaxed =
        ax.mul(options_.alpha).add(z.mul(1.0 - options_.alpha));
    z = project(relaxed.add(y.div(rho)), lo, hi);
    y = y.add(relaxed.sub(z).mul(rho));
    iterations = iteration;

    if (iteration % options_.check_every != 0 &&
        iteration != options_.max_iters)
      continue;

    const Series px = p_.matmul(x.unsqueeze(-1)).squeeze(-1);
    const Series aty = at_.matmul(y.unsqueeze(-1)).squeeze(-1);
    primal = inf_norm(ax.sub(z));
    dual = inf_norm(px.add(qq).add(aty));
    const double eps_primal =
        std::sqrt(static_cast<double>(constraints_)) * options_.abs_tol +
        options_.rel_tol * std::max(inf_norm(ax), inf_norm(z));
    const double eps_dual =
        std::sqrt(static_cast<double>(variables_)) * options_.abs_tol +
        options_.rel_tol *
            std::max({inf_norm(px), inf_norm(aty), inf_norm(qq)});
    if (primal <= eps_primal && dual <= eps_dual) {
      converged = true;
      break;
    }

    if (options_.adaptive_rho) {
      // Residual balancing: raise rho when the primal residual dominates, lower it when the dual one does. The multiplier y is
      // the true dual variable, so only the factorisation of P + sigma I + rho A'A has to be redone.
      const double primal_scale = std::max({inf_norm(ax), inf_norm(z), 1e-12});
      const double dual_scale = std::max({inf_norm(px), inf_norm(aty), inf_norm(qq), 1e-12});
      const double ratio = std::sqrt((primal / primal_scale) / std::max(dual / dual_scale, 1e-300));
      const double proposed = std::clamp(rho * ratio, 1e-6, 1e6);
      if (std::isfinite(proposed) && (proposed > rho * options_.adaptive_rho_tolerance ||
                                      proposed < rho / options_.adaptive_rho_tolerance)) {
        rho = proposed;
        const Series identity = pluss::series::eye(variables_, dtype_).to(p_.platform());
        const Series system = p_.add(identity.mul(options_.sigma)).add(at_.matmul(a_).mul(rho));
        factor = system.linearalg.cholesky(false);
      }
    }
  }

  const Series px = p_.matmul(x.unsqueeze(-1)).squeeze(-1);
  const Series objective =
      x.mul(px).sum(-1, false).mul(0.5).add(qq.mul(x).sum(-1, false));
  return {x, objective, z, y, primal, dual, iterations, converged};
}

Result solve(const Series &p, const Series &q, const Series &a,
             const Series &lower, const Series &upper, Options options,
             WarmStart warm) {
  ScalarType dtype = codem::promote_types(p.scalar_type(), q.scalar_type());
  dtype = codem::promote_types(dtype, a.scalar_type());
  dtype = codem::promote_types(dtype, lower.scalar_type());
  dtype = codem::promote_types(dtype, upper.scalar_type());
  return prepare(p.to(dtype), a.to(dtype), options)
      .solve(q.to(dtype), lower.to(dtype), upper.to(dtype), std::move(warm));
}

} // namespace pluss::subsp::math::qp

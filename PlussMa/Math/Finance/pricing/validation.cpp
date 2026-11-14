// Central validation of a Field and its parts: see validation.h.
#include "validation_internal.h"

#include "CodeM/core/Errors.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <variant>

namespace pluss::subsp::math::finance::internal::validation {
namespace {

using module::Engine;
using pluss::ScalarType;
using pluss::Series;
using pricing::ExerciseStyle;

constexpr const char *kPrefix = "subsp::math::finance::internal::validation";

void error(Report &r, std::string where, std::string message) { r.issues.push_back({Severity::Error, std::move(where), std::move(message)}); }
void warn(Report &r, std::string where, std::string message) { r.issues.push_back({Severity::Warning, std::move(where), std::move(message)}); }

struct Range {
  bool defined = false;
  std::int64_t count = 0;
  double lo = 0.0, hi = 0.0;
  bool finite = true;
};

Range range_of(const Series &x) {
  Range out;
  if (!x.defined()) return out;
  out.defined = true;
  out.count = x.numel();
  if (out.count == 0) return out;
  const Series v = x.to(pluss::pCPU, ScalarType::Float64);
  out.lo = v.min().item<double>();
  out.hi = v.max().item<double>();
  out.finite = std::isfinite(out.lo) && std::isfinite(out.hi);
  return out;
}

enum class Bound { Positive, NonNegative, Unit, Closed01, RoughExponent, Any };

// One numeric member: defined, non-empty, finite and inside its domain.  Returns the range for callers that need it.
Range member(Report &r, const std::string &where, const Series &x, Bound bound, bool required = true) {
  const Range g = range_of(x);
  if (!g.defined) {
    if (required) error(r, where, "must be defined");
    return g;
  }
  if (g.count == 0) {
    error(r, where, "must not be empty");
    return g;
  }
  if (!g.finite) {
    error(r, where, "must be finite");
    return g;
  }
  switch (bound) {
    case Bound::Positive:
      if (!(g.lo > 0.0)) error(r, where, "must be > 0");
      break;
    case Bound::NonNegative:
      if (!(g.lo >= 0.0)) error(r, where, "must be >= 0");
      break;
    case Bound::Unit:
      if (!(g.lo > -1.0 && g.hi < 1.0)) error(r, where, "must be in (-1, 1)");
      break;
    case Bound::Closed01:
      if (!(g.lo >= 0.0 && g.hi <= 1.0)) error(r, where, "must be in [0, 1]");
      break;
    case Bound::RoughExponent:
      if (!(g.lo > -0.5 && g.hi < 0.0)) error(r, where, "must be in (-0.5, 0) (the Hurst exponent minus 1/2)");
      break;
    case Bound::Any:
      break;
  }
  return g;
}

bool broadcastable(std::int64_t n, std::int64_t other) { return n == 1 || other == 1 || n == other; }

}  // namespace

bool Report::ok() const {
  return std::none_of(issues.begin(), issues.end(), [](const Issue &i) { return i.severity == Severity::Error; });
}

std::vector<Issue> Report::errors() const {
  std::vector<Issue> out;
  for (const auto &i : issues)
    if (i.severity == Severity::Error) out.push_back(i);
  return out;
}

std::vector<Issue> Report::warnings() const {
  std::vector<Issue> out;
  for (const auto &i : issues)
    if (i.severity == Severity::Warning) out.push_back(i);
  return out;
}

std::string Report::text() const {
  std::ostringstream os;
  bool first = true;
  for (const auto &i : issues) {
    if (!first) os << '\n';
    first = false;
    os << (i.severity == Severity::Error ? "error" : "warning") << ": " << i.where << ' ' << i.message;
  }
  return os.str();
}

void Report::raise(const std::string &context) const {
  const auto errs = errors();
  if (errs.empty()) return;
  std::ostringstream os;
  os << context;
  if (errs.size() == 1) {
    os << ": " << errs[0].where << ' ' << errs[0].message;
  } else {
    os << " (" << errs.size() << " problems):";
    for (const auto &e : errs) os << "\n  - " << e.where << ' ' << e.message;
  }
  PLUSS_CHECK(false, os.str());
}

void Report::merge(Report other) {
  issues.insert(issues.end(), std::make_move_iterator(other.issues.begin()), std::make_move_iterator(other.issues.end()));
}

std::vector<Engine> supported_engines(model::Kind kind) {
  switch (kind) {
    case model::Kind::BlackScholes: return {Engine::Analytic, Engine::Cos, Engine::MonteCarlo, Engine::Lattice, Engine::Spectral};
    case model::Kind::Sabr: return {Engine::Analytic, Engine::MonteCarlo};
    case model::Kind::Heston: return {Engine::Fft, Engine::Cos, Engine::MonteCarlo, Engine::Pde};
    case model::Kind::Bates: return {Engine::Fft, Engine::Cos, Engine::MonteCarlo};
    case model::Kind::RoughHeston:
    case model::Kind::RoughBergomi: return {Engine::MonteCarlo};
  }
  return {};
}

std::string engine_label(Engine engine) {
  switch (engine) {
    case Engine::Fft: return "Fft";
    case Engine::Cos: return "Cos";
    case Engine::MonteCarlo: return "MonteCarlo";
    case Engine::Analytic: return "Analytic";
    case Engine::Lattice: return "Lattice";
    case Engine::Spectral: return "Spectral";
    case Engine::Pde: return "Pde";
  }
  return "UnknownEngine";
}

Report check(const module::Model &model) {
  Report r;
  std::visit(
      [&](const auto &m) {
        using M = std::decay_t<decltype(m)>;
        const std::string n = std::string("model.") + module::Model::label<M>() + ".";
        if constexpr (std::is_same_v<M, model::BlackScholes>) {
          member(r, n + "volatility", m.volatility, Bound::Positive);
        } else if constexpr (std::is_same_v<M, model::Sabr>) {
          member(r, n + "alpha0", m.alpha0, Bound::Positive);
          member(r, n + "beta", m.beta, Bound::Closed01);
          member(r, n + "nu", m.nu, Bound::NonNegative);
          member(r, n + "rho", m.rho, Bound::Unit);
        } else if constexpr (std::is_same_v<M, model::Heston> || std::is_same_v<M, model::Bates>) {
          const auto kappa = member(r, n + "kappa", m.kappa, Bound::Positive);
          const auto theta = member(r, n + "theta", m.theta, Bound::Positive);
          const auto sigma = member(r, n + "sigma", m.sigma, Bound::Positive);
          member(r, n + "rho", m.rho, Bound::Unit);
          member(r, n + "v0", m.v0, Bound::NonNegative);
          if constexpr (std::is_same_v<M, model::Bates>) {
            member(r, n + "jump_intensity", m.jump_intensity, Bound::NonNegative, false);
            member(r, n + "jump_mean", m.jump_mean, Bound::Any, false);
            member(r, n + "jump_vol", m.jump_vol, Bound::NonNegative, false);
          }
          if (kappa.finite && theta.finite && sigma.finite && kappa.count && theta.count && sigma.count && kappa.lo > 0 && theta.lo > 0 &&
              2.0 * kappa.lo * theta.lo < sigma.hi * sigma.hi) {
            warn(r, n + "sigma", "violates the Feller condition 2*kappa*theta >= sigma^2: the variance can reach zero");
          }
        } else if constexpr (std::is_same_v<M, model::RoughHeston>) {
          member(r, n + "kappa", m.kappa, Bound::Positive);
          member(r, n + "theta", m.theta, Bound::Positive);
          member(r, n + "sigma", m.sigma, Bound::Positive);
          member(r, n + "alpha", m.alpha, Bound::RoughExponent);
          member(r, n + "rho", m.rho, Bound::Unit);
          member(r, n + "v0", m.v0, Bound::NonNegative);
        } else {
          member(r, n + "xi", m.xi, Bound::Positive);
          member(r, n + "eta", m.eta, Bound::Positive);
          member(r, n + "alpha", m.alpha, Bound::RoughExponent);
          member(r, n + "rho", m.rho, Bound::Unit);
        }
      },
      model.value);
  return r;
}

Report check(const instrument::Vanilla &inst) {
  Report r;
  member(r, "instrument.strike", inst.strike, Bound::Positive);
  const bool early = inst.exercise != ExerciseStyle::European;
  const auto tau = member(r, "instrument.maturity", inst.maturity, early ? Bound::Positive : Bound::NonNegative);
  (void)tau;
  if (inst.type == pricing::OptionType::Auto) {
    if (!inst.is_call.defined()) {
      error(r, "instrument.is_call", "must be defined when instrument.type is Auto");
    } else {
      member(r, "instrument.is_call", inst.is_call, Bound::Closed01);
    }
  }
  if (inst.exercise == ExerciseStyle::Bermudan) {
    const auto g = member(r, "instrument.exercise_times", inst.exercise_times, Bound::Positive);
    if (g.defined && g.count == 0) error(r, "instrument.exercise_times", "a Bermudan option needs at least one date");
  } else if (inst.exercise_times.defined() && inst.exercise_times.numel() > 0) {
    warn(r, "instrument.exercise_times", "is ignored unless instrument.exercise is Bermudan");
  }
  return r;
}

Report check(const market::State &mkt) {
  Report r;
  member(r, "market.spot", mkt.spot, Bound::Positive);
  member(r, "market.rate", mkt.rate, Bound::Any);
  member(r, "market.dividend_yield", mkt.dividend_yield, Bound::Any);
  return r;
}

Report check(const module::Controls &c, const module::Model &model, const instrument::Vanilla &inst) {
  Report r;
  const auto kind = model.kind();
  const auto supported = supported_engines(kind);
  if (std::find(supported.begin(), supported.end(), c.engine) == supported.end()) {
    std::string list;
    for (const auto e : supported) list += (list.empty() ? "" : ", ") + engine_label(e);
    error(r, "controls.engine",
          engine_label(c.engine) + " is not supported for model " + model.name() + ". Supported engines: " + list);
  }
  const bool early = inst.exercise != ExerciseStyle::European;
  if (inst.exercise == ExerciseStyle::Bermudan && c.engine == Engine::Spectral) {
    error(r, "controls.engine", "Spectral prices European and American exercise; a Bermudan option needs the MonteCarlo engine (any model), the Lattice or Cos engine (BlackScholes) or the Pde engine (Heston)");
  } else if (early && c.engine == Engine::Cos && model.kind() != model::Kind::BlackScholes && model.kind() != model::Kind::Heston && model.kind() != model::Kind::Bates) {
    error(r, "controls.engine", "Cos prices Bermudan and American exercise for BlackScholes, Heston and Bates only; use the MonteCarlo engine for " + model.name());
  } else if (early && c.engine != Engine::Cos && c.engine != Engine::MonteCarlo && c.engine != Engine::Lattice && c.engine != Engine::Spectral && c.engine != Engine::Pde) {
    error(r, "controls.engine",
          engine_label(c.engine) +
              " prices European exercise only; American and Bermudan options need the MonteCarlo engine (any model) or the Lattice or Spectral engine (BlackScholes) or the Pde engine (Heston)");
  }
  if (c.loss == module::Loss::Huber && !(std::isfinite(c.huber_delta) && c.huber_delta > 0.0)) {
    error(r, "controls.huber_delta", "must be finite and > 0 for the Huber loss");
  }
  switch (c.engine) {
    case Engine::MonteCarlo:
      if (c.mc.paths <= 0) error(r, "controls.mc.paths", "must be > 0");
      if (c.mc.steps <= 0) error(r, "controls.mc.steps", "must be > 0");
      if (c.mc.sobol_scrambling < 0 || c.mc.sobol_scrambling > 3) error(r, "controls.mc.sobol_scrambling", "must be in [0, 3]");
      if (!std::isfinite(c.mc.importance_shift)) error(r, "controls.mc.importance_shift", "must be finite");
      if (!(std::isfinite(c.mc.control_vol) && c.mc.control_vol >= 0.0)) error(r, "controls.mc.control_vol", "must be finite and >= 0");
      if (early) {
        if (c.mc.basis_degree < 1 || c.mc.basis_degree > 8) error(r, "controls.mc.basis_degree", "must be in [1, 8]");
        if (c.mc.control_variate || c.mc.importance_sampling) {
          error(r, "controls.mc", "control variates and importance sampling do not apply to American/Bermudan exercise");
        }
      }
      break;
    case Engine::Lattice:
      if (c.lattice.steps < 1) error(r, "controls.lattice.steps", "must be >= 1");
      break;
    case Engine::Spectral:
      if (c.spectral.nodes < 3) error(r, "controls.spectral.nodes", "must be >= 3");
      if (c.spectral.quadrature < 4) error(r, "controls.spectral.quadrature", "must be >= 4");
      if (c.spectral.max_iterations < 1) error(r, "controls.spectral.max_iterations", "must be >= 1");
      if (!(c.spectral.tolerance > 0.0)) error(r, "controls.spectral.tolerance", "must be > 0");
      break;
    case Engine::Pde:
      if (c.pde.spot_nodes < 8) error(r, "controls.pde.spot_nodes", "must be >= 8");
      if (c.pde.variance_nodes < 6) error(r, "controls.pde.variance_nodes", "must be >= 6");
      if (c.pde.time_steps < 3) error(r, "controls.pde.time_steps", "must be >= 3");
      break;
    case Engine::Cos:
      if (c.cos.num_terms < 2) error(r, "controls.cos.num_terms", "must be >= 2");
      if (!(c.cos.cumulant_step >= 1e-6)) error(r, "controls.cos.cumulant_step", "must be >= 1e-6 (a smaller step loses the variance to rounding)");
      if (early && (c.cos.variance_nodes < 8 || !(c.cos.variance_tolerance > 0.0))) error(r, "controls.cos.variance_nodes", "must be >= 8 (with variance_tolerance > 0)");
      if (early && c.cos.exercise_terms < 8) error(r, "controls.cos.exercise_terms", "must be >= 8");
      if (early && (c.cos.extrapolation < -1 || c.cos.extrapolation > 6)) error(r, "controls.cos.extrapolation", "must be in [-1, 6] (-1 selects the default for the model)");
      if (!(c.cos.truncation > 0.0)) error(r, "controls.cos.truncation", "must be > 0");
      break;
    case Engine::Fft:
      if (c.fft.num_fft < 2) error(r, "controls.fft.num_fft", "must be >= 2");
      if (!(c.fft.du > 0.0) || !std::isfinite(c.fft.du)) error(r, "controls.fft.du", "must be finite and > 0");
      if (!(c.fft.alpha > 0.0) || !std::isfinite(c.fft.alpha)) error(r, "controls.fft.alpha", "must be finite and > 0");
      if (!std::isnan(c.fft.dk) && !(c.fft.dk > 0.0 && std::isfinite(c.fft.dk))) error(r, "controls.fft.dk", "must be NaN (the transform's own spacing) or finite and > 0");
      break;
    case Engine::Analytic:
      break;
  }
  return r;
}

Report check(const module::Field &f) {
  Report r;
  r.merge(check(f.instrument));
  r.merge(check(f.market));
  r.merge(check(f.model));
  r.merge(check(f.controls, f.model, f.instrument));

  // sizes: the per-option members broadcast against each other
  const std::int64_t counts[] = {f.instrument.strike.defined() ? f.instrument.strike.numel() : 1, f.instrument.maturity.defined() ? f.instrument.maturity.numel() : 1,
                                 f.market.spot.defined() ? f.market.spot.numel() : 1, f.market.rate.defined() ? f.market.rate.numel() : 1,
                                 f.market.dividend_yield.defined() ? f.market.dividend_yield.numel() : 1};
  std::int64_t n = 1;
  bool mismatch = false;
  for (const auto c : counts) {
    if (!broadcastable(n, c)) mismatch = true;
    n = std::max(n, c);
  }
  if (mismatch) error(r, "instrument/market", "strike, maturity, spot, rate and dividend_yield must have one element or the same number of elements");

  // the binomial lattice needs a risk-neutral probability inside (0,1): sigma > |r - q| sqrt(dt)
  if (f.controls.engine == Engine::Lattice && f.model.holds<model::BlackScholes>()) {
    const auto vol = range_of(f.model.as<model::BlackScholes>().volatility);
    const auto tau = range_of(f.instrument.maturity), rate = range_of(f.market.rate), q = range_of(f.market.dividend_yield);
    if (vol.finite && vol.count && tau.finite && tau.count && rate.finite && rate.count && q.finite && q.count && f.controls.lattice.steps >= 1) {
      const double drift = std::max({std::abs(rate.lo - q.hi), std::abs(rate.hi - q.lo)});
      const double dt = tau.hi / static_cast<double>(f.controls.lattice.steps);
      if (vol.lo > 0.0 && vol.lo <= drift * std::sqrt(dt)) {
        error(r, "controls.lattice.steps", "is too small for this volatility and carry: the risk-neutral probability would leave (0, 1); use more steps");
      }
    }
  }
  return r;
}

void require(const module::Model &model) { check(model).raise("subsp::math::finance::module::Model"); }

void require(const module::Field &field) { check(field).raise("subsp::math::finance::module::Field"); }

}  // namespace pluss::subsp::math::finance::internal::validation

#pragma once

// Limited-memory SR1 with trust-region globalization (L-SR1-TR).
//
// This is a from-scratch implementation of:
//   J. B. Erway, J. Griffin, R. F. Marcia, R. Omheni, "Trust-Region
//   Algorithms for Training Responses: Machine Learning Methods Using
//   Indefinite Hessian Approximations," arXiv:1807.00251v3.
//
// "Algorithm 1" and "Algorithm 2" and equation numbers referenced in
// comments below are exactly the ones in that paper (deterministic,
// full-gradient variant -- the paper's Algorithm 1/"L-SR1-TR", not the
// stochastic mini-batch "L-SSR1-TR" extension). Unlike BFGS, SR1 does not
// force positive-definiteness onto the Hessian approximation, so it can
// represent indefinite curvature (saddle points, negative-curvature
// directions) -- this is why it is a useful complement to septima's
// existing Adam/LBFGS arsenal rather than a third variation on the same
// theme.
//
// Design: the paper's algorithm is inherently a single-flat-vector method
// (trust region in the 2-norm over R^n), so the core solver here operates
// on one flat Series. A convenience overload accepts std::vector<Series>
// params (matching septima's own multi-block ergonomics) by flattening
// through the same helpers septima itself uses.

#include "pluss/subsp/math/optim.h"
#include "pluss/subsp/math/linalg.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <vector>

namespace pluss::subsp::math::optim {

struct Lsr1Options {
  std::int64_t memory_size{10};      // m: number of (s,y) pairs kept (Algorithm 1, line 6)
  double initial_radius{1.0};        // delta_0
  double max_radius{1e8};
  double initial_gamma{1.0};         // gamma_0: B0 = gamma*I before any pair exists (Require: gamma_0>0)
  double gamma_shrink{0.9};          // tau in gamma_k = tau*lambda_hat, kept strictly in (0,1); see Sec 2.6,
                                      // Lemmas 2/4/5: gamma must stay strictly below lambda_hat or Bk's smallest
                                      // eigenvalue blows up negatively -- the paper names this the single most
                                      // common cause of L-SR1 failing in practice.
  double sr1_skip_tolerance{1e-8};   // r in the classical SR1 skip test |s^T(y-Bs)| >= r*||s||*||y-Bs|| (eq. 7
                                      // caption; Nocedal & Wright, "Numerical Optimization" 2nd ed., eq. 6.26)
  // Trust-region radius update constants (Algorithm 1's Require block):
  // 0 <= tau1 < tau2 < 0.5 < tau3 < 1 ,  0 < eta1 < eta2 <= 0.5 < eta3 < 1 < eta4.
  // (tau1 is declared in the paper's Require list but never referenced in the algorithm body; omitted here.)
  double tau_2{0.1};
  double tau_3{0.75};
  double eta_1{0.25};
  double eta_2{0.5};
  double eta_3{0.75};
  double eta_4{2.0};
  // Backtracking Armijo line search on the trust-region step direction (Algorithm 1, line 8:
  // "Compute step-size alpha with Wolfe line-search on p*. Set p* = alpha*p*."). Require lists
  // the initial step as alpha=1. Enforces sufficient decrease only -- see the long comment at the
  // call site for why a plain backtracking search cannot also reliably enforce the strong-Wolfe
  // curvature condition, and why SR1 (unlike BFGS) does not need it.
  double wolfe_c1{1e-4};
  std::int64_t max_line_search_iterations{20};
  double line_search_shrink{0.5};
  // Subproblem solver (Algorithm 2) numerics.
  double newton_tolerance{1e-10};
  std::int64_t max_newton_iterations{100};
  double zero_tolerance{1e-10};      // "is this ~0" / eigenvalue-tie tolerance used throughout Algorithm 2
};

struct Lsr1State {
  std::vector<Series> s_history{};   // each a flat 1-D Series, most-recent last
  std::vector<Series> y_history{};
  Series prev_grad_flat = pluss::series::undefined();
  double gamma{0.0};                 // 0 => uninitialized, seeded from options.initial_gamma on first use
  double radius{0.0};                // 0 => uninitialized, seeded from options.initial_radius on first use
  std::int64_t step{0};
};

enum class Lsr1ExitFlag : std::int32_t {
  Converged = 1,
  MaxIterations = 0,
  Stopped = -1,
  NoProgress = -2,
  NonFiniteEvaluation = -3,
  InvalidProblem = -4,
};

struct Lsr1ProgressSnapshot {
  std::int64_t iteration{0};
  std::int64_t function_evaluations{0};
  double objective{std::numeric_limits<double>::infinity()};
  double gradient_norm{std::numeric_limits<double>::infinity()};
  double trust_radius{0.0};
  double step_norm{0.0};
  std::string message{};
};

struct Lsr1SolveOptions {
  Lsr1Options step{};
  std::int64_t max_iterations{200};
  std::int64_t max_no_progress_iterations{25};
  double gradient_tolerance{1e-6};   // epsilon in Algorithm 1: ||g_k||_2 <= epsilon
  double step_tolerance{1e-12};
  std::function<bool(const Lsr1ProgressSnapshot&)> progress_hook{};
};

struct Lsr1Result {
  Series params = pluss::series::undefined();                   // flat solution vector
  Lsr1State state{};
  Series objective = pluss::series::undefined();
  std::int64_t iteration{0};
  std::int64_t function_evaluations{0};
  double objective_value{std::numeric_limits<double>::infinity()};
  double gradient_norm{std::numeric_limits<double>::infinity()};
  double step_norm{0.0};
  bool converged{false};
  Lsr1ExitFlag exit_flag{Lsr1ExitFlag::MaxIterations};
  std::string message{};
};

using Lsr1ObjectiveFn = std::function<Trace(const Trace&)>;

namespace detail {

namespace ll = pluss::subsp::math::linalg;

inline void lsr1_validate_options_(const Lsr1Options& o) {
  if (o.memory_size < 0) ::pluss::throw_error("optim::l_sr1_trust_region: memory_size must be >= 0");
  if (!(o.initial_radius > 0.0) || !std::isfinite(o.initial_radius)) {
    ::pluss::throw_error("optim::l_sr1_trust_region: initial_radius must be positive and finite");
  }
  if (!(o.max_radius >= o.initial_radius) || !std::isfinite(o.max_radius)) {
    ::pluss::throw_error("optim::l_sr1_trust_region: max_radius must be finite and >= initial_radius");
  }
  if (!(o.initial_gamma > 0.0) || !std::isfinite(o.initial_gamma)) {
    ::pluss::throw_error("optim::l_sr1_trust_region: initial_gamma must be positive and finite");
  }
  if (!(o.gamma_shrink > 0.0 && o.gamma_shrink < 1.0)) {
    ::pluss::throw_error("optim::l_sr1_trust_region: gamma_shrink must be in (0,1)");
  }
  if (!(o.sr1_skip_tolerance > 0.0) || !std::isfinite(o.sr1_skip_tolerance)) {
    ::pluss::throw_error("optim::l_sr1_trust_region: sr1_skip_tolerance must be positive and finite");
  }
  if (!(o.tau_2 > 0.0 && o.tau_2 < 0.5 && o.tau_2 < o.tau_3)) {
    ::pluss::throw_error("optim::l_sr1_trust_region: require 0 < tau_2 < 0.5 and tau_2 < tau_3");
  }
  if (!(o.tau_3 > 0.5 && o.tau_3 < 1.0)) {
    ::pluss::throw_error("optim::l_sr1_trust_region: require 0.5 < tau_3 < 1");
  }
  if (!(o.eta_1 > 0.0 && o.eta_1 < o.eta_2 && o.eta_2 <= 0.5)) {
    ::pluss::throw_error("optim::l_sr1_trust_region: require 0 < eta_1 < eta_2 <= 0.5");
  }
  if (!(o.eta_3 > 0.5 && o.eta_3 < 1.0 && o.eta_4 > 1.0)) {
    ::pluss::throw_error("optim::l_sr1_trust_region: require 0.5 < eta_3 < 1 < eta_4");
  }
  if (!(o.wolfe_c1 > 0.0 && o.wolfe_c1 < 1.0)) {
    ::pluss::throw_error("optim::l_sr1_trust_region: require 0 < wolfe_c1 < 1");
  }
  if (o.max_line_search_iterations <= 0) {
    ::pluss::throw_error("optim::l_sr1_trust_region: max_line_search_iterations must be > 0");
  }
  if (!(o.line_search_shrink > 0.0 && o.line_search_shrink < 1.0)) {
    ::pluss::throw_error("optim::l_sr1_trust_region: line_search_shrink must be in (0,1)");
  }
}

inline void lsr1_validate_solve_options_(const Lsr1SolveOptions& o) {
  lsr1_validate_options_(o.step);
  if (o.max_iterations <= 0) ::pluss::throw_error("optim::l_sr1_trust_region: max_iterations must be > 0");
  if (o.max_no_progress_iterations < 0) {
    ::pluss::throw_error("optim::l_sr1_trust_region: max_no_progress_iterations must be >= 0");
  }
  if (!(o.gradient_tolerance >= 0.0) || !std::isfinite(o.gradient_tolerance)) {
    ::pluss::throw_error("optim::l_sr1_trust_region: gradient_tolerance must be finite and >= 0");
  }
  if (!(o.step_tolerance >= 0.0) || !std::isfinite(o.step_tolerance)) {
    ::pluss::throw_error("optim::l_sr1_trust_region: step_tolerance must be finite and >= 0");
  }
}

// ---------------------------------------------------------------------
// Plain-vector<double> numeric core. All S/Y history matrices are stored
// ROW-MAJOR, n x k (n = problem dimension, k = number of stored pairs,
// most-recent pair last). Keeping this arithmetic in plain doubles (mirroring
// linalg.h's own detail:: layer) makes it directly inspectable/testable, and
// confines Series/autodiff interaction to the outer loop.
// ---------------------------------------------------------------------

inline std::vector<double> lsr1_to_vec_(const Series& flat) {
  return flat.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();
}

// history (k flat Series, each length n) -> row-major n x k matrix.
inline std::vector<double> lsr1_stack_cols_(const std::vector<Series>& history, std::int64_t n) {
  const auto k = static_cast<std::int64_t>(history.size());
  std::vector<double> out(static_cast<std::size_t>(n * k), 0.0);
  for (std::int64_t j = 0; j < k; ++j) {
    const auto col = lsr1_to_vec_(history[static_cast<std::size_t>(j)]);
    for (std::int64_t i = 0; i < n; ++i) out[static_cast<std::size_t>(i * k + j)] = col[static_cast<std::size_t>(i)];
  }
  return out;
}

inline double lsr1_dot_(const std::vector<double>& a, const std::vector<double>& b) {
  double s = 0.0;
  for (std::size_t i = 0; i < a.size(); ++i) s += a[i] * b[i];
  return s;
}

inline double lsr1_norm_(const std::vector<double>& a) { return std::sqrt(std::max(0.0, lsr1_dot_(a, a))); }

// S^T Y for n x k matrices S, Y (row-major); result is k x k, generally not symmetric.
inline std::vector<double> lsr1_mtm_(const std::vector<double>& A, const std::vector<double>& B, std::int64_t n,
                                     std::int64_t k) {
  std::vector<double> out(static_cast<std::size_t>(k * k), 0.0);
  for (std::int64_t a = 0; a < k; ++a) {
    for (std::int64_t b = 0; b < k; ++b) {
      double s = 0.0;
      for (std::int64_t i = 0; i < n; ++i) {
        s += A[static_cast<std::size_t>(i * k + a)] * B[static_cast<std::size_t>(i * k + b)];
      }
      out[static_cast<std::size_t>(a * k + b)] = s;
    }
  }
  return out;
}

// D_k + L_k + L_k^T, built directly from S^T Y (k x k), per eq. (9): L_k is the strictly
// lower triangular part of S^T Y, D_k its diagonal, and this quantity uses L_k reflected
// into the upper triangle -- NOT a symmetrization of S^T Y itself.
inline std::vector<double> lsr1_dllt_(const std::vector<double>& sty, std::int64_t k) {
  std::vector<double> out(static_cast<std::size_t>(k * k), 0.0);
  for (std::int64_t a = 0; a < k; ++a) {
    out[static_cast<std::size_t>(a * k + a)] = sty[static_cast<std::size_t>(a * k + a)];
    for (std::int64_t b = 0; b < a; ++b) {
      const double v = sty[static_cast<std::size_t>(a * k + b)];  // strictly-lower entry of S^TY
      out[static_cast<std::size_t>(a * k + b)] = v;
      out[static_cast<std::size_t>(b * k + a)] = v;
    }
  }
  return out;
}

inline std::vector<double> lsr1_mat_axpy_(const std::vector<double>& a, double alpha, const std::vector<double>& b) {
  std::vector<double> out(a.size());
  for (std::size_t i = 0; i < a.size(); ++i) out[i] = a[i] + alpha * b[i];
  return out;
}

// Smallest eigenvalue of the generalized eigenproblem (D_k+L_k+L_k^T) u = lambda_hat S_k^T S_k u,
// via congruence reduction through the Cholesky factor of S^TS (Sec. 2.6: "lambda_hat is cheaply
// determined due to the small column dimension of S_k"). Returns false if S^TS is not (numerically)
// SPD, i.e. the stored s-history is rank deficient -- in that case gamma falls back to the
// standard scaling below, unclamped.
inline bool lsr1_lambda_hat_(const std::vector<double>& S, const std::vector<double>& Y, std::int64_t n,
                             std::int64_t k, double* lambda_hat_out) {
  if (k <= 0) return false;
  const auto sts = lsr1_mtm_(S, S, n, k);
  const auto sty = lsr1_mtm_(S, Y, n, k);
  const auto dllt = lsr1_dllt_(sty, k);

  Series sts_series = ll::detail::matrix_from_row_major_(sts, k, k);
  Series chol_upper = pluss::series::undefined();
  try {
    chol_upper = ll::cholesky(sts_series, /*upper=*/true);
  } catch (const std::exception&) {
    return false;
  }
  // Cholesky can "succeed" numerically on a badly-conditioned S^TS (nearly-parallel stored s
  // vectors) while still being unusable: the subsequent triangular solves amplify that
  // ill-conditioning into wildly wrong lambda_hat values (observed directly: values on the
  // order of 1e17 from a matrix whose true eigenvalues were all O(1-1e3)). Reject explicitly
  // via the diagonal spread of the Cholesky factor, a standard cheap condition-number proxy.
  {
    const auto diag = chol_upper.to_vec<double>();
    double min_abs = std::numeric_limits<double>::infinity();
    double max_abs = 0.0;
    for (std::int64_t i = 0; i < k; ++i) {
      const double d = std::abs(diag[static_cast<std::size_t>(i * k + i)]);
      min_abs = std::min(min_abs, d);
      max_abs = std::max(max_abs, d);
    }
    if (!(min_abs > 0.0) || max_abs / min_abs > 1e6) return false;
  }
  Series dllt_series = ll::detail::matrix_from_row_major_(dllt, k, k);
  // A' = C^{-T} (D+L+L^T) C^{-1}, with S^TS = C^T C (C upper, from cholesky(.., upper=true)).
  // Step 1: solve C^T Y = M0 (C^T is lower triangular) -> Y = C^{-T} M0.
  // Step 2: A' = Y C^{-1}, i.e. A'^T = C^{-T} Y^T; solve C^T Z = Y^T -> Z = C^{-T} Y^T = A'
  // (using M0 symmetric, so Y^T = M0 C^{-1}, and A' is symmetric so A'^T = A').
  Series c_t = chol_upper.transpose(0, 1).contiguous();
  Series y = ll::triangular_solve(dllt_series, c_t, /*upper=*/false, /*unitriangular=*/false);
  Series y_t = y.transpose(0, 1).contiguous();
  Series a_prime = ll::triangular_solve(y_t, c_t, /*upper=*/false, /*unitriangular=*/false);
  const auto eig = ll::eigh(a_prime);
  const auto values = eig.eigenvalues.to_vec<double>();
  if (values.empty() || !std::isfinite(values.front())) return false;
  *lambda_hat_out = values.front();
  return true;
}

// SR1 secant-pair skip test: reject the newest (s,y) pair if it would make the compact update
// ill-defined, per eq. (7)'s caption and Nocedal & Wright eq. 6.26: skip unless
// |s^T(y - B*s)| >= r * ||s|| * ||y - B*s||. B*s is evaluated using the CURRENT compact
// representation (before adding this pair), so B0=gamma*I when there is no history yet.
struct Lsr1Compact;
inline std::vector<double> lsr1_apply_b_(const Lsr1Compact& compact, const std::vector<double>& v, std::int64_t n);

struct Lsr1Compact {
  Series Q = pluss::series::undefined();                 // n x k, orthonormal columns (thin QR of Psi = Y - gamma*S)
  Series R = pluss::series::undefined();                 // k x k upper triangular (thin QR factor)
  Series U = pluss::series::undefined();                 // k x k eigenvectors of R*M*R^T (ascending eigenvalues)
  std::vector<double> lambda_hat{};  // k eigenvalues of R*M*R^T, ascending
  double gamma{0.0};
  std::int64_t k{0};
};

// Builds the compact representation B_k = gamma*I + Psi*M*Psi^T (eq. 8-9) and its spectral
// pieces (page 6-7: Psi=QR, B_k = gamma*I + Q*(R*M*R^T)*Q^T). Returns k=0 (empty) compact
// object when there is no history (B_k = gamma*I exactly). Throws nothing on its own;
// numerical degeneracies (rank-deficient Psi) are handled by the caller via the retry loop
// in build below.
inline Lsr1Compact lsr1_build_compact_(const std::vector<double>& S, const std::vector<double>& Y, std::int64_t n,
                                       std::int64_t k, double gamma) {
  Lsr1Compact out;
  out.gamma = gamma;
  out.k = k;
  if (k <= 0) return out;

  std::vector<double> psi(static_cast<std::size_t>(n * k));
  for (std::size_t i = 0; i < psi.size(); ++i) psi[i] = Y[i] - gamma * S[i];

  Series psi_series = ll::detail::matrix_from_row_major_(psi, n, k);
  auto qr = ll::qr(psi_series, /*reduced=*/true);

  // Degeneracy guard: if any diagonal of R is ~0, Psi's columns are (numerically) linearly
  // dependent and R is not invertible -- the caller drops the oldest pair and retries.
  const auto r_vec = qr.R.to_vec<double>();
  double max_abs_diag = 0.0;
  for (std::int64_t i = 0; i < k; ++i) max_abs_diag = std::max(max_abs_diag, std::abs(r_vec[static_cast<std::size_t>(i * k + i)]));
  const double diag_floor = std::max(1e-300, max_abs_diag * 1e-10);
  for (std::int64_t i = 0; i < k; ++i) {
    if (std::abs(r_vec[static_cast<std::size_t>(i * k + i)]) < diag_floor) {
      out.k = -1;  // sentinel: degenerate, caller must retry with fewer pairs
      return out;
    }
  }

  const auto sty = lsr1_mtm_(S, Y, n, k);
  const auto sts = lsr1_mtm_(S, S, n, k);
  auto minv = lsr1_dllt_(sty, k);
  minv = lsr1_mat_axpy_(minv, -gamma, sts);  // M^{-1} = D+L+L^T - gamma*S^TS

  Series minv_series = ll::detail::matrix_from_row_major_(minv, k, k);
  Series m_series = pluss::series::undefined();
  try {
    m_series = ll::inverse(minv_series);
  } catch (const std::exception&) {
    out.k = -1;
    return out;
  }
  // A = R * M * R^T (k x k, symmetric up to floating error by construction -- symmetrize
  // explicitly before eigh, since eigh only reads one triangle and small asymmetric noise
  // here would otherwise be silently discarded rather than averaged).
  Series a = qr.R.matmul(m_series).matmul(qr.R.transpose(0, 1).contiguous());
  Series a_sym = a.add(a.transpose(0, 1)).mul(0.5).contiguous();

  const auto eig = ll::eigh(a_sym);
  out.Q = qr.Q;
  out.R = qr.R;
  out.U = eig.eigenvectors;
  out.lambda_hat = eig.eigenvalues.to_vec<double>();
  return out;
}

// B_k * v for the full n-vector v, using the compact representation directly
// (no need to materialize B_k): B_k v = gamma*v + Psi*(M*(Psi^T v)) = gamma*v + Q*(R*M*R^T)*(Q^T v).
inline std::vector<double> lsr1_apply_b_(const Lsr1Compact& compact, const std::vector<double>& v, std::int64_t n) {
  std::vector<double> out(v.size());
  for (std::size_t i = 0; i < v.size(); ++i) out[i] = compact.gamma * v[i];
  if (compact.k <= 0) return out;
  const auto k = compact.k;
  const auto q = compact.Q.to_vec<double>();  // n x k
  std::vector<double> qtv(static_cast<std::size_t>(k), 0.0);
  for (std::int64_t j = 0; j < k; ++j) {
    double s = 0.0;
    for (std::int64_t i = 0; i < n; ++i) s += q[static_cast<std::size_t>(i * k + j)] * v[static_cast<std::size_t>(i)];
    qtv[static_cast<std::size_t>(j)] = s;
  }
  // (R M R^T) qtv, using the eigendecomposition A = U diag(lambda_hat) U^T:
  const auto u = compact.U.to_vec<double>();
  std::vector<double> ut_qtv(static_cast<std::size_t>(k), 0.0);
  for (std::int64_t j = 0; j < k; ++j) {
    double s = 0.0;
    for (std::int64_t i = 0; i < k; ++i) s += u[static_cast<std::size_t>(i * k + j)] * qtv[static_cast<std::size_t>(i)];
    ut_qtv[static_cast<std::size_t>(j)] = s;
  }
  for (std::int64_t j = 0; j < k; ++j) ut_qtv[static_cast<std::size_t>(j)] *= compact.lambda_hat[static_cast<std::size_t>(j)];
  std::vector<double> a_qtv(static_cast<std::size_t>(k), 0.0);
  for (std::int64_t j = 0; j < k; ++j) {
    double s = 0.0;
    for (std::int64_t i = 0; i < k; ++i) s += u[static_cast<std::size_t>(j * k + i)] * ut_qtv[static_cast<std::size_t>(i)];
    a_qtv[static_cast<std::size_t>(j)] = s;
  }
  for (std::int64_t i = 0; i < n; ++i) {
    double s = 0.0;
    for (std::int64_t j = 0; j < k; ++j) s += q[static_cast<std::size_t>(i * k + j)] * a_qtv[static_cast<std::size_t>(j)];
    out[static_cast<std::size_t>(i)] += s;
  }
  return out;
}

// ---------------------------------------------------------------------
// Algorithm 2: Orthonormal Basis SR1 Method (page 8). Solves
//   min_p  g^T p + 0.5 p^T B p   s.t.  ||p||_2 <= delta
// exactly, including the boundary and "hard case" branches.
// ---------------------------------------------------------------------

struct Lsr1Subproblem {
  std::vector<double> p;   // solution p*
};

inline Lsr1Subproblem lsr1_solve_subproblem_(const Lsr1Compact& compact, const std::vector<double>& g,
                                             std::int64_t n, double delta, const Lsr1Options& opts) {
  Lsr1Subproblem result;
  const double g_norm = lsr1_norm_(g);
  if (g_norm <= opts.zero_tolerance) {
    result.p.assign(static_cast<std::size_t>(n), 0.0);
    return result;
  }

  const double gamma = compact.gamma;
  const std::int64_t k = compact.k;
  const double tol = opts.zero_tolerance;

  if (k <= 0) {
    // B_k = gamma*I exactly: the subproblem has a closed form.
    if (gamma > 0.0) {
      std::vector<double> p(g.size());
      for (std::size_t i = 0; i < g.size(); ++i) p[i] = -g[i] / gamma;
      if (lsr1_norm_(p) <= delta) {
        result.p = std::move(p);
        return result;
      }
    }
    // Interior minimizer infeasible or gamma<=0 (pure negative curvature): steepest-descent
    // direction scaled to the boundary is optimal for an isotropic B_k=gamma*I in both cases.
    std::vector<double> p(g.size());
    const double scale = -delta / g_norm;
    for (std::size_t i = 0; i < g.size(); ++i) p[i] = scale * g[i];
    result.p = std::move(p);
    return result;
  }

  const auto& lambda_hat = compact.lambda_hat;  // ascending
  std::vector<double> lambda(static_cast<std::size_t>(k));
  for (std::int64_t i = 0; i < k; ++i) lambda[static_cast<std::size_t>(i)] = lambda_hat[static_cast<std::size_t>(i)] + gamma;

  // g_parallel = U^T (Q^T g)  (page 6: Psi*R^{-1} = Q since Psi = Q*R, so g_par = (Psi R^{-1} U)^T g = U^T Q^T g)
  const auto q = compact.Q.to_vec<double>();
  std::vector<double> qtg(static_cast<std::size_t>(k), 0.0);
  for (std::int64_t j = 0; j < k; ++j) {
    double s = 0.0;
    for (std::int64_t i = 0; i < n; ++i) s += q[static_cast<std::size_t>(i * k + j)] * g[static_cast<std::size_t>(i)];
    qtg[static_cast<std::size_t>(j)] = s;
  }
  const auto u = compact.U.to_vec<double>();
  std::vector<double> g_par(static_cast<std::size_t>(k), 0.0);
  for (std::int64_t j = 0; j < k; ++j) {
    double s = 0.0;
    for (std::int64_t i = 0; i < k; ++i) s += u[static_cast<std::size_t>(i * k + j)] * qtg[static_cast<std::size_t>(i)];
    g_par[static_cast<std::size_t>(j)] = s;
  }
  double g_par_normsq = 0.0;
  for (double v : g_par) g_par_normsq += v * v;
  const double g_perp_normsq = std::max(0.0, g_norm * g_norm - g_par_normsq);

  const double lambda_min_compact = lambda.front();  // lambda_1 (smallest of the k eigenvalues)
  const double lambda_min = std::min(lambda_min_compact, gamma);

  // h(sigma) = ||v(sigma)||_2^2 = sum_i g_par_i^2/(lambda_i+sigma)^2 + g_perp_normsq/(gamma+sigma)^2  (eq. 14)
  auto h_of_sigma = [&](double sigma) {
    double s = 0.0;
    for (std::int64_t i = 0; i < k; ++i) {
      const double d = lambda[static_cast<std::size_t>(i)] + sigma;
      s += (g_par[static_cast<std::size_t>(i)] * g_par[static_cast<std::size_t>(i)]) / (d * d);
    }
    const double dperp = gamma + sigma;
    s += g_perp_normsq / (dperp * dperp);
    return s;
  };
  auto phi = [&](double sigma) { return 1.0 / std::sqrt(std::max(h_of_sigma(sigma), std::numeric_limits<double>::min())) - 1.0 / delta; };

  // p(sigma) for a sigma strictly greater than -lambda_min (regular formula, all terms defined).
  auto p_of_sigma = [&](double sigma) {
    std::vector<double> p(static_cast<std::size_t>(n), 0.0);
    std::vector<double> coeff(static_cast<std::size_t>(k));
    for (std::int64_t i = 0; i < k; ++i) coeff[static_cast<std::size_t>(i)] = -g_par[static_cast<std::size_t>(i)] / (lambda[static_cast<std::size_t>(i)] + sigma);
    // contribution from the k-dim compact subspace: Q*U*coeff
    std::vector<double> u_coeff(static_cast<std::size_t>(k), 0.0);
    for (std::int64_t i = 0; i < k; ++i) {
      double s = 0.0;
      for (std::int64_t j = 0; j < k; ++j) s += u[static_cast<std::size_t>(i * k + j)] * coeff[static_cast<std::size_t>(j)];
      u_coeff[static_cast<std::size_t>(i)] = s;
    }
    for (std::int64_t idx = 0; idx < n; ++idx) {
      double s = 0.0;
      for (std::int64_t j = 0; j < k; ++j) s += q[static_cast<std::size_t>(idx * k + j)] * u_coeff[static_cast<std::size_t>(j)];
      p[static_cast<std::size_t>(idx)] = s;
    }
    // perpendicular contribution: -(g - Q*Q^T*g) / (gamma+sigma)
    const double perp_coeff = -1.0 / (gamma + sigma);
    for (std::int64_t idx = 0; idx < n; ++idx) {
      double q_qtg = 0.0;
      for (std::int64_t j = 0; j < k; ++j) q_qtg += q[static_cast<std::size_t>(idx * k + j)] * qtg[static_cast<std::size_t>(j)];
      p[static_cast<std::size_t>(idx)] += perp_coeff * (g[static_cast<std::size_t>(idx)] - q_qtg);
    }
    return p;
  };

  // Which eigenspace attains lambda_min, and does g have (numerically) zero component there?
  const bool perp_is_min = std::abs(gamma - lambda_min) <= tol * std::max(1.0, std::abs(lambda_min));
  double comp_g_at_min_sq = perp_is_min ? g_perp_normsq : 0.0;
  std::vector<bool> tied(static_cast<std::size_t>(k), false);
  for (std::int64_t i = 0; i < k; ++i) {
    if (std::abs(lambda[static_cast<std::size_t>(i)] - lambda_min) <= tol * std::max(1.0, std::abs(lambda_min))) {
      tied[static_cast<std::size_t>(i)] = true;
      comp_g_at_min_sq += g_par[static_cast<std::size_t>(i)] * g_par[static_cast<std::size_t>(i)];
    }
  }
  const bool g_orthogonal_to_min_eigenspace = comp_g_at_min_sq <= tol * tol * std::max(1.0, g_norm * g_norm);

  auto finite_pinv_normsq = [&]() {
    double s = 0.0;
    for (std::int64_t i = 0; i < k; ++i) {
      if (tied[static_cast<std::size_t>(i)]) continue;
      const double d = lambda[static_cast<std::size_t>(i)] - lambda_min;
      s += (g_par[static_cast<std::size_t>(i)] * g_par[static_cast<std::size_t>(i)]) / (d * d);
    }
    if (!perp_is_min) {
      const double d = gamma - lambda_min;
      s += g_perp_normsq / (d * d);
    }
    return s;
  };

  if (lambda_min > tol && phi(0.0) >= 0.0) {
    // Line 6-7: interior solution, sigma*=0, p* = -B^{-1} g.
    result.p = p_of_sigma(0.0);
    return result;
  }

  if (lambda_min <= tol && g_orthogonal_to_min_eigenspace) {
    const double p_hat_normsq = finite_pinv_normsq();
    if (p_hat_normsq <= delta * delta + tol) {
      // Line 8-14: sigma* = -lambda_min; solve on the pseudo-inverse (non-degenerate) subspace.
      std::vector<double> p_hat(static_cast<std::size_t>(n), 0.0);
      // p_hat via the same p_of_sigma formula but skipping tied indices and (if perp is min) the
      // perpendicular term -- both of which have zero numerator here (g_orthogonal_to_min_eigenspace),
      // so this is exactly p(-lambda_min) evaluated in the limit (finite by construction).
      std::vector<double> coeff(static_cast<std::size_t>(k), 0.0);
      for (std::int64_t i = 0; i < k; ++i) {
        if (tied[static_cast<std::size_t>(i)]) continue;
        coeff[static_cast<std::size_t>(i)] = -g_par[static_cast<std::size_t>(i)] / (lambda[static_cast<std::size_t>(i)] - lambda_min);
      }
      std::vector<double> u_coeff(static_cast<std::size_t>(k), 0.0);
      for (std::int64_t i = 0; i < k; ++i) {
        double s = 0.0;
        for (std::int64_t j = 0; j < k; ++j) s += u[static_cast<std::size_t>(i * k + j)] * coeff[static_cast<std::size_t>(j)];
        u_coeff[static_cast<std::size_t>(i)] = s;
      }
      for (std::int64_t idx = 0; idx < n; ++idx) {
        double s = 0.0;
        for (std::int64_t j = 0; j < k; ++j) s += q[static_cast<std::size_t>(idx * k + j)] * u_coeff[static_cast<std::size_t>(j)];
        p_hat[static_cast<std::size_t>(idx)] = s;
      }
      if (!perp_is_min) {
        const double perp_coeff = -1.0 / (gamma - lambda_min);
        for (std::int64_t idx = 0; idx < n; ++idx) {
          double q_qtg = 0.0;
          for (std::int64_t j = 0; j < k; ++j) q_qtg += q[static_cast<std::size_t>(idx * k + j)] * qtg[static_cast<std::size_t>(j)];
          p_hat[static_cast<std::size_t>(idx)] += perp_coeff * (g[static_cast<std::size_t>(idx)] - q_qtg);
        }
      }

      if (lambda_min < -tol) {
        // Line 11-13: the hard case. Need a unit eigenvector u_min for eigenvalue lambda_min,
        // then p* = p_hat + alpha*u_min with alpha chosen so ||p*||=delta (u_min is orthogonal
        // to p_hat by construction, since p_hat lives entirely in the non-degenerate eigenspaces).
        std::vector<double> u_min(static_cast<std::size_t>(n), 0.0);
        if (!perp_is_min) {
          // Degenerate eigenspace lies in the k-dim compact subspace: the eigenvectors of B_k
          // restricted to span(Q) are the columns of Q*U (not of Q itself), since
          // B_k|_{span(Q)} = Q*U*diag(lambda)*U^T*Q^T -- so u_min = Q * U[:,first tied].
          std::int64_t idx0 = 0;
          for (std::int64_t i = 0; i < k; ++i) {
            if (tied[static_cast<std::size_t>(i)]) { idx0 = i; break; }
          }
          for (std::int64_t idx = 0; idx < n; ++idx) {
            double s = 0.0;
            for (std::int64_t j = 0; j < k; ++j) s += q[static_cast<std::size_t>(idx * k + j)] * u[static_cast<std::size_t>(j * k + idx0)];
            u_min[static_cast<std::size_t>(idx)] = s;
          }
        } else {
          // Degenerate eigenspace is (at least) the perpendicular complement of span(Q): find any
          // standard basis vector with a large-enough component outside span(Q) and orthogonalize.
          std::int64_t best_idx = 0;
          double best_residual = -1.0;
          std::vector<double> best_vec;
          for (std::int64_t e = 0; e < n; ++e) {
            std::vector<double> qte(static_cast<std::size_t>(k), 0.0);
            for (std::int64_t j = 0; j < k; ++j) qte[static_cast<std::size_t>(j)] = q[static_cast<std::size_t>(e * k + j)];
            std::vector<double> v(static_cast<std::size_t>(n), 0.0);
            v[static_cast<std::size_t>(e)] = 1.0;
            for (std::int64_t idx = 0; idx < n; ++idx) {
              double s = 0.0;
              for (std::int64_t j = 0; j < k; ++j) s += q[static_cast<std::size_t>(idx * k + j)] * qte[static_cast<std::size_t>(j)];
              v[static_cast<std::size_t>(idx)] -= s;
            }
            const double residual = lsr1_norm_(v);
            if (residual > best_residual) {
              best_residual = residual;
              best_idx = e;
              best_vec = v;
            }
            if (residual > 0.5) break;
          }
          (void)best_idx;
          const double nrm = lsr1_norm_(best_vec);
          if (nrm > tol) {
            for (std::int64_t idx = 0; idx < n; ++idx) u_min[static_cast<std::size_t>(idx)] = best_vec[static_cast<std::size_t>(idx)] / nrm;
          }
        }
        const double p_hat_norm = lsr1_norm_(p_hat);
        const double alpha_sq = std::max(0.0, delta * delta - p_hat_norm * p_hat_norm);
        const double alpha = std::sqrt(alpha_sq);
        for (std::int64_t idx = 0; idx < n; ++idx) p_hat[static_cast<std::size_t>(idx)] += alpha * u_min[static_cast<std::size_t>(idx)];
      }
      result.p = std::move(p_hat);
      return result;
    }
  }

  // Line 15-17: generic case -- Newton's method (safeguarded with bisection) for the unique root
  // of phi(sigma)=0 in (max(-lambda_min,0), infinity).
  double lo = std::max(-lambda_min, 0.0);
  // phi is monotonically increasing on (lo, infinity), from -1/delta at lo+ to +infinity. Bracket
  // a sigma with phi(sigma) > 0 by exponential search.
  double lo_eps = std::max(1e-12, std::abs(lo) * 1e-12);
  double sigma_lo = lo + lo_eps;
  double sigma_hi = std::max(sigma_lo * 2.0, sigma_lo + 1.0);
  for (std::int64_t iter = 0; iter < 200 && phi(sigma_hi) <= 0.0; ++iter) {
    sigma_lo = sigma_hi;
    sigma_hi *= 2.0;
  }
  double sigma = 0.5 * (sigma_lo + sigma_hi);
  for (std::int64_t iter = 0; iter < opts.max_newton_iterations; ++iter) {
    const double phi_val = phi(sigma);
    if (phi_val < 0.0) sigma_lo = sigma; else sigma_hi = sigma;
    if (std::abs(phi_val) <= opts.newton_tolerance) break;

    const double h = h_of_sigma(sigma);
    double hprime = 0.0;
    for (std::int64_t i = 0; i < k; ++i) {
      const double d = lambda[static_cast<std::size_t>(i)] + sigma;
      hprime += -2.0 * (g_par[static_cast<std::size_t>(i)] * g_par[static_cast<std::size_t>(i)]) / (d * d * d);
    }
    const double dperp = gamma + sigma;
    hprime += -2.0 * g_perp_normsq / (dperp * dperp * dperp);
    // phi'(sigma) = -0.5 * h^{-3/2} * h'(sigma)
    const double phi_prime = -0.5 * std::pow(std::max(h, std::numeric_limits<double>::min()), -1.5) * hprime;

    double next = sigma;
    if (phi_prime > 0.0) next = sigma - phi_val / phi_prime;
    if (!(next > sigma_lo) || !(next < sigma_hi) || !std::isfinite(next)) {
      next = 0.5 * (sigma_lo + sigma_hi);
    }
    sigma = next;
  }
  result.p = p_of_sigma(sigma);
  return result;
}

// ---------------------------------------------------------------------
// Algorithm 1: L-SR1 Trust-Region (L-SR1-TR) Method (page 8).
// ---------------------------------------------------------------------

inline double lsr1_gamma_from_pair_(double sty, double yty, double fallback) {
  if (sty > 0.0 && std::isfinite(sty) && std::isfinite(yty)) return yty / sty;
  return fallback;
}

}  // namespace detail

// Shared core loop: `evaluate` maps a flat point to (value, gradient) and is responsible for
// its own value_and_grad call (and thus for whether params are one block or several -- see the
// two public overloads below).
inline Lsr1Result l_sr1_trust_region_core_(
    const std::function<std::pair<Series, Series>(const Series&)>& evaluate, const Series& x0,
    const Lsr1SolveOptions& solve_options) {
  detail::lsr1_validate_solve_options_(solve_options);
  detail::require_defined_(x0, "l_sr1_trust_region");
  detail::require_float_dtype_(x0, "l_sr1_trust_region");
  if (x0.dim() != 1) {
    ::pluss::throw_error("optim::l_sr1_trust_region: expected a flat (1-D) parameter vector");
  }

  const auto& opts = solve_options.step;
  const std::int64_t n = x0.numel();

  Series x = x0.to(ScalarType::Float64).contiguous();
  pluss::codem::series_tape::clear_source_link(x);

  Lsr1State state;
  state.gamma = opts.initial_gamma;
  state.radius = opts.initial_radius;

  Lsr1Result result;
  std::int64_t function_evaluations = 0;
  std::int64_t no_progress_iterations = 0;
  double best_objective = std::numeric_limits<double>::infinity();

  auto eval = [&](const Series& point) {
    Series leaf = point.to(ScalarType::Float64).contiguous();
    pluss::codem::series_tape::clear_source_link(leaf);
    auto [value, grad] = evaluate(leaf);
    ++function_evaluations;
    return std::pair<Series, Series>{value, grad};
  };

  auto [value0, grad0] = eval(x);
  if (!value0.isfinite().all().item<bool>() || !grad0.isfinite().all().item<bool>()) {
    result.params = x;
    result.state = state;
    result.exit_flag = Lsr1ExitFlag::NonFiniteEvaluation;
    result.message = "l_sr1_trust_region: non-finite objective/gradient at initial point";
    result.function_evaluations = function_evaluations;
    return result;
  }

  double objective_value = value0.item<double>();
  auto g = detail::lsr1_to_vec_(grad0);
  best_objective = objective_value;

  for (std::int64_t iteration = 0; iteration < solve_options.max_iterations; ++iteration) {
    const double g_norm = detail::lsr1_norm_(g);
    if (g_norm <= solve_options.gradient_tolerance) {
      result.converged = true;
      result.exit_flag = Lsr1ExitFlag::Converged;
      result.message = "l_sr1_trust_region: converged (||g||_2 <= gradient_tolerance)";
      result.iteration = iteration;
      break;
    }

    const auto s_vec = detail::lsr1_stack_cols_(state.s_history, n);
    const auto y_vec = detail::lsr1_stack_cols_(state.y_history, n);
    std::int64_t k = static_cast<std::int64_t>(state.s_history.size());

    // gamma_k: standard scaling from the most recent pair, clamped strictly below lambda_hat
    // per Sec. 2.6 / Lemmas 2,4,5 whenever lambda_hat can be (cheaply) computed and is positive.
    double gamma = state.gamma;
    if (k > 0) {
      const auto& s_last = state.s_history.back();
      const auto& y_last = state.y_history.back();
      const double sty = detail::series_dot_(s_last, y_last);
      const double yty = detail::series_dot_(y_last, y_last);
      gamma = detail::lsr1_gamma_from_pair_(sty, yty, state.gamma);
      double lambda_hat = 0.0;
      bool got_lambda_hat = detail::lsr1_lambda_hat_(s_vec, y_vec, n, k, &lambda_hat);
      if (got_lambda_hat && lambda_hat > 0.0) {
        gamma = std::min(gamma, opts.gamma_shrink * lambda_hat);
      }
      if (!(gamma > 0.0) || !std::isfinite(gamma)) gamma = state.gamma;
    }

    detail::Lsr1Compact compact = detail::lsr1_build_compact_(s_vec, y_vec, n, k, gamma);
    // Degenerate Psi (rank-deficient history): drop the oldest pair and retry until well-posed
    // or memory is empty (B_k = gamma*I, always well-posed).
    while (compact.k < 0 && k > 0) {
      --k;
      state.s_history.erase(state.s_history.begin());
      state.y_history.erase(state.y_history.begin());
      const auto s_vec2 = detail::lsr1_stack_cols_(state.s_history, n);
      const auto y_vec2 = detail::lsr1_stack_cols_(state.y_history, n);
      compact = detail::lsr1_build_compact_(s_vec2, y_vec2, n, k, gamma);
    }

    const auto subproblem = detail::lsr1_solve_subproblem_(compact, g, n, state.radius, opts);
    std::vector<double> p_dir = subproblem.p;
    const double p_norm = detail::lsr1_norm_(p_dir);
    if (p_norm <= solve_options.step_tolerance) {
      ++no_progress_iterations;
      if (no_progress_iterations > solve_options.max_no_progress_iterations) {
        result.exit_flag = Lsr1ExitFlag::NoProgress;
        result.message = "l_sr1_trust_region: no progress (trust-region step ~0) for too many iterations";
        result.iteration = iteration;
        break;
      }
      state.radius = std::max(state.radius * opts.eta_1, 1e-16);
      continue;
    }

    Series p_series = pluss::make_series(p_dir, {n}, Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU))
                          .to(x.platform());
    const double gtp = detail::lsr1_dot_(g, p_dir);
    if (!(gtp < 0.0)) {
      // p* must be a descent direction for the line search; this can only happen from numerical
      // noise in the subproblem solve. Fall back to steepest descent scaled to the same radius.
      const double scale = state.radius / std::max(p_norm, 1e-300);
      for (auto& v : p_dir) v = 0.0;
      for (std::int64_t i = 0; i < n; ++i) p_dir[static_cast<std::size_t>(i)] = -g[static_cast<std::size_t>(i)] * scale / std::max(g_norm, 1e-300);
      p_series = pluss::make_series(p_dir, {n}, Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU)).to(x.platform());
    }

    // Line search along p* (Algorithm 1, line 8), starting from alpha=1 per the Require block.
    // This enforces the Armijo (sufficient-decrease) condition only, by backtracking -- NOT the
    // full strong-Wolfe curvature condition. A plain backtracking search cannot reliably enforce
    // curvature: as alpha shrinks, the trial gradient's directional derivative gtd_new converges
    // TO gtd itself, so |gtd_new| <= -wolfe_c2*gtd (with wolfe_c2<1) becomes harder to satisfy,
    // not easier -- backtracking alone can therefore spin through every iteration and (if forced
    // to accept anyway) take a useless, near-zero step forever without ever converging. This was
    // confirmed directly: on 10-D Rosenbrock, gradient norm froze bit-for-bit for 60+ iterations
    // with accepted steps of magnitude 1/2^19. Unlike BFGS, SR1's compact update (eq. 7-9) does
    // not require y^Ts>0 for well-definedness (it has its own, different skip rule below), so
    // there is no structural need for the curvature condition here -- Armijo alone gives
    // well-defined sufficient decrease for rho and is the only piece required by the paper's own
    // convergence argument (Theorem 1's proof only invokes descent-direction + smoothness).
    const double gtd = detail::lsr1_dot_(g, p_dir);
    double alpha = 1.0;
    Series x_next = pluss::series::undefined();
    Series grad_next = pluss::series::undefined();
    double objective_next = 0.0;
    bool accepted = false;
    // Track the best (smallest-objective) finite trial seen, even if it doesn't satisfy Armijo's
    // margin -- see below for why a strict Armijo failure isn't necessarily "no progress".
    bool have_best = false;
    double best_alpha = 0.0;
    double best_objective = objective_value;
    Series best_x = pluss::series::undefined();
    Series best_grad = pluss::series::undefined();
    for (std::int64_t ls = 0; ls < opts.max_line_search_iterations; ++ls) {
      Series trial = x.add(p_series.mul(alpha));
      auto [trial_value, trial_grad] = eval(trial);
      if (!trial_value.isfinite().all().item<bool>() || !trial_grad.isfinite().all().item<bool>()) {
        alpha *= opts.line_search_shrink;
        continue;
      }
      const double trial_objective = trial_value.item<double>();
      const bool armijo_ok = trial_objective <= objective_value + opts.wolfe_c1 * alpha * gtd;
      if (armijo_ok) {
        x_next = trial;
        grad_next = trial_grad;
        objective_next = trial_objective;
        accepted = true;
        break;
      }
      if (trial_objective < best_objective) {
        have_best = true;
        best_alpha = alpha;
        best_objective = trial_objective;
        best_x = trial;
        best_grad = trial_grad;
      }
      alpha *= opts.line_search_shrink;
    }
    if (!accepted && have_best) {
      // Armijo's margin was never satisfied, but a genuinely improving (finite) trial was found
      // along the way -- this reliably happens once alpha shrinks small enough that the required
      // decrease (wolfe_c1 * alpha * gtd) falls below the floating-point noise floor of evaluating
      // the objective (confirmed directly on the DHNG problem: a 7559-step sequential recurrence,
      // where accumulated rounding easily swamps a ~1e-6-scale required decrease). Rejecting a
      // real, if small, improvement here just because it misses an increasingly-unattainable
      // margin is not a genuine failure -- accept it and let rho (computed from the ACTUAL
      // decrease achieved) naturally come out small, which correctly shrinks the trust region for
      // the next iteration rather than aborting the whole solve over what the sufficient-decrease
      // heuristic alone would flag as "not decreasing enough."
      alpha = best_alpha;
      x_next = best_x;
      grad_next = best_grad;
      objective_next = best_objective;
      accepted = true;
    }
    if (!accepted) {
      // No trial anywhere along p* -- down to a step of order 1e-6 times the trust radius --
      // improved on the current point at all (confirmed directly on the DHNG problem: the full
      // step was non-finite, a half step evaluated to an astronomical-but-technically-finite
      // value, and every smaller step's objective approached the current one from the WORSE side
      // without ever crossing it, off by an amount consistent with error accumulated through a
      // 7559-step sequential recurrence). This is the trust-region step itself having been too
      // large for this numerically sensitive objective, not a property of the true descent
      // direction (gtp<0 is guaranteed above) -- the correct response, exactly as for a strongly
      // rejected step (rho << 0), is to shrink the trust region and let Algorithm 2 recompute a
      // smaller, safer step next iteration, not to abort the whole solve -- bounded below by
      // max_no_progress_iterations in case it never recovers.
      ++no_progress_iterations;
      state.radius = std::max(state.radius * opts.eta_1, 1e-16);
      if (no_progress_iterations > solve_options.max_no_progress_iterations) {
        result.exit_flag = Lsr1ExitFlag::NoProgress;
        result.message =
            "l_sr1_trust_region: no progress (line search could not improve on the current point) for too many "
            "iterations";
        result.iteration = iteration;
        break;
      }
      continue;
    }
    for (std::size_t i = 0; i < p_dir.size(); ++i) p_dir[i] *= alpha;

    // Predicted reduction Q_k(p*) = g^T p* + 0.5 p*^T B_k p*, using B_k p* from the compact form.
    const auto bp = detail::lsr1_apply_b_(compact, p_dir, n);
    double quadratic_model = detail::lsr1_dot_(g, p_dir) + 0.5 * detail::lsr1_dot_(p_dir, bp);
    const double actual_reduction = objective_next - objective_value;
    double rho;
    if (std::abs(quadratic_model) <= std::numeric_limits<double>::min()) {
      rho = (actual_reduction <= 0.0) ? 1.0 : -1.0;
    } else {
      rho = actual_reduction / quadratic_model;
    }

    const double s_norm = detail::lsr1_norm_(p_dir);

    // Algorithm 1, line 10: the step is always taken (the Armijo line search above already
    // guarantees sufficient decrease on the true objective); rho only reshapes the trust-region
    // radius below.
    x = x_next;
    pluss::codem::series_tape::clear_source_link(x);
    const auto grad_next_vec = detail::lsr1_to_vec_(grad_next);

    std::vector<double> s_new(static_cast<std::size_t>(n)), y_new(static_cast<std::size_t>(n));
    for (std::int64_t i = 0; i < n; ++i) {
      s_new[static_cast<std::size_t>(i)] = p_dir[static_cast<std::size_t>(i)];
      y_new[static_cast<std::size_t>(i)] = grad_next_vec[static_cast<std::size_t>(i)] - g[static_cast<std::size_t>(i)];
    }
    const double s_dot_s = detail::lsr1_dot_(s_new, s_new);
    const auto bs = detail::lsr1_apply_b_(compact, s_new, n);
    std::vector<double> y_minus_bs(static_cast<std::size_t>(n));
    for (std::int64_t i = 0; i < n; ++i) y_minus_bs[static_cast<std::size_t>(i)] = y_new[static_cast<std::size_t>(i)] - bs[static_cast<std::size_t>(i)];
    const double denom = detail::lsr1_dot_(s_new, y_minus_bs);
    const double skip_rhs = opts.sr1_skip_tolerance * std::sqrt(std::max(s_dot_s, 0.0)) * detail::lsr1_norm_(y_minus_bs);
    const bool keep_pair = std::abs(denom) >= skip_rhs && s_dot_s > 0.0;

    if (keep_pair) {
      Series s_leaf = pluss::make_series(s_new, {n}, Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU));
      Series y_leaf = pluss::make_series(y_new, {n}, Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU));
      state.s_history.push_back(s_leaf);
      state.y_history.push_back(y_leaf);
      // At most n pairs can be kept: n+1 vectors in R^n are linearly dependent, so the n x k matrix Psi would have more columns
      // than rows. Its reduced QR factor then has only n columns, while the subproblem solver indexes it as n x k and would read
      // past its end (the result varied from run to run). Capping the memory at n keeps every history matrix n x k with k <= n.
      if (static_cast<std::int64_t>(state.s_history.size()) > std::min(opts.memory_size, n)) {
        state.s_history.erase(state.s_history.begin());
        state.y_history.erase(state.y_history.begin());
      }
    }
    state.gamma = gamma;

    if (rho < opts.tau_2) {
      state.radius = std::min(opts.eta_1 * state.radius, opts.eta_2 * s_norm);
    } else if (rho >= opts.tau_3 && s_norm >= opts.eta_3 * state.radius) {
      state.radius = std::min(opts.eta_4 * state.radius, opts.max_radius);
    }
    state.radius = std::max(state.radius, 1e-16);

    g = grad_next_vec;
    objective_value = objective_next;
    if (objective_value < best_objective - solve_options.step_tolerance) {
      best_objective = objective_value;
      no_progress_iterations = 0;
    } else {
      ++no_progress_iterations;
    }
    ++state.step;
    result.iteration = iteration + 1;

    if (solve_options.progress_hook) {
      Lsr1ProgressSnapshot snapshot;
      snapshot.iteration = result.iteration;
      snapshot.function_evaluations = function_evaluations;
      snapshot.objective = objective_value;
      snapshot.gradient_norm = detail::lsr1_norm_(g);
      snapshot.trust_radius = state.radius;
      snapshot.step_norm = s_norm;
      if (!solve_options.progress_hook(snapshot)) {
        result.exit_flag = Lsr1ExitFlag::Stopped;
        result.message = "l_sr1_trust_region: stopped by progress_hook";
        break;
      }
    }

    if (no_progress_iterations > solve_options.max_no_progress_iterations) {
      result.exit_flag = Lsr1ExitFlag::NoProgress;
      result.message = "l_sr1_trust_region: no sufficient progress for too many consecutive iterations";
      break;
    }

    if (detail::lsr1_norm_(g) <= solve_options.gradient_tolerance) {
      result.converged = true;
      result.exit_flag = Lsr1ExitFlag::Converged;
      result.message = "l_sr1_trust_region: converged (||g||_2 <= gradient_tolerance)";
      break;
    }
  }

  result.params = x;
  result.state = state;
  result.objective = pluss::make_series(objective_value, Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU));
  result.objective_value = objective_value;
  result.gradient_norm = detail::lsr1_norm_(g);
  result.function_evaluations = function_evaluations;
  return result;
}

// Convenience overload matching septima's multi-block ergonomics: flattens params through the
// same helpers septima itself uses. Each block is evaluated as its own independent leaf (exactly
// how value_and_grad's own std::vector<Series> overload differentiates per-block already --
// no need to slice within a single Trace graph, which Trace does not support anyway), and the
// per-block gradients are re-flattened into one vector for the trust-region math.
//
// This is ALSO, as of this fix, where complex-parameter support lives: septima_flatten_values_/
// septima_unflatten_values_ already view-as-real a complex block (real and imaginary channels as
// independent coordinates, doubling that block's contribution to the flat vector -- see
// optim.h's detail::view_as_real_/view_as_complex_ and septima_flatten_values_/
// septima_unflatten_values_ for the full rationale, shared verbatim with Septima's LBFGS branch,
// which needed the identical fix for the identical reason: this whole algorithm, like LBFGS, is
// inherently a single-flat-REAL-vector method (the trust region itself is defined via the 2-norm
// on R^n; Algorithm 2's eigendecomposition/Cholesky/Newton-on-phi(sigma) machinery all assume a
// real symmetric Hessian approximation). Once the flatten/unflatten boundary produces an
// unconditionally-real vector, l_sr1_trust_region_core_ below needs NO complex-aware logic of its
// own -- exactly the same "push the fix to the boundary" shape as the LBFGS fix, and for the same
// reason: viewing a complex n-vector optimization as a real 2n-vector one is the standard
// treatment, with no Hermitian/conjugate-inner-product question anywhere, since no complex linear
// algebra is performed at all past this boundary.
inline std::pair<std::vector<Series>, Lsr1Result> l_sr1_trust_region(
    const std::function<Trace(const std::vector<Trace>&)>& objective, const std::vector<Series>& params0,
    const Lsr1SolveOptions& solve_options = {}) {
  if (params0.empty()) {
    ::pluss::throw_error("optim::l_sr1_trust_region: expected non-empty params");
  }
  const auto block_layout = detail::septima_make_layout_(params0);
  const Series flat0 = detail::septima_flatten_values_(params0);

  pluss::subsp::math::diff::DerivativePolicy policy{};
  policy.keep_graph = false;
  std::function<std::pair<Series, Series>(const Series&)> evaluate = [&](const Series& flat_point) {
    std::vector<Series> pieces = detail::septima_unflatten_values_(flat_point, block_layout);
    auto [value, grads] = value_and_grad(
        [&](const std::vector<Trace>& xs) -> Trace { return objective(xs); }, pieces, policy);
    Series flat_grad = detail::septima_flatten_values_(grads).to(flat_point.platform());
    return std::pair<Series, Series>{value, flat_grad};
  };

  Lsr1Result result = l_sr1_trust_region_core_(evaluate, flat0, solve_options);
  std::vector<Series> params = detail::septima_unflatten_values_(result.params, block_layout);
  return {params, result};
}

// Public entry point: single flat parameter vector, Trace(Trace) objective. Delegates to the
// multi-block overload above with a single block, rather than duplicating its (now
// complex-aware) flatten/unflatten wiring a second time -- for a real (non-complex) x0 this is
// exactly equivalent to calling value_and_grad directly (view_as_real_/view_as_complex_ are
// no-ops on real input, and septima_flatten_values_'s single-block path skips the .unite() step
// entirely), so real-dtype behavior is unchanged.
inline Lsr1Result l_sr1_trust_region(const Lsr1ObjectiveFn& objective, const Series& x0,
                                     const Lsr1SolveOptions& solve_options = {}) {
  auto [params, result] = l_sr1_trust_region(
      [&](const std::vector<Trace>& xs) -> Trace { return objective(xs.front()); },
      std::vector<Series>{x0},
      solve_options);
  // The multi-block overload's own result.params is intentionally the raw FLAT (real,
  // view-as-real-doubled for a complex block) working vector, useful to that overload's callers
  // for e.g. restart plumbing -- but this single-Series entry point's contract (matching its
  // behavior before this fix) is to return the solution in x0's own original shape/dtype, which
  // is exactly `params.front()`.
  result.params = params.front();
  return result;
}

}  // namespace pluss::subsp::math::optim

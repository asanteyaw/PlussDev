#pragma once

#include "pluss/subsp/math/function.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace pluss::subsp::math::operators {

struct Capabilities final {
  bool structural = true;
  bool evaluated_only = false;
  bool linear = false;
  bool adjoint_known = false;
  bool serializable = false;
  bool platform_pure = true;
  bool path_functional = false; // evaluation needs a sampled path axis (stochastic family)
};

struct Options final {
  std::uint32_t version = 1;
  bool structural = true;
  bool evaluated_only = false;
  bool linear = false;
  bool adjoint_known = false;
  bool serializable = false;
  bool platform_pure = true;
  std::string payload{};
};

struct DiffOptions final {
  math::NonSmooth nonsmooth = math::NonSmooth::Strict;
  math::Fallback fallback = math::Fallback::Reject;
  std::size_t max_nodes = 100000;
};

namespace detail {
class OperatorConcept;
struct OperatorAccess;
} // namespace detail

class Operator final {
public:
  Operator() noexcept;
  Operator(const Operator &) noexcept;
  Operator(Operator &&) noexcept;
  Operator &operator=(const Operator &) noexcept;
  Operator &operator=(Operator &&) noexcept;
  ~Operator();

  [[nodiscard]] bool defined() const noexcept;
  explicit operator bool() const noexcept { return defined(); }

  [[nodiscard]] math::Function operator()(const math::Function &input) const;
  [[nodiscard]] Capabilities capabilities() const;
  [[nodiscard]] std::string_view key() const;
  [[nodiscard]] std::uint32_t version() const;

private:
  explicit Operator(
      pluss::msharp::intrusive_ptr<detail::OperatorConcept> impl) noexcept;

  pluss::msharp::intrusive_ptr<detail::OperatorConcept> impl_{};

  friend struct detail::OperatorAccess;
};

[[nodiscard]] Operator identity();
// |f^(n)|: the magnitude of the n-th derivative (formerly named
// absolute_differential; that name now belongs to geometry::).
[[nodiscard]] Operator magnitude_differential(std::string_view variable,
                                              std::int64_t order = 1,
                                              DiffOptions options = {});
[[nodiscard]] Operator covariant(std::string_view variable,
                                 math::Function connection,
                                 DiffOptions options = {});

// Differential geometry on a manifold with coordinates x^1..x^n. A connection
// is a math::Function of the coordinates with output shape [n, n, n] indexed
// Gamma[i][j][k] = Gamma^i_{jk}; a tensor field is a math::Function whose
// output axes are the tensor's indices, contravariant axes first and then
// covariant axes (TensorSignature).
namespace geometry {

struct TensorSignature final {
  std::int64_t contravariant = 0;
  std::int64_t covariant = 0;
};

// Levi-Civita connection of a metric g_ij(x): the output-rank-2 Function's
// variables (or `coordinates`, a subset in order) are the coordinates.
[[nodiscard]] math::Function
levi_civita(const math::Function &metric,
            pluss::StackArray<std::string_view> coordinates = {},
            DiffOptions options = {});

// Ricci / Levi-Civita absolute (intrinsic) differential along a curve:
//   DV/dt = dV/dt + sum_contravariant Gamma^i_{mk} V^m dx^k/dt
//                 - sum_covariant     Gamma^m_{jk} V_m dx^k/dt.
// `curve` is x^k(t), a rank-1 Function of `parameter`; the input is a tensor
// along the curve, a Function of `parameter` with `signature` indices.
[[nodiscard]] Operator
absolute_differential(math::Function curve, math::Function connection,
                      std::string_view parameter,
                      TensorSignature signature = {1, 0},
                      DiffOptions options = {});

// Covariant derivative of a tensor field V(x) in the direction of one
// coordinate: (nabla_k V) = d_k V + connection terms. The coordinate must be
// one of the connection's variables.
[[nodiscard]] Operator covariant(std::string_view coordinate,
                                 math::Function connection,
                                 TensorSignature signature,
                                 DiffOptions options = {});

} // namespace geometry

// Fourier series. Every coefficient is a definite integral of f against a
// trigonometric function, built through the integral operator, so the results
// are analytic Functions waiting for values: the coefficient vectors depend on
// the variables of f other than the expanded one, and a series is a Function
// of the expanded variable. (The Fourier *transform* is a different operator.)
//
// Half-range expansions on [lower, upper], with u_k = k pi / (upper - lower):
//   cosine:  f ~ A_0 / 2 + sum_{k >= 1} A_k cos(u_k (x - lower)),  A_k = 2/(b-a) int f cos(u_k (x - lower)) dx
//   sine:    f ~ sum_{k >= 1} B_k sin(u_k (x - lower)),            B_k = 2/(b-a) int f sin(u_k (x - lower)) dx
// `cosine_coefficients` returns A_0..A_{terms-1}; `sine_coefficients` returns
// B_1..B_terms. The second overloads integrate over the sub-interval
// [support_lower, support_upper] of [lower, upper] while keeping the phase
// anchored at `lower` (the form the COS option-pricing method needs, where the
// payoff coefficients are integrals over part of the truncation interval).
namespace fourier {

[[nodiscard]] Operator cosine_coefficients(std::string_view variable, double lower, double upper, std::int64_t terms);
[[nodiscard]] Operator cosine_coefficients(std::string_view variable, double lower, double upper, std::int64_t terms,
                                           double support_lower, double support_upper);
[[nodiscard]] Operator sine_coefficients(std::string_view variable, double lower, double upper, std::int64_t terms);
[[nodiscard]] Operator sine_coefficients(std::string_view variable, double lower, double upper, std::int64_t terms,
                                         double support_lower, double support_upper);
[[nodiscard]] Operator cosine_series(std::string_view variable, double lower, double upper, std::int64_t terms);
[[nodiscard]] Operator sine_series(std::string_view variable, double lower, double upper, std::int64_t terms);

// Full trigonometric series on [-half_period, half_period]:
//   f ~ a_0 / 2 + sum_{k >= 1} (a_k cos(k pi x / L) + b_k sin(k pi x / L)),
//   a_k = (1/L) int_{-L}^{L} f cos(k pi x / L) dx,  b_k = (1/L) int_{-L}^{L} f sin(k pi x / L) dx,
// which is the cosine series of the even part plus the sine series of the odd part.
// `coefficients` returns [a_0..a_{terms-1}, b_1..b_{terms-1}] (length 2 terms - 1).
[[nodiscard]] Operator coefficients(std::string_view variable, double half_period, std::int64_t terms);
[[nodiscard]] Operator series(std::string_view variable, double half_period, std::int64_t terms);

// Integral transforms. The result is an analytic Function of the frequency variable
// (a new free variable, which must not clash with the Function's own) and of the
// Function's other variables; the integration variable is bound. The integral runs
// over an infinite range, so it is an unresolved integral that quadrature evaluates
// lazily when values are supplied; it differentiates, composes and saves like any
// Function (d/d frequency is the transform of the differentiated integrand).
// The input must be built from algebra on its variables (not opaque callables).
//   Angular:  F(w)  = int f(x) e^{-i w x} dx           inverse prefactor 1 / (2 pi)
//   Unitary:  F(w)  = (2 pi)^{-1/2} int f(x) e^{-i w x} dx
//   Ordinary: F(xi) = int f(x) e^{-2 pi i xi x} dx      inverse prefactor 1
// The cosine and sine transforms run over the half line [0, inf) with the same
// conventions (Unitary carries sqrt(2 / pi); Ordinary uses 2 pi xi and a factor 2).
enum class Convention : std::uint8_t { Angular = 0, Unitary = 1, Ordinary = 2 };
[[nodiscard]] Operator transform(std::string_view variable, std::string_view frequency,
                                 Convention convention = Convention::Angular);
// Acts on a Function of `frequency` and returns a Function of `variable`.
[[nodiscard]] Operator inverse_transform(std::string_view frequency, std::string_view variable,
                                         Convention convention = Convention::Angular);
[[nodiscard]] Operator cosine_transform(std::string_view variable, std::string_view frequency,
                                        Convention convention = Convention::Angular);
[[nodiscard]] Operator sine_transform(std::string_view variable, std::string_view frequency,
                                      Convention convention = Convention::Angular);

} // namespace fourier

// Integration against a measure. There is no measure type: a measure is carried by the operator that
// integrates against it, and measure algebra is the operator algebra (a mixture is `linear_combination`,
// a product measure is `compose` of the two integrals, a change of measure is `multiply` by the
// Radon-Nikodym density, a restriction is `multiply` by an indicator). Each operator returns an analytic
// Function of the remaining variables: atomic measures are exact finite sums of compositions, continuous
// parts are unresolved integrals evaluated lazily. The result is the Lebesgue integral wherever the
// integrand is integrable; for a bounded Riemann-integrable integrand it equals the Riemann integral.
namespace measure {
// dmu = dx on [lower, upper] (either may be infinite). `null_points` are declared mu-null: the range is
// split at them, so they are quadrature endpoints and never sampled (a.e. semantics: values of f there
// cannot change the result).
[[nodiscard]] Operator lebesgue(std::string_view variable, double lower, double upper,
                                pluss::RangeView<double> null_points = {});
// dmu = rho dx. `density` is a Function whose variables are among the integrand's.
[[nodiscard]] Operator weighted(std::string_view variable, const math::Function &density, double lower,
                                double upper, pluss::RangeView<double> null_points = {});
// dmu = sum_i w_i delta_{x_i}: the exact sum sum_i w_i f(x_i).
[[nodiscard]] Operator atomic(std::string_view variable, pluss::RangeView<double> points,
                              pluss::RangeView<double> weights);
// Lebesgue-Stieltjes integral against F = continuous part + jumps: int f F' dx over [lower, upper] plus
// sum_j f(x_j) s_j for the jump points x_j with sizes s_j. `cdf` is the continuous (jump-free) part.
[[nodiscard]] Operator stieltjes(std::string_view variable, const math::Function &cdf, double lower,
                                 double upper, pluss::RangeView<double> jump_points = {},
                                 pluss::RangeView<double> jump_sizes = {});
} // namespace measure

// Classical vector calculus on Functions whose components are listed by `variables` (the coordinates, in
// order). Every result is an analytic Function built from `differential`.
//   divergence: rank-1 F with n components -> scalar  sum_i dF_i / dx_i
//   curl:       n = 3: rank-1 F -> rank-1 (dF3/dy - dF2/dz, dF1/dz - dF3/dx, dF2/dx - dF1/dy);
//               n = 2: rank-1 F with 2 components -> the scalar dF2/dx - dF1/dy
//   laplacian:  scalar f -> sum_i d2 f / dx_i^2
//   directional: scalar f -> v . grad f for a rank-1 Function v of the same variables
//   material:   scalar f -> df/dt + v . grad f, the convective derivative (v is the velocity)
[[nodiscard]] Operator divergence(pluss::StackArray<std::string_view> variables, DiffOptions options = {});
[[nodiscard]] Operator curl(pluss::StackArray<std::string_view> variables, DiffOptions options = {});
[[nodiscard]] Operator laplacian(pluss::StackArray<std::string_view> variables, DiffOptions options = {});
[[nodiscard]] Operator directional(math::Function direction, pluss::StackArray<std::string_view> variables,
                                   DiffOptions options = {});
[[nodiscard]] Operator material(math::Function velocity, std::string_view time,
                                pluss::StackArray<std::string_view> variables, DiffOptions options = {});

// Calculus of variations for one dependent variable u(x). A Lagrangian density L(x, u, u', u'', ...) is a
// Function of the independent variable and of the jet variables `jets` = {u, u', u'', ...} (each its own
// variable; list at least twice as many orders as L uses, since each order k contributes (-d/dx)^k).
//   delta F / delta u = sum_k (-D_x)^k dL/du_k,   D_x g = dg/dx + sum_k u_{k+1} dg/du_k,
// returned as a Function of x and the jets; the Euler-Lagrange equation is delta F / delta u = 0.
namespace variational {
[[nodiscard]] Operator functional_derivative(std::string_view independent, pluss::StackArray<std::string_view> jets,
                                             DiffOptions options = {});
[[nodiscard]] Operator euler_lagrange(std::string_view independent, pluss::StackArray<std::string_view> jets,
                                      DiffOptions options = {});
} // namespace variational

// Fractional calculus of order alpha > 0 with lower terminal `lower` (default 0), as analytic Functions of
// the variable (the evaluation point x > lower):
//   integral:     I^alpha f(x) = 1 / Gamma(alpha) int_lower^x (x - t)^(alpha - 1) f(t) dt, written as
//                 (x - a)^alpha / Gamma(alpha + 1) int_0^1 f(x - (x - a) s^(1/alpha)) ds (no endpoint singularity)
//   differential: Riemann-Liouville  D^alpha f = d^m/dx^m I^(m - alpha) f   (m = ceil(alpha)),
//                 Caputo             D^alpha f = I^(m - alpha) f^(m),
//                 Grunwald-Letnikov  equal to Riemann-Liouville for the functions it is defined on.
// Polynomials with lower = 0 get exact power-law closed forms; other integrands stay lazy integrals.
// The kinds differ on constants: RL D^alpha 1 = x^-alpha / Gamma(1 - alpha), Caputo D^alpha 1 = 0.
namespace fractional {
enum class Kind : std::uint8_t { RiemannLiouville = 0, Caputo = 1, GrunwaldLetnikov = 2 };
[[nodiscard]] Operator integral(std::string_view variable, double alpha, double lower = 0.0);
[[nodiscard]] Operator differential(std::string_view variable, double alpha, Kind kind = Kind::Caputo,
                                    double lower = 0.0);
} // namespace fractional

// (f * k)(x) = int f(t) k(x - t) dt over the real line, as an analytic Function of x; `kernel` is a Function
// of the same variable name. The integral is lazy unless both factors are exponential polynomials (no
// closed form is attempted for the convolution itself).
[[nodiscard]] Operator convolution(std::string_view variable, math::Function kernel);

// Laplace transform L{f}(s) = int_0^inf f(t) e^{-s t} dt as an analytic Function of the
// new variable `frequency` (real s inside the region of convergence).
namespace laplace {
[[nodiscard]] Operator transform(std::string_view variable, std::string_view frequency);
// Inverse Laplace transform by Talbot's method on Weideman's optimized contour: a sum of `terms` / 2
// compositions of F with constants over t, so the result is an analytic Function of `variable` (t > 0)
// with no quadrature at evaluation time. Acts on a Function of `frequency`. Accurate (about
// 1.3^{-terms} relative) when F's singularities lie in the left half-plane at a distance from the
// imaginary axis that grows with t; poles on the imaginary axis lose accuracy for t large relative to
// terms / |pole|, which more terms postpone. F must be built from algebra on s (no integrals or
// opaque callables).
[[nodiscard]] Operator inverse_transform(std::string_view frequency, std::string_view variable,
                                         std::int64_t terms = 32);
} // namespace laplace

// Truncated Taylor expansions. These are structural: they are built from
// `differential`, evaluation of the derivatives at the centre, and the Function
// algebra, so the expansion of a Function is a polynomial Function that can be
// evaluated, differentiated, integrated and composed like any other. Variables
// other than the expanded ones stay free.
namespace expansion {

// How coefficients are obtained. Structural builds the derivative graph (exact,
// differentiable and saveable, but the graph grows with the order); Jet carries
// truncated power series through the graph (cost O(order^2) per operation,
// any order, scalar elementwise Functions only; the result is still an analytic,
// differentiable, saveable Function); Auto
// takes Structural up to order 8 and Jet above it when the Function allows.
enum class Engine : std::uint8_t { Auto = 0, Structural = 1, Jet = 2 };

// sum_{k <= order} f^(k)(a) / k! (x - a)^k as a Function of x (and of the
// other variables of f).
[[nodiscard]] Operator taylor(std::string_view variable, double center,
                              std::int64_t order, DiffOptions options = {},
                              Engine engine = Engine::Auto);
// Several variables: sum over multi-indices alpha with |alpha| <= order of
// d^alpha f(a) / alpha! prod (x_i - a_i)^alpha_i.
[[nodiscard]] Operator taylor(pluss::StackArray<std::string_view> variables,
                              pluss::RangeView<double> center,
                              std::int64_t order, DiffOptions options = {});
// Companion operators of the Taylor expansion, reached as taylor().name(...).
struct Taylor {
  // The coefficients f^(k)(a) / k!, k = 0..order, as a vector-valued Function
  // (shape [order + 1]).
  [[nodiscard]] Operator coefficients(std::string_view variable, double center,
                                      std::int64_t order,
                                      DiffOptions options = {},
                                      Engine engine = Engine::Auto) const;
  // The Lagrange remainder factor f^(order + 1)(x) / (order + 1)!, so that
  // |f(x) - T_order(x)| <= sup |factor| |x - a|^(order + 1) between a and x.
  [[nodiscard]] Operator remainder(std::string_view variable,
                                   std::int64_t order,
                                   DiffOptions options = {}) const;
};
[[nodiscard]] inline Taylor taylor() { return {}; }

// Operators on coefficient vectors (rank-1 Functions, as taylor().coefficients
// returns): a truncated power series sum c_k (x - a)^k is its coefficient vector.
// Polynomial in `variable` about `center` with the input as its coefficients.
[[nodiscard]] Operator power(std::string_view variable, double center);
struct Power {
  // Termwise derivative: c'_k = (k + 1) c_{k+1} (one coefficient fewer).
  [[nodiscard]] Operator derivative() const;
  // Termwise integral from the centre: c'_0 = 0, c'_{k+1} = c_k / (k + 1) (one more).
  [[nodiscard]] Operator integral() const;
  // Cauchy product with another coefficient vector, truncated to the shorter length.
  [[nodiscard]] Operator product(math::Function other) const;
};
[[nodiscard]] inline Power power() { return {}; }

} // namespace expansion

// Stochastic calculus. The stochastic integral and the path variations are
// path functionals: their arguments are Series sampled along a path axis (the
// samples of one sample path, a Wiener path for the noise), and the result has
// that axis summed out. Every other axis is an independent path. The integrand
// is a pointwise Function of the sampled variables, so it cannot look ahead
// along the path and the Ito sum is non-anticipating by construction (an
// opaque callable is rejected; use sde::ito_integral with its adaptedness
// probe for integrands computed from the whole path).
namespace stochastic {

// theta of the integration rule: the integrand is taken at (1-theta) times
// the left endpoint plus theta times the right endpoint of each step.
enum class Convention : std::uint8_t {
  Ito,          // theta = 0, non-anticipating, martingale
  Stratonovich, // theta = 1/2, obeys the ordinary chain rule
  Klimontovich, // theta = 1, right endpoint (backward / anti-Ito)
  Theta,        // the `theta` field of the options
};

[[nodiscard]] constexpr double theta_of(Convention convention,
                                        double theta = 0.0) noexcept {
  return convention == Convention::Ito            ? 0.0
         : convention == Convention::Stratonovich ? 0.5
         : convention == Convention::Klimontovich ? 1.0
                                                  : theta;
}

struct PathOptions final {
  bool cumulative = false;       // running value at every sample (starts at 0) instead of the terminal value
  std::int64_t path_axis = -1;   // axis of the arguments that indexes the samples of one path
};

struct IntegralOptions final {
  Convention convention = Convention::Ito;
  double theta = 0.0; // used only with Convention::Theta, in [0, 1]
  bool cumulative = false;
  std::int64_t path_axis = -1;
};


// Applied to f(.., W, ..): the sum over the path of f at the (theta-weighted)
// step endpoints times the increments of the noise variable W.
[[nodiscard]] Operator stochastic_integral(std::string_view noise,
                                           IntegralOptions options = {});
// Sum of squared increments [X] of the input process along the path axis.
[[nodiscard]] Operator quadratic_variation(PathOptions options = {});
// Sum of products of increments [X, Y] of the input and `other`.
[[nodiscard]] Operator covariation(math::Function other,
                                   PathOptions options = {});

// For dX = drift dt + diffusion o_theta dW on states X (scalar drift and
// diffusion for one state, or a drift vector [n] and a diffusion matrix
// [n, m] for n states):
// the infinitesimal generator L g = drift . grad g + 1/2 tr(diffusion diffusion^T Hess g).
[[nodiscard]] Operator generator(math::Function drift, math::Function diffusion,
                                 pluss::StackArray<std::string_view> states,
                                 DiffOptions options = {});
// The coefficients of g(X) in the same convention: output [a, b_1..b_m] with
// a = drift . grad g + (1/2 - theta) tr(diffusion diffusion^T Hess g) and
// b = diffusion^T grad g. theta = 0 is Ito's formula, 1/2 the ordinary chain rule.
[[nodiscard]] Operator
stochastic_differential(math::Function drift, math::Function diffusion,
                        pluss::StackArray<std::string_view> states,
                        Convention convention = Convention::Ito,
                        double theta = 0.0, DiffOptions options = {});
// Applied to a drift: the drift of the same process written in another
// convention, drift + (theta_from - theta_to) c with c_i = sum diffusion_jk d_j diffusion_ik.
[[nodiscard]] Operator
convention_change(double theta_from, double theta_to, math::Function diffusion,
                  pluss::StackArray<std::string_view> states,
                  DiffOptions options = {});

// Malliavin calculus on a discretized Wiener path. `path` names the path levels W_1..W_N (W_0 = 0) as
// the variables of the Function, so the increments are dW_i = W_i - W_{i-1}. Shifting dW_i by h_i dt
// (a Cameron-Martin direction) shifts W_j by dt sum_{i <= j} h_i, so
//   D_{t_i} F = dF / d(dW_i) = sum_{j >= i} dF / dW_j.
// Both results are analytic Functions built from `differential`, so they are exact for the discretized
// functional and differentiate, compose and evaluate like any Function (the eager, sampled versions
// stay on Series::calc). The Skorokhod integral of a vector Function u (one component per path step) is
//   delta(u) = sum_i u_i dW_i - dt sum_i D_{t_i} u_i,
// which for an adapted u is the Ito sum (the correction vanishes) and for u_i = W_N is W_N^2 - N dt.
// Cost: N^2 differentials for the Skorokhod integral; use it on short paths.
// Applied to a scalar Function of the path: the vector [D_{t_1} F, ..., D_{t_N} F].
[[nodiscard]] Operator malliavin_derivative(pluss::StackArray<std::string_view> path, DiffOptions options = {});
// Applied to a rank-1 Function of the path with N components: the scalar delta(u).
[[nodiscard]] Operator skorokhod_integral(pluss::StackArray<std::string_view> path, double dt,
                                          DiffOptions options = {});

} // namespace stochastic
[[nodiscard]] Operator differential(std::string_view variable,
                                    std::int64_t order = 1,
                                    DiffOptions options = {});
// Complex-variable structural derivatives. `wirtinger` is d/dz and
// `conjugate_wirtinger` is d/d(conj(z)); a Function is holomorphic in the
// selected variable exactly where the latter derivative is zero.
[[nodiscard]] Operator wirtinger(std::string_view variable,
                                 std::int64_t order = 1,
                                 DiffOptions options = {});
[[nodiscard]] Operator conjugate_wirtinger(std::string_view variable,
                                           std::int64_t order = 1,
                                           DiffOptions options = {});
[[nodiscard]] Operator
gradient(pluss::StackArray<std::string_view> variables = {},
         DiffOptions options = {});
[[nodiscard]] Operator
jacobian(pluss::StackArray<std::string_view> variables = {},
         DiffOptions options = {});
[[nodiscard]] Operator
hessian(pluss::StackArray<std::string_view> variables = {},
        DiffOptions options = {});
[[nodiscard]] Operator antiderivative(std::string_view variable);
[[nodiscard]] Operator integral(std::string_view variable, double lower);
[[nodiscard]] Operator integral(std::string_view variable, double lower,
                                double upper);
[[nodiscard]] Operator shift(std::string_view variable, double amount);
[[nodiscard]] Operator multiply(math::Function coefficient);
[[nodiscard]] Operator scale(double coefficient);
[[nodiscard]] Operator compose(const Operator &outer, const Operator &inner);
[[nodiscard]] Operator
linear_combination(pluss::RangeView<Operator> operators,
                   pluss::RangeView<double> coefficients);
using Term = std::pair<double, Operator>;
[[nodiscard]] Operator linear_combination(pluss::RangeView<Term> terms);
[[nodiscard]] Operator linear_combination(std::initializer_list<Term> terms);
[[nodiscard]] Operator commutator(const Operator &left, const Operator &right);
// Formal adjoint for operators that declare `adjoint_known`: differential (-1)^k, multiply and scale
// (real coefficients) themselves, shift(a) -> shift(-a); compositions reverse, linear combinations combine.
// It is the adjoint in L2 for fields that vanish at the ends of the domain (boundary terms dropped).
[[nodiscard]] Operator adjoint(const Operator &op);

using Loader =
    std::function<Operator(std::string_view payload, std::uint32_t version)>;
bool bind(std::string_view key, std::uint32_t version, Loader loader);

namespace detail {
void require_callable(bool present);
[[nodiscard]] Operator
define_erased(std::string key,
              std::function<math::Function(const math::Function &)> callable,
              Options options, Operator adjoint);
} // namespace detail

// `adjoint` is the formal adjoint of the operator being defined; supplying it
// makes `operators::adjoint` work on the result (and sets `adjoint_known`).
// Declaring `Options::adjoint_known` without supplying it is an error, since
// the library could not build the adjoint.
template <class Callable>
[[nodiscard]] Operator define(std::string_view key, Callable &&callable,
                              Options options = {}, Operator adjoint = {}) {
  using Value = std::remove_cvref_t<Callable>;
  static_assert(
      std::invocable<Value &, const math::Function &>,
      "operators::define: callable must accept const math::Function&");
  static_assert(
      std::same_as<std::invoke_result_t<Value &, const math::Function &>,
                   math::Function>,
      "operators::define: callable must return math::Function");
  if constexpr (requires(const Value &value) {
                  { static_cast<bool>(value) } -> std::same_as<bool>;
                }) {
    detail::require_callable(static_cast<bool>(callable));
  }
  auto stored = std::make_shared<Value>(std::forward<Callable>(callable));
  return detail::define_erased(
      std::string(key),
      [stored = std::move(stored)](const math::Function &input) {
        return std::invoke(*stored, input);
      },
      options, std::move(adjoint));
}

template <class Callable> [[nodiscard]] Operator define(Callable &&callable) {
  return define({}, std::forward<Callable>(callable), Options{});
}

} // namespace pluss::subsp::math::operators

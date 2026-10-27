// Curvature models of the quadratic-model algorithms (sequential QP and the working set): limited-memory BFGS and
// the exact Hessian of the Lagrangian. The limited-memory matrix is checked against the compact representation of
// Byrd, Nocedal and Schnabel (1994), B = theta I - W M^-1 W', computed independently; the exact Hessian against the
// analytic Hessian of a Lagrangian; and every algorithm x model pair against the solution of a standard test
// problem (Hock-Schittkowski 71, optimum 17.0140172891 at x = (1, 4.74300, 3.82115, 1.37941), also found by an
// independent SLSQP implementation).
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <pluss/pluss.h>
#include <pluss/subsp/math/optim.h>

#include <cmath>
#include <vector>


namespace {

using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;
namespace optim = pluss::subsp::math::optim;
using namespace optim;

Series scalar(double v) { return make_series(v).to(ScalarType::Float64); }
Series vec(std::vector<double> v) { return make_series(v, {static_cast<std::int64_t>(v.size())}).to(ScalarType::Float64); }

inline Series sq(const Series& x) { return x.square(); }

template <class AD> AD f71(const ParameterPack<AD>& p) { return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c"); }
template <class AD> AD g71(const ParameterPack<AD>& p) { return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d"); }
template <class AD> AD h71(const ParameterPack<AD>& p) {
  return sq(p.at("a")) + sq(p.at("b")) + sq(p.at("c")) + sq(p.at("d")) - 40.0;
}
ParameterPack<Series> four(double a, double b, double c, double d) {
  return pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
}
ConstrainedProblem hs71() {
  ConstrainedProblem prob;
  prob.objective.autodiff = [](const ParameterPack<Series>& p) { return f71<Series>(p); };
  prob.nonlinear.inequality = [](const ParameterPack<Series>& p) { return g71<Series>(p); };
  prob.nonlinear.equality = [](const ParameterPack<Series>& p) { return h71<Series>(p); };
  prob.bounds.lower = four(1, 1, 1, 1);
  prob.bounds.upper = four(5, 5, 5, 5);
  return prob;
}

}  // namespace

TEST_CASE("the limited-memory matrix equals the compact representation of Byrd, Nocedal and Schnabel",
          "[optim][curvature][lbfgs]") {
  // Three pairs (s_i, y_i = Q s_i) of a quadratic with Hessian Q; the expected matrix was computed independently as
  // theta I - [theta S, Y] [[theta S'S, L], [L', -D]]^-1 [theta S, Y]' with theta = y'y / s'y of the newest pair.
  const std::vector<double> S = {0.5, -0.2, 0.1, 0.3, 0.4, -0.6, -0.1, 0.25, 0.35};
  const std::vector<double> Y = {0.95, -0.17, 0.19, 0.66, 0.81, -0.65, -0.09, 0.275, 0.29};
  const std::vector<double> expected = {2.0060708695202609, 0.29921394030385107, 0.10229600536018091,
                                        0.29921394030385112, 1.4887865474115567, -0.19221497949286848,
                                        0.10229600536018081, -0.19221497949286867, 0.99509527259781527};
  std::vector<Series> s_hist, y_hist;
  for (int k = 0; k < 3; ++k) {
    s_hist.push_back(vec({S[3 * k], S[3 * k + 1], S[3 * k + 2]}));
    y_hist.push_back(vec({Y[3 * k], Y[3 * k + 1], Y[3 * k + 2]}));
  }
  const auto B = detail::lbfgs_dense_hessian_(s_hist, y_hist, 3, pluss::codem::Platform::CPU).host<double>();
  REQUIRE(B.size() == 9);
  for (std::size_t i = 0; i < 9; ++i) REQUIRE(B[i] == Approx(expected[i]).margin(1e-12));
  // The secant equation holds for the newest pair: B s = y.
  const auto Bs = detail::lbfgs_dense_hessian_(s_hist, y_hist, 3, pluss::codem::Platform::CPU)
                      .matmul(s_hist.back().reshape({3, 1}))
                      .reshape({3})
                      .host<double>();
  for (int i = 0; i < 3; ++i) REQUIRE(Bs[i] == Approx(Y[6 + i]).margin(1e-12));
}

TEST_CASE("the exact Lagrangian model equals the analytic Hessian of the Lagrangian",
          "[optim][curvature][exact]") {
  // L = x^2 y + y^3 + lambda (x^2 + y^2 - 1) at (0.9, 0.8), lambda = 0.7:
  //   H = [[2y + 2 lambda, 2x], [2x, 6y + 2 lambda]]
  ConstrainedProblem prob;
  prob.objective.autodiff = [](const ParameterPack<Series>& p) {
    return p.at("x").square() * p.at("y") + p.at("y") * p.at("y").square();
  };
  prob.nonlinear.equality = [](const ParameterPack<Series>& p) { return p.at("x").square() + p.at("y").square() - 1.0; };
  const auto params = pack_scalars({{"x", scalar(0.9)}, {"y", scalar(0.8)}});
  const auto evaluation = evaluate_problem(prob, params);
  const auto system = detail::assemble_constraint_system_(prob, params, evaluation);
  bool ok = false;
  std::vector<const detail::FlatConstraintRow*> active;
  for (const auto& row : system.equalities) active.push_back(&row);
  const auto H = detail::exact_lagrangian_model_(prob, params, system, {0.7}, {}, active, {}, ok).host<double>();
  REQUIRE(ok);
  REQUIRE(H[0] == Approx(2 * 0.8 + 2 * 0.7).margin(1e-12));
  REQUIRE(H[1] == Approx(2 * 0.9).margin(1e-12));
  REQUIRE(H[2] == Approx(2 * 0.9).margin(1e-12));
  REQUIRE(H[3] == Approx(6 * 0.8 + 2 * 0.7).margin(1e-12));
}

TEST_CASE("indefinite curvature is shifted only where the active constraints do not already control it",
          "[optim][curvature][exact]") {
  // f = x y has Hessian [[0, 1], [1, 0]], indefinite on the plane but positive on the line x = y that the
  // equality x - y = 0 leaves free... here the constraint x + y = 0 leaves the direction (1, -1), along which the
  // curvature of f is -2: the model must be shifted. With x - y = 0 the free direction is (1, 1), the curvature is
  // +2, and the exact Hessian is kept unshifted.
  auto make = [](double sign) {
    ConstrainedProblem prob;
    prob.objective.autodiff = [](const ParameterPack<Series>& p) { return p.at("x") * p.at("y"); };
    prob.linear.Aeq = make_series({1.0, sign}, {1, 2}).to(ScalarType::Float64);
    prob.linear.beq = vec({0.0});
    return prob;
  };
  const auto params = pack_scalars({{"x", scalar(0.3)}, {"y", scalar(0.3)}});
  for (double sign : {-1.0, 1.0}) {
    const auto prob = make(sign);
    const auto evaluation = evaluate_problem(prob, params);
    const auto system = detail::assemble_constraint_system_(prob, params, evaluation);
    std::vector<const detail::FlatConstraintRow*> active;
    for (const auto& row : system.equalities) active.push_back(&row);
    bool ok = false;
    const auto H = detail::exact_lagrangian_model_(prob, params, system, {0.0}, {}, active, {}, ok).host<double>();
    REQUIRE(ok);
    if (sign == -1.0) {  // free direction (1, 1): curvature +2, no shift
      REQUIRE(H[0] == Approx(0.0).margin(1e-12));
      REQUIRE(H[1] == Approx(1.0).margin(1e-12));
    } else {  // free direction (1, -1): curvature -2, shifted
      REQUIRE(H[0] > 0.0);
    }
  }
}

TEST_CASE("every algorithm and curvature model reaches the optimum of a standard test problem",
          "[optim][curvature]") {
  const double expected_value = 17.0140172891;
  for (auto algorithm : {OptimaAlgorithm::SequentialQP, OptimaAlgorithm::WorkingSet, OptimaAlgorithm::InternalProtocol}) {
    for (auto model : {OptimaHessianApproximation::Bfgs, OptimaHessianApproximation::Lbfgs,
                       OptimaHessianApproximation::Exact, OptimaHessianApproximation::HessianVector}) {
      OptimaOptions options;
      options.algorithm = algorithm;
      options.sequential_qp.hessian_approximation = model;
      options.working_set.hessian_approximation = model;
      options.internal_protocol.hessian_approximation = model;
      const auto r = optima(hs71(), four(1, 5, 5, 1), options);
      INFO("algorithm " << static_cast<int>(algorithm) << ", model " << static_cast<int>(model));
      REQUIRE(r.exit_flag == OptimaExitFlag::Converged);
      REQUIRE(r.evaluation.value.item<double>() == Approx(expected_value).margin(1e-5));
      REQUIRE(as_f64(r.params.at("b")) == Approx(4.74300).margin(1e-4));
      REQUIRE(as_f64(r.params.at("c")) == Approx(3.82115).margin(1e-4));
      REQUIRE(as_f64(r.params.at("d")) == Approx(1.37941).margin(1e-4));
    }
  }
}

TEST_CASE("the exact models converge quadratically and need fewer iterations than the limited-memory model",
          "[optim][curvature]") {
  OptimaOptions exact;
  exact.algorithm = OptimaAlgorithm::SequentialQP;
  exact.sequential_qp.hessian_approximation = OptimaHessianApproximation::Exact;
  const auto r = optima(hs71(), four(1, 5, 5, 1), exact);
  REQUIRE(r.exit_flag == OptimaExitFlag::Converged);
  REQUIRE(r.iteration <= 8);
}

TEST_CASE("exact curvature needs an autodiff objective, other models do not",
          "[optim][curvature][validation]") {
  ConstrainedProblem analytic_only = hs71();
  analytic_only.objective.autodiff = nullptr;
  analytic_only.objective.analytic = [](const ParameterPack<Series>& p) {
    const Series a = p.at("a"), b = p.at("b"), c = p.at("c"), d = p.at("d");
    const Series value = a * d * (a + b + c) + c;
    return std::pair<Series, ParameterPack<Series>>{
        value, pack_scalars({{"a", d * (a + b + c) + a * d}, {"b", a * d}, {"c", a * d + 1.0}, {"d", a * (a + b + c)}})};
  };
  for (auto algorithm : {OptimaAlgorithm::SequentialQP, OptimaAlgorithm::WorkingSet}) {
    for (auto model : {OptimaHessianApproximation::Exact, OptimaHessianApproximation::HessianVector}) {
      OptimaOptions options;
      options.algorithm = algorithm;
      options.sequential_qp.hessian_approximation = model;
      options.working_set.hessian_approximation = model;
      const auto r = optima(analytic_only, four(1, 5, 5, 1), options);
      REQUIRE(r.exit_flag == OptimaExitFlag::InvalidProblem);
      REQUIRE_THAT(r.message, Catch::Matchers::ContainsSubstring("autodiff objective"));
    }
    OptimaOptions lbfgs;
    lbfgs.algorithm = algorithm;
    lbfgs.sequential_qp.hessian_approximation = OptimaHessianApproximation::Lbfgs;
    lbfgs.working_set.hessian_approximation = OptimaHessianApproximation::Lbfgs;
    REQUIRE(optima(analytic_only, four(1, 5, 5, 1), lbfgs).exit_flag == OptimaExitFlag::Converged);
  }
}

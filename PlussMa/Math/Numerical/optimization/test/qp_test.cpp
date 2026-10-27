#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/qp.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace qp = pluss::subsp::math::qp;

namespace {
void expect(bool value, const std::string &message) {
  if (!value)
    throw std::runtime_error(message);
}

void close(const pluss::Series &actual, const std::vector<double> &expected,
           double tolerance = 2e-4) {
  const auto values = actual.to(pluss::ScalarType::Float64).host<double>();
  expect(values.size() == expected.size(), "QP result size mismatch");
  for (std::size_t i = 0; i < values.size(); ++i)
    expect(std::abs(values[i] - expected[i]) <=
               tolerance *
                   std::max({1.0, std::abs(values[i]), std::abs(expected[i])}),
           "QP value mismatch at lane " + std::to_string(i));
}

template <class Fn> void expect_error(Fn &&fn, const std::string &message) {
  bool failed = false;
  try {
    fn();
  } catch (const pluss::Error &) {
    failed = true;
  }
  expect(failed, message);
}
} // namespace

int main() {
  const auto p = pluss::series::make({{4.0, 1.0}, {1.0, 2.0}});
  const auto a = pluss::series::eye(2, pluss::ScalarType::Float64);
  const auto inf = std::numeric_limits<double>::infinity();
  auto free = qp::solve(p, pluss::series::make({1.0, 1.0}), a,
                        pluss::series::make({-inf, -inf}),
                        pluss::series::make({inf, inf}));
  expect(free.converged, "unconstrained QP did not converge");
  close(free.x, {-1.0 / 7.0, -3.0 / 7.0});

  auto boxed = qp::solve(p, pluss::series::make({1.0, 1.0}), a,
                         pluss::series::make({0.0, 0.0}),
                         pluss::series::make({inf, inf}));
  expect(boxed.converged, "box QP did not converge");
  close(boxed.x, {0.0, 0.0});

  auto equality =
      qp::solve(pluss::series::eye(2, pluss::ScalarType::Float64),
                pluss::series::zeros({2}, pluss::ScalarType::Float64),
                pluss::series::make({{1.0, 1.0}}), pluss::series::make({1.0}),
                pluss::series::make({1.0}));
  expect(equality.converged, "equality QP did not converge");
  close(equality.x, {0.5, 0.5});

  const auto batch_q = pluss::series::make({{1.0, 1.0}, {-2.0, -4.0}});
  auto batch = qp::solve(pluss::series::eye(2, pluss::ScalarType::Float64),
                         batch_q, a, pluss::series::make({-inf, -inf}),
                         pluss::series::make({inf, inf}));
  expect(batch.converged, "batched QP did not converge");
  expect(batch.x.sizes() == std::vector<std::int64_t>{2, 2},
         "batched QP shape is wrong");
  close(batch.x, {-1.0, -1.0, 2.0, 4.0});

  auto warm = qp::solve(p, pluss::series::make({1.0, 1.0}), a,
                        pluss::series::make({-inf, -inf}),
                        pluss::series::make({inf, inf}), {}, free.warm());
  expect(warm.converged, "warm-started QP did not converge");
  close(warm.x, {-1.0 / 7.0, -3.0 / 7.0});

  const auto prepared = qp::prepare(p, a);
  auto reused = prepared.solve(pluss::series::make({-2.0, -1.0}),
                               pluss::series::make({-inf, -inf}),
                               pluss::series::make({inf, inf}));
  expect(reused.converged, "prepared QP did not converge");
  close(reused.x, {3.0 / 7.0, 2.0 / 7.0});

  auto tracked_q = pluss::series::make({1.0, -2.0});
  tracked_q.tracks_slope_(true);
  auto differentiable = qp::solve(
      pluss::series::eye(2, pluss::ScalarType::Float64), tracked_q, a,
      pluss::series::make({-inf, -inf}), pluss::series::make({inf, inf}));
  differentiable.x.sum().reverse();
  close(tracked_q.sensitivity(), {-1.0, -1.0}, 8e-4);

  expect_error(
      [&] {
        (void)qp::prepare(pluss::series::make({{1.0, 2.0}, {2.0, -1.0}}), a);
      },
      "indefinite P was accepted");
  expect_error(
      [&] {
        (void)prepared.solve(pluss::series::make({1.0, 1.0}),
                             pluss::series::make({1.0, 0.0}),
                             pluss::series::make({0.0, 1.0}));
      },
      "reversed bounds were accepted");

  {
    // A badly chosen starting penalty must not matter: residual balancing finds a good one.
    const auto bp = pluss::series::make({{4.0, 1.0}, {1.0, 2.0}});
    const auto ba = pluss::series::make({{1.0, 1.0}, {1.0, 0.0}, {0.0, 1.0}});
    const auto bq = pluss::series::make({1.0, 1.0});
    const auto blo = pluss::series::make({1.0, 0.0, 0.0});
    const auto bhi = pluss::series::make({1.0, 0.7, 0.7});
    qp::Options poor;
    poor.rho = 1e-3;
    poor.max_iters = 400;
    auto adaptive = qp::solve(bp, bq, ba, blo, bhi, poor);
    expect(adaptive.converged, "adaptive penalty rescues a poor starting rho");
    close(adaptive.x, {0.3, 0.7}, 1e-3);
    poor.adaptive_rho = false;
    auto fixed = qp::solve(bp, bq, ba, blo, bhi, poor);
    expect(!fixed.converged, "without adaptation the same poor rho does not converge in the same iterations");
  }

  std::cout << "Dense batched QP checks passed\n";
}

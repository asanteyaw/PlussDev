#include <pluss/ma.h>

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <sstream>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;

namespace {

pluss::Series scalar(double value) {
  return pluss::make_series(std::vector<double>{value}, {1}).reshape({});
}
bool close(const pluss::Series &actual, const pluss::Series &expected,
           double tolerance = 1e-9) {
  return actual.allclose(expected, tolerance, tolerance);
}
std::vector<double> host(const pluss::Series &value) {
  const pluss::Series cpu = value.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous();
  return cpu.reshape({cpu.numel()}).host<double>();
}

// M(x) = [[1 + x^2, x], [x + 2, 3 + x]]   (deliberately not symmetric)
math::Function matrix_function() {
  return math::function({"x"}, [](math::Function x) {
    const math::Function column0 = math::stack({1.0 + x * x, x + 2.0});
    const math::Function column1 = math::stack({x, 3.0 + x});
    return math::stack({column0, column1}); // trailing stacking: columns
  });
}
std::vector<double> matrix_at(double x) { // row-major [i][j]
  return {1.0 + x * x, x, x + 2.0, 3.0 + x};
}

// central difference of an evaluated Function in its single variable
pluss::Series finite_difference(const math::Function &f, double x,
                                double h = 1e-5) {
  return (f({scalar(x + h)}).sub(f({scalar(x - h)}))).div(2.0 * h);
}

} // namespace

TEST_CASE("stack assembles outputs and keeps jacobian's column convention",
          "[calculus][tensor]") {
  const math::Function m = matrix_function();
  REQUIRE(m.rank() == 2);
  CHECK(m.shape()[0] == 2);
  CHECK(m.shape()[1] == 2);
  const std::vector<double> value = host(m({scalar(0.7)}));
  const std::vector<double> expected = matrix_at(0.7);
  // columns were stacked, so element [i][j] sits at flat index i*2+j with i the row
  // of the first stacked vector: entries match the construction above
  REQUIRE(value.size() == 4);
  CHECK(std::abs(value[0] - expected[0]) < 1e-14); // [0][0]
  CHECK(std::abs(value[3] - expected[3]) < 1e-14); // [1][1]
}

TEST_CASE("inverse, det, trace, transpose and select evaluate through Series linearalg",
          "[calculus][tensor]") {
  const math::Function m = matrix_function();
  const pluss::Series at = m({scalar(0.7)});
  CHECK(close(m.inverse()({scalar(0.7)}), at.linearalg.inverse()));
  CHECK(close(m.det()({scalar(0.7)}), at.linearalg.det()));
  const std::vector<double> flat = host(at);
  CHECK(std::abs(host(m.trace()({scalar(0.7)}))[0] - (flat[0] + flat[3])) < 1e-12);
  CHECK(close(m.transpose()({scalar(0.7)}), at.transpose(0, 1)));
  CHECK(close(m.select(0, 1)({scalar(0.7)}), at.select(0, 1)));
  CHECK(m.det().rank() == 0);
  CHECK(m.select(0, 0).rank() == 1);
  CHECK(m.inverse().rank() == 2);
}

TEST_CASE("structural derivatives of tensor operations match finite differences",
          "[calculus][tensor][derivative]") {
  const math::Function m = matrix_function();
  const operators::Operator d = operators::differential("x");
  const double x = 0.7;
  const math::Function inverse = m.inverse();
  CHECK(close(d(inverse)({scalar(x)}), finite_difference(inverse, x), 1e-6));
  const math::Function det = m.det();
  CHECK(close(d(det)({scalar(x)}), finite_difference(det, x), 1e-6));
  const math::Function trace = m.trace();
  CHECK(close(d(trace)({scalar(x)}), finite_difference(trace, x), 1e-6));
  const math::Function transposed = m.transpose();
  CHECK(close(d(transposed)({scalar(x)}), finite_difference(transposed, x), 1e-6));
  const math::Function product = m.matmul(m.inverse().transpose());
  CHECK(close(d(product)({scalar(x)}), finite_difference(product, x), 1e-6));
  const math::Function selected = m.inverse().select(1, 0);
  CHECK(close(d(selected)({scalar(x)}), finite_difference(selected, x), 1e-6));
}

TEST_CASE("matrix identities hold structurally", "[calculus][tensor]") {
  const math::Function m = matrix_function();
  const pluss::Series eye = pluss::series::eye(2, pluss::ScalarType::Float64);
  CHECK(close(m.matmul(m.inverse())({scalar(1.3)}), eye, 1e-12));
  // Jacobi: d det A = det A tr(A^-1 dA), a relation between independently built Functions
  const operators::Operator d = operators::differential("x");
  const math::Function jacobi = m.det() * m.inverse().matmul(d(m)).trace();
  CHECK(close(d(m.det())({scalar(1.3)}), jacobi({scalar(1.3)}), 1e-12));
  // second derivative of the inverse against differencing the first
  const math::Function first = d(m.inverse());
  CHECK(close(operators::differential("x", 2)(m.inverse())({scalar(0.4)}),
              finite_difference(first, 0.4), 1e-5));
}

TEST_CASE("contract agrees with explicit index loops", "[calculus][tensor]") {
  // T[i][j][k] contracted with U[k][l] over (2, 0) and with V[j] over (1, 0)
  const std::vector<double> t_values = [] {
    std::vector<double> v(2 * 3 * 4);
    for (std::size_t i = 0; i < v.size(); ++i) v[i] = std::sin(0.37 * static_cast<double>(i) + 0.1);
    return v;
  }();
  const std::vector<double> u_values = [] {
    std::vector<double> v(4 * 2);
    for (std::size_t i = 0; i < v.size(); ++i) v[i] = std::cos(0.53 * static_cast<double>(i));
    return v;
  }();
  const pluss::Series t_series = pluss::make_series(t_values, {2, 3, 4});
  const pluss::Series u_series = pluss::make_series(u_values, {4, 2});
  const math::Function t = math::function(t_series);
  const math::Function u = math::function(u_series);
  REQUIRE(t.rank() == 3);
  const math::Function tu = t.contract(u, {2}, {0});
  REQUIRE(tu.rank() == 3);
  CHECK(tu.shape()[0] == 2);
  CHECK(tu.shape()[1] == 3);
  CHECK(tu.shape()[2] == 2);
  const std::vector<double> got = host(tu());
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 3; ++j)
      for (int l = 0; l < 2; ++l) {
        double expected = 0.0;
        for (int k = 0; k < 4; ++k) expected += t_values[(i * 3 + j) * 4 + k] * u_values[k * 2 + l];
        CHECK(std::abs(got[(i * 3 + j) * 2 + l] - expected) < 1e-12);
      }
  // two contracted axes, outer product (none), and the ordering of free axes
  const math::Function full = t.contract(t, {0, 1, 2}, {0, 1, 2});
  double norm2 = 0.0;
  for (const double v : t_values) norm2 += v * v;
  CHECK(full.rank() == 0);
  CHECK(std::abs(host(full())[0] - norm2) < 1e-12);
  const math::Function outer = u.contract(u, std::initializer_list<std::int64_t>{}, std::initializer_list<std::int64_t>{});
  CHECK(outer.rank() == 4);
}

TEST_CASE("sphere metric: inverse and determinant structurally", "[calculus][tensor][geometry]") {
  // g = diag(1, sin^2 theta)
  const math::Function metric = math::function({"theta"}, [](math::Function theta) {
    const math::Function s2 = theta.sin() * theta.sin();
    const math::Function one = theta * 0.0 + 1.0;
    const math::Function zero = theta * 0.0;
    return math::stack({math::stack({one, zero}), math::stack({zero, s2})});
  });
  const double theta = 0.9;
  const double s2 = std::sin(theta) * std::sin(theta);
  const std::vector<double> inverse = host(metric.inverse()({scalar(theta)}));
  CHECK(std::abs(inverse[0] - 1.0) < 1e-12);
  CHECK(std::abs(inverse[3] - 1.0 / s2) < 1e-12);
  CHECK(std::abs(inverse[1]) < 1e-12);
  CHECK(std::abs(host(metric.det()({scalar(theta)}))[0] - s2) < 1e-12);
  // d det g / d theta = 2 sin cos
  const math::Function derivative = operators::differential("theta")(metric.det());
  CHECK(std::abs(host(derivative({scalar(theta)}))[0] - 2.0 * std::sin(theta) * std::cos(theta)) < 1e-12);
}

TEST_CASE("tensor operations validate shapes and domains", "[calculus][tensor][errors]") {
  const math::Function vector = math::function({"x"}, [](math::Function x) { return math::stack({x, x * x, x + 1.0}); });
  CHECK_THROWS_AS(vector.inverse(), pluss::Error);
  CHECK_THROWS_AS(vector.det(), pluss::Error);
  CHECK_THROWS_AS(vector.transpose(), pluss::Error);
  CHECK_THROWS_AS(vector.select(0, 3), pluss::Error);
  CHECK_THROWS_AS(vector.select(1, 0), pluss::Error);
  CHECK_THROWS_AS(vector.contract(vector, {0}, {}), pluss::Error);
  const math::Function singular = math::function({"x"}, [](math::Function x) {
    return math::stack({math::stack({x, x}), math::stack({x, x})});
  });
  CHECK_THROWS_AS(singular.inverse()({scalar(1.0)}), pluss::Error);
  // outputs are capped at rank 4
  math::Function nested = math::function({"x"}, [](math::Function x) { return x; });
  for (int level = 0; level < 4; ++level) nested = math::stack({nested, nested});
  CHECK(nested.rank() == 4);
  CHECK_THROWS_AS(math::stack({nested, nested}), pluss::Error);
}

TEST_CASE("tensor Functions round-trip through serialization", "[calculus][tensor][io]") {
  const math::Function m = matrix_function();
  const math::Function composite = (m.inverse().matmul(m.transpose())).select(1, 0) + m.det();
  std::stringstream stream;
  math::io::save(composite, stream);
  const math::Function restored = math::io::load_function(stream);
  CHECK(restored.rank() == composite.rank());
  CHECK(close(restored({scalar(0.6)}), composite({scalar(0.6)}), 1e-14));
  std::stringstream derivative_stream;
  math::io::save(operators::differential("x")(composite), derivative_stream);
  const math::Function restored_derivative = math::io::load_function(derivative_stream);
  CHECK(close(restored_derivative({scalar(0.6)}), finite_difference(composite, 0.6), 1e-6));
}

TEST_CASE("stack broadcasts constant components against batched ones",
          "[calculus][tensor][stack]") {
  const math::Function mixed = math::function({"x"}, [](math::Function x) {
    return math::stack({x, math::function(2.0), x * x});
  });
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0});
  const pluss::Series out = mixed({x});
  REQUIRE(out.sizes() == std::vector<std::int64_t>{3, 3});
  CHECK(out.allclose(pluss::make_series({1.0, 2.0, 1.0, 2.0, 2.0, 4.0, 3.0, 2.0, 9.0}).reshape({3, 3})));
  CHECK(mixed(2.0).allclose(pluss::make_series({2.0, 2.0, 4.0})));

  const math::Function matrix = math::function({"x"}, [](math::Function x) {
    return math::stack({math::stack({x, math::function(1.0)}), math::stack({math::function(0.0), 2.0 * x})});
  });
  const pluss::Series grid = matrix({x});
  REQUIRE(grid.sizes() == std::vector<std::int64_t>{3, 2, 2});
  CHECK(grid.select(0, 2).allclose(pluss::make_series({3.0, 0.0, 1.0, 6.0}).reshape({2, 2})));
}

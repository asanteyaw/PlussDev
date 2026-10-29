#include <pluss/ma.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <complex>
#include <sstream>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;

namespace {

using Complex = std::complex<double>;

pluss::Series complex_series(std::initializer_list<Complex> values) {
  return pluss::make_series(std::vector<Complex>(values),
                            {static_cast<std::int64_t>(values.size())});
}

math::Function complex_constant(Complex value) {
  return math::function(value);
}

void check_complex(const pluss::Series &actual,
                   const std::vector<Complex> &expected,
                   double tolerance = 1e-10) {
  const std::vector<Complex> host =
      actual.to(pluss::pCPU).to(pluss::ScalarType::Complex128)
          .reshape({actual.numel()})
          .host<Complex>();
  REQUIRE(host.size() == expected.size());
  for (std::size_t i = 0; i < host.size(); ++i) {
    CAPTURE(i, host[i], expected[i]);
    CHECK(std::abs(host[i] - expected[i]) <= tolerance);
  }
}

} // namespace

TEST_CASE("complex Function primitives evaluate structurally",
          "[calculus][complex][function]") {
  const math::Function f = math::function({"z"}, [](math::Function z) {
    return math::stack({z.conj(), z.real(), z.imag(), z.abs(), z.angle()});
  });
  const Complex z{3.0, 4.0};
  const pluss::Series value = f({complex_series({z})});
  check_complex(value,
                {{3.0, -4.0}, {3.0, 0.0}, {4.0, 0.0}, {5.0, 0.0},
                 {std::atan2(4.0, 3.0), 0.0}});
}

TEST_CASE("holomorphic graphs have zero conjugate Wirtinger derivative",
          "[calculus][complex][wirtinger][holomorphic]") {
  const math::Function f = math::function({"z"}, [](math::Function z) {
    return z.pow(3.0) + z.exp() + z.sin();
  });
  const math::Function dz = operators::wirtinger("z")(f);
  const math::Function dbar = operators::conjugate_wirtinger("z")(f);
  const std::vector<Complex> points{{0.3, -0.4}, {1.2, 0.5}};
  const pluss::Series z =
      pluss::make_series(points, {static_cast<std::int64_t>(points.size())});
  std::vector<Complex> expected;
  for (const Complex point : points) {
    expected.push_back(3.0 * point * point + std::exp(point) + std::cos(point));
  }

  check_complex(dz({z}), expected);
  check_complex(dbar({z}), std::vector<Complex>(points.size()));
}

TEST_CASE("Wirtinger rules distinguish conjugate real imaginary magnitude and phase",
          "[calculus][complex][wirtinger][nonholomorphic]") {
  const math::Function z =
      math::function({"z"}, [](math::Function value) { return value; });
  const pluss::Series point = complex_series({{3.0, 4.0}});

  check_complex(operators::wirtinger("z")(z.conj())({point}), {{0.0, 0.0}});
  check_complex(operators::conjugate_wirtinger("z")(z.conj())({point}),
                {{1.0, 0.0}});
  check_complex(operators::wirtinger("z")(z.real())({point}), {{0.5, 0.0}});
  check_complex(operators::conjugate_wirtinger("z")(z.real())({point}),
                {{0.5, 0.0}});
  check_complex(operators::wirtinger("z")(z.imag())({point}), {{0.0, -0.5}});
  check_complex(operators::conjugate_wirtinger("z")(z.imag())({point}),
                {{0.0, 0.5}});
  check_complex(operators::wirtinger("z")(z.abs())({point}), {{0.3, -0.4}});
  check_complex(operators::conjugate_wirtinger("z")(z.abs())({point}),
                {{0.3, 0.4}});
  check_complex(operators::wirtinger("z")(z.angle())({point}),
                {{-0.08, -0.06}});
  check_complex(operators::conjugate_wirtinger("z")(z.angle())({point}),
                {{-0.08, 0.06}});
}

TEST_CASE("real-parameter differentiation supports characteristic functions",
          "[calculus][complex][characteristic-function]") {
  constexpr double mean = 1.5;
  constexpr double variance = 0.7;
  const math::Function i = complex_constant({0.0, 1.0});
  const math::Function phi = math::function({"t"}, [i](math::Function t) {
    return (i * mean * t - 0.5 * variance * t.pow(2.0)).exp();
  });
  const math::Function first = operators::differential("t")(phi);
  const math::Function second = operators::differential("t", 2)(phi);
  const pluss::Series zero = pluss::make_series(0.0);

  check_complex(first({zero}), {{0.0, mean}});
  check_complex(second({zero}), {{-(variance + mean * mean), 0.0}});
}

TEST_CASE("complex logarithm and square root use principal branches",
          "[calculus][complex][branch]") {
  const math::Function z =
      math::function({"z"}, [](math::Function value) { return value; });
  const pluss::Series minus_one = complex_series({{-1.0, 0.0}});

  check_complex(z.log()({minus_one}), {{0.0, std::acos(-1.0)}});
  check_complex(z.sqrt()({minus_one}), {{0.0, 1.0}});
}

TEST_CASE("structural complex integrands delegate to complex quadrature",
          "[calculus][complex][integral]") {
  const math::Function i = complex_constant({0.0, 1.0});
  const math::Function wave = math::function(
      {"x"}, [i](math::Function x) { return (i * x).exp(); });
  const math::Function area =
      operators::integral("x", 0.0, std::acos(-1.0))(wave);

  check_complex(area(), {{0.0, 2.0}}, 1e-7);
}

TEST_CASE("complex structural calculus preserves MPS Complex64 execution",
          "[calculus][complex][mps]") {
  if (!pluss::platform_available(pluss::pMPS)) {
    SKIP("MPS backend is unavailable");
  }
  const math::Function f = math::function({"z"}, [](math::Function z) {
    return z.conj() * z + z.exp();
  });
  const math::Function dz = operators::wirtinger("z")(f);
  const pluss::Series cpu =
      complex_series({{0.25, -0.5}, {1.0, 0.75}})
          .to(pluss::ScalarType::Complex64);
  const pluss::Series mps = cpu.to(pluss::pMPS);
  const pluss::Series result = dz({mps});

  CHECK(result.platform() == pluss::pMPS);
  CHECK(result.scalar_type() == pluss::ScalarType::Complex64);
  CHECK(result.to(pluss::pCPU).allclose(dz({cpu}), 2e-5, 2e-5));
}

TEST_CASE("the complete complex Function vocabulary has CPU and MPS parity",
          "[calculus][complex][mps][vocabulary]") {
  if (!pluss::platform_available(pluss::pMPS)) {
    SKIP("MPS backend is unavailable");
  }
  const math::Function f = math::function({"z"}, [](math::Function z) {
    return math::stack({z.pow(2.5), z.exp(), z.log(), z.log1p(), z.expm1(),
                        z.sqrt(), z.sin(), z.cos(), z.tan(), z.sinh(),
                        z.cosh(), z.tanh(), z.conj(), z.real(), z.imag(),
                        z.abs(), z.angle()});
  });
  const pluss::Series cpu =
      complex_series({{0.25, -0.5}, {1.0, 0.75}})
          .to(pluss::ScalarType::Complex64);
  const pluss::Series mps = cpu.to(pluss::pMPS);
  const pluss::Series expected = f({cpu});
  const pluss::Series actual = f({mps});

  CHECK(actual.platform() == pluss::pMPS);
  CHECK(actual.scalar_type() == pluss::ScalarType::Complex64);
  CHECK(actual.to(pluss::pCPU).allclose(expected, 4e-5, 4e-5));
}

TEST_CASE("complex Functions and Wirtinger operators persist",
          "[calculus][complex][persistence]") {
  const math::Function f = math::function({"z"}, [](math::Function z) {
    return z.conj() + z.real() * z.exp();
  });
  const operators::Operator derivative = operators::wirtinger("z");
  std::stringstream function_stream;
  std::stringstream operator_stream;
  math::io::save(f, function_stream);
  math::io::save(derivative, operator_stream);
  const math::Function restored = math::io::load_function(function_stream);
  const operators::Operator restored_derivative =
      math::io::load_operator(operator_stream);
  const pluss::Series z = complex_series({{0.5, 0.25}, {1.0, -0.5}});

  CHECK(restored({z}).allclose(f({z}), 1e-12, 1e-12));
  CHECK(restored_derivative(restored)({z}).allclose(
      derivative(f)({z}), 1e-12, 1e-12));
}

TEST_CASE("complex scalars evaluate without building a Series",
          "[calculus][complex][function]") {
  const math::Function f =
      math::function({"z"}, [](math::Function z) { return z * z + z.exp(); });
  const Complex z{0.5, -1.25};
  check_complex(f(z), {z * z + std::exp(z)});
  check_complex(f(std::complex<float>(0.5f, -1.25f)), {z * z + std::exp(z)}, 1e-5);
  const pluss::Series value = f(z);
  CHECK(value.dim() == 0);
}

TEST_CASE("Wirtinger derivatives of a scalar point are scalars",
          "[calculus][complex][wirtinger]") {
  const math::Function z =
      math::function({"z"}, [](math::Function v) { return v; });
  const Complex point{3.0, 4.0};
  for (const math::Function &f : {z.conj(), z.real(), z.imag(), z.abs(), z.angle()}) {
    CHECK(operators::wirtinger("z")(f)(point).dim() == 0);
    CHECK(operators::conjugate_wirtinger("z")(f)(point).dim() == 0);
  }
}

TEST_CASE("Functions combine with bare complex scalars",
          "[calculus][complex][function]") {
  const Complex c{0.5, -2.0};
  const math::Function f = math::function({"z"}, [c](math::Function z) {
    return (z * c + c) / c - c * z + (c - z) * Complex{0.0, 1.0};
  });
  const Complex z{1.5, 0.25};
  const Complex expected = (z * c + c) / c - c * z + (c - z) * Complex{0.0, 1.0};
  check_complex(f(z), {expected});
}

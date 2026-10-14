// Scalar operands follow libtorch's "wrapped number" semantics:
//  * a scalar constant is rank 0, so it never changes the rank of the other operand
//    (aten/src/ATen/ScalarOps.h: wrapped_scalar_tensor -> 0-dim tensor);
//  * a fractional scalar combined with an integer/bool tensor promotes to a floating result
//    instead of being truncated into the tensor's integer dtype.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>

#include <cmath>
#include <vector>

namespace {
using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;

Series scalar(double v, ScalarType dt = ScalarType::Float64) { return pluss::series::full({}, v, dt); }
} // namespace

TEST_CASE("scalar operands keep a 0-d result 0-d", "[series][scalar][rank]") {
  const Series s = scalar(3.0);
  REQUIRE(s.dim() == 0);
  REQUIRE(s.add(0.5).dim() == 0);
  REQUIRE(s.sub(0.5).dim() == 0);
  REQUIRE(s.mul(2.0).dim() == 0);
  REQUIRE(s.div(2.0).dim() == 0);
  REQUIRE(s.pow(2.0).dim() == 0);
  REQUIRE(s.gt(0.5).dim() == 0);
  REQUIRE(s.add(s).dim() == 0);
  REQUIRE(s.sub(s).dim() == 0); // used to be rank 1 through the alpha multiply
  REQUIRE(s.add(0.5).item<double>() == Approx(3.5));
  // ... and a higher-rank operand keeps its own rank
  const Series v = pluss::series::arange(0, 3, 1, ScalarType::Float64);
  REQUIRE(v.add(1.0).dim() == 1);
  REQUIRE(v.add(1.0).size(0) == 3);
}

TEST_CASE("fractional scalar promotes integer tensors instead of truncating", "[series][scalar][dtype]") {
  const Series i = pluss::series::arange(0, 4, 1, ScalarType::Int64); // 0 1 2 3
  const Series sum = i.add(0.5);
  REQUIRE(sum.scalar_type() == ScalarType::Float32);
  const auto sv = sum.to(ScalarType::Float64).to_vec<double>();
  REQUIRE(sv == std::vector<double>{0.5, 1.5, 2.5, 3.5});

  const Series prod = i.mul(1.5);
  REQUIRE(prod.scalar_type() == ScalarType::Float32);
  REQUIRE(prod.to(ScalarType::Float64).to_vec<double>() == std::vector<double>{0.0, 1.5, 3.0, 4.5});

  const Series diff = i.sub(0.25);
  REQUIRE(diff.to(ScalarType::Float64).to_vec<double>() == std::vector<double>{-0.25, 0.75, 1.75, 2.75});

  // comparisons see the real constant, not its truncation
  const auto gt = i.gt(0.5).to(ScalarType::Int64).to_vec<std::int64_t>();
  REQUIRE(gt == std::vector<std::int64_t>{0, 1, 1, 1});
  const auto lt = i.lt(2.5).to(ScalarType::Int64).to_vec<std::int64_t>();
  REQUIRE(lt == std::vector<std::int64_t>{1, 1, 1, 0});
  const auto eq = i.eq(0.5).to(ScalarType::Int64).to_vec<std::int64_t>();
  REQUIRE(eq == std::vector<std::int64_t>{0, 0, 0, 0}); // used to match 0 == trunc(0.5)

  // an integral value keeps the integer dtype (a C++ double cannot be told from a Python int)
  REQUIRE(i.add(2.0).scalar_type() == ScalarType::Int64);
  REQUIRE(i.add(2.0).to_vec<std::int64_t>() == std::vector<std::int64_t>{2, 3, 4, 5});
  // floating tensors are untouched
  REQUIRE(pluss::series::arange(0, 2, 1, ScalarType::Float32).add(0.5).scalar_type() == ScalarType::Float32);
  REQUIRE(pluss::series::arange(0, 2, 1, ScalarType::Float64).add(0.5).scalar_type() == ScalarType::Float64);
  // bool tensors with a fractional scalar become floating as well
  REQUIRE(i.gt(1).add(0.5).scalar_type() == ScalarType::Float32);
}

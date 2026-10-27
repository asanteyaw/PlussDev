#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <limits>
#include <vector>

using pluss::codem::Series;
using pluss::codem::ScalarType;



TEST_CASE("matmul 2D", "[matmul]") {
  auto a = Series::zeros({2,3}, ScalarType::Float64);
  auto b = Series::zeros({3,2}, ScalarType::Float64);

  // a = [[1,2,3],[4,5,6]]
  auto* ap = reinterpret_cast<double*>(a.raw_data());
  ap[0]=1; ap[1]=2; ap[2]=3;
  ap[3]=4; ap[4]=5; ap[5]=6;

  // b = [[7,8],[9,10],[11,12]]
  auto* bp = reinterpret_cast<double*>(b.raw_data());
  bp[0]=7; bp[1]=8;
  bp[2]=9; bp[3]=10;
  bp[4]=11; bp[5]=12;

  auto c = a.matmul(b);
  REQUIRE(c.sizes()[0] == 2);
  REQUIRE(c.sizes()[1] == 2);
  auto* cp = reinterpret_cast<const double*>(c.raw_data());
  // expected [[58,64],[139,154]]
  REQUIRE(cp[0] == 58.0);
  REQUIRE(cp[1] == 64.0);
  REQUIRE(cp[2] == 139.0);
  REQUIRE(cp[3] == 154.0);
}

TEST_CASE("vector matmul returns a scalar", "[matmul]") {
  const auto a = pluss::make_series({1.0, 2.0, 3.0});
  const auto b = pluss::make_series({4.0, 5.0, 6.0});

  const auto out = a.matmul(b);

  REQUIRE(out.dim() == 0);
  REQUIRE(out.item<double>() == 32.0);
}

TEST_CASE("linearalg named products enforce their rank contracts", "[matmul][linearalg]") {
  const auto matrix = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
  const auto vector = pluss::make_series({5.0, 6.0});

  const auto mm_out = matrix.linearalg.mm(matrix);
  const auto mv_out = matrix.linearalg.mv(vector);
  REQUIRE(mm_out.dim() == 2);
  REQUIRE(mm_out.sizes()[0] == 2);
  REQUIRE(mm_out.sizes()[1] == 2);
  REQUIRE(mv_out.dim() == 1);
  REQUIRE(mv_out.sizes()[0] == 2);
  REQUIRE(vector.linearalg.dot(vector).dim() == 0);
  REQUIRE(vector.linearalg.vdot(vector).dim() == 0);

  REQUIRE_THROWS(matrix.linearalg.mm(vector));
  REQUIRE_THROWS(vector.linearalg.mv(vector));
  REQUIRE_THROWS(matrix.linearalg.dot(matrix));
  REQUIRE_THROWS(matrix.linearalg.vdot(matrix));
}

TEST_CASE("fused BLAS names enforce matrix vector and batch ranks", "[matmul][fused-blas]") {
  const auto matrix = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
  const auto vector = pluss::make_series({5.0, 6.0});
  const auto batch = matrix.reshape({1, 2, 2});

  REQUIRE_THROWS(matrix.addmm(vector, vector));
  REQUIRE_THROWS(vector.addmv(vector, vector));
  REQUIRE_THROWS(matrix.addr(matrix, vector));
  REQUIRE_THROWS(matrix.bmm(matrix));
  REQUIRE_THROWS(batch.bmm(matrix.reshape({2, 1, 2})));
  REQUIRE_THROWS(batch.baddbmm(matrix, matrix));
  REQUIRE_THROWS(matrix.outer(vector));
}

TEST_CASE("fused BLAS scaling and receiver broadcasting are exact", "[matmul][fused-blas]") {
  const auto lhs = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
  const auto rhs = pluss::make_series({{5.0, 6.0}, {7.0, 8.0}});
  const auto row_bias = pluss::make_series({10.0, 20.0});

  REQUIRE(row_bias.addmm(lhs, rhs, 0.5, 2.0).host<double>() ==
          std::vector<double>{43.0, 54.0, 91.0, 110.0});

  const auto nan_base = pluss::make_series({std::numeric_limits<double>::quiet_NaN()});
  REQUIRE(nan_base.addmv(pluss::make_series({2.0}).reshape({1, 1}), pluss::make_series({3.0}), 0.0, 1.0)
              .host<double>().front() == 6.0);

  const auto wrong_receiver = pluss::make_series({1.0, 2.0, 3.0});
  REQUIRE_THROWS(wrong_receiver.addmm(lhs, rhs, 0.0, 1.0));

  const auto integer_receiver = pluss::make_series({1, 2}, pluss::ScalarType::Int64);
  REQUIRE_THROWS(integer_receiver.addmm(lhs, rhs));
}

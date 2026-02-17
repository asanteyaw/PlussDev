#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>


TEST_CASE("scalar operators", "[scalar]") {
  Series a = pluss::full({4}, 2.0, ScalarType::Float32);
  Series b = a + 3.0;
  auto* bp = reinterpret_cast<const float*>(b.raw_data());
  REQUIRE(bp[0] == 5.0f);

  a += 1.0;
  auto* ap = reinterpret_cast<const float*>(a.raw_data());
  REQUIRE(ap[0] == 3.0f);

  Series m = (a > 2.0);
  REQUIRE(m.scalar_type() == ScalarType::Bool);
}

TEST_CASE("nonzero/take/put", "[index]") {
  Series x = pluss::zeros({5}, ScalarType::Int64);
  auto* xp = reinterpret_cast<std::int64_t*>(x.raw_data());
  xp[1] = 7; xp[3] = 9;

  Series nz = x.ne(0.0).nonzero(); // indices
  REQUIRE(nz.dim() == 2);

  // take
  Series idx = pluss::zeros({2}, ScalarType::Int64);
  auto* ip = reinterpret_cast<std::int64_t*>(idx.raw_data());
  ip[0]=1; ip[1]=3;
  Series t = x.take(idx);
  auto* tp = reinterpret_cast<const std::int64_t*>(t.raw_data());
  REQUIRE(tp[0] == 7);
  REQUIRE(tp[1] == 9);

  // put_
  Series src = pluss::full({2}, 1.0, ScalarType::Int64);
  x.put_(idx, src, false);
  REQUIRE(xp[1] == 1);
  REQUIRE(xp[3] == 1);
}

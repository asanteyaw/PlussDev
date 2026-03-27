#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;



TEST_CASE("scalar operators", "[scalar]") {
  Series a = pluss::full({4}, 2.0, ScalarType::Float32);
  Series b = a.add(3.0);
  auto* bp = reinterpret_cast<const float*>(b.raw_data());
  REQUIRE(bp[0] == 5.0f);

  a.add_(1.0);
  auto* ap = reinterpret_cast<const float*>(a.raw_data());
  REQUIRE(ap[0] == 3.0f);

  Series m = a.gt(2.0);
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

  Series src = pluss::full({2}, 1.0, ScalarType::Int64);
  x = x.put(idx, src, false);
  auto* outp = reinterpret_cast<const std::int64_t*>(x.raw_data());
  REQUIRE(outp[1] == 1);
  REQUIRE(outp[3] == 1);
}

TEST_CASE("Series::pos bounds-checked scalar selection", "[index][pos]") {
  Series s = pluss::make_series({1.0, 2.0, 3.0});
  const pluss::codem::SeriesBase& b = s;

  Series picked = s.pos(1);
  REQUIRE(picked.dim() == 0);
  REQUIRE(picked.numel() == 1);
  REQUIRE(picked.item<double>() == 2.0);

  Series picked_from_base = b.pos(2);
  REQUIRE(picked_from_base.dim() == 0);
  REQUIRE(picked_from_base.item<double>() == 3.0);

  REQUIRE_THROWS(s.pos(-1));
  REQUIRE_THROWS(s.pos(3));

  Series m = pluss::ones({2, 2}, ScalarType::Float64);
  REQUIRE_THROWS(m.pos(0));
}

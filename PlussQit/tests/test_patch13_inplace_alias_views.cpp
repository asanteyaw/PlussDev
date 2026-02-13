#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>


TEST_CASE("in-place ops mutate shared storage across views", "[alias]") {
  auto a = Series::zeros({2,3}, ScalarType::Float32);
  auto v = a.transpose(0,1); // view sharing storage
  v.add_(Series::full({3,2}, 1.0, ScalarType::Float32));
  auto* ap = reinterpret_cast<const float*>(a.contiguous().raw_data());
  for (int i=0;i<6;++i) REQUIRE(ap[i] == 1.0f);
}

TEST_CASE("index_put_ writes into original storage", "[alias]") {
  auto a = Series::zeros({3,3}, ScalarType::Int64);
  auto r = a.view({9}); // view
  auto idx = Series::arange(0, 9, 2, ScalarType::Int64);
  auto val = Series::full({idx.numel()}, 5.0, ScalarType::Int64);
  r.index_put_({idx}, val, false);
  auto aa = a.contiguous();
  auto* p = reinterpret_cast<const std::int64_t*>(aa.raw_data());
  REQUIRE(p[0] == 5);
  REQUIRE(p[1] == 0);
  REQUIRE(p[2] == 5);
}

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;



TEST_CASE("index with per-dim broadcasted int64 indices", "[adv_index]") {
  // a shape [2,3]
  auto a = Series::zeros({2,3}, ScalarType::Float64);
  auto* ap = reinterpret_cast<double*>(a.raw_data());
  for (int i=0;i<6;++i) ap[i] = double(i+1); // [[1,2,3],[4,5,6]]

  // idx0 shape [2,1] selects rows [0,1]
  auto i0 = Series::zeros({2,1}, ScalarType::Int64);
  auto* i0p = reinterpret_cast<std::int64_t*>(i0.raw_data());
  i0p[0]=0; i0p[1]=1;

  // idx1 shape [1,2] selects cols [2,0]
  auto i1 = Series::zeros({1,2}, ScalarType::Int64);
  auto* i1p = reinterpret_cast<std::int64_t*>(i1.raw_data());
  i1p[0]=2; i1p[1]=0;

  auto out = a.index({i0, i1});
  REQUIRE(out.sizes()[0] == 2);
  REQUIRE(out.sizes()[1] == 2);
  auto* op = reinterpret_cast<const double*>(out.raw_data());
  // broadcast gives [[a[0,2], a[0,0]],[a[1,2],a[1,0]]] => [[3,1],[6,4]]
  REQUIRE(op[0] == 3.0);
  REQUIRE(op[1] == 1.0);
  REQUIRE(op[2] == 6.0);
  REQUIRE(op[3] == 4.0);
}

TEST_CASE("index_put overwrite and accumulate", "[adv_index]") {
  auto a = Series::zeros({2,3}, ScalarType::Float32);
  // indices broadcast to shape [2,2]
  auto i0 = Series::zeros({2,1}, ScalarType::Int64);
  auto i1 = Series::zeros({1,2}, ScalarType::Int64);
  auto* i0p = reinterpret_cast<std::int64_t*>(i0.raw_data());
  auto* i1p = reinterpret_cast<std::int64_t*>(i1.raw_data());
  i0p[0]=0; i0p[1]=1;
  i1p[0]=1; i1p[1]=1; // both map to col1

  auto vals = Series::zeros({2,2}, ScalarType::Float32);
  auto* vp = reinterpret_cast<float*>(vals.raw_data());
  vp[0]=1; vp[1]=2; vp[2]=3; vp[3]=4;

  auto ovr = pluss::index_put(a, std::vector<Series>{i0, i1}, vals, false);
  auto* op = reinterpret_cast<const float*>(ovr.raw_data());
  // overwrite: last write wins at each (row,col1): row0 uses vals[0,1]=2, row1 uses vals[1,1]=4
  REQUIRE(op[1] == 2.0f);
  REQUIRE(op[4] == 4.0f);

  auto acc = pluss::index_put(a, std::vector<Series>{i0, i1}, vals, true);
  auto* ap = reinterpret_cast<const float*>(acc.raw_data());
  // accumulate: sums into same location
  REQUIRE(ap[1] == 3.0f); // 1+2
  REQUIRE(ap[4] == 7.0f); // 3+4
}

TEST_CASE("put accumulate works", "[flat]") {
  auto a = Series::zeros({5}, ScalarType::Int64);
  auto idx = Series::zeros({3}, ScalarType::Int64);
  auto src = Series::zeros({3}, ScalarType::Int64);
  auto* ip = reinterpret_cast<std::int64_t*>(idx.raw_data());
  auto* sp = reinterpret_cast<std::int64_t*>(src.raw_data());
  ip[0]=2; ip[1]=2; ip[2]=2;
  sp[0]=1; sp[1]=2; sp[2]=3;

  auto out = a.put(idx, src, true);
  auto* op = reinterpret_cast<const std::int64_t*>(out.raw_data());
  REQUIRE(op[2] == 6);
}

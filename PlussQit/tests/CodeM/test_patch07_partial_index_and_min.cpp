#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;



TEST_CASE("partial index appends remaining dims", "[adv_index]") {
  // a [2,3,2]
  auto a = Series::zeros({2,3,2}, ScalarType::Float64);
  auto* ap = reinterpret_cast<double*>(a.raw_data());
  for (int i=0;i<12;++i) ap[i] = double(i+1);

  // index only first dim with shape [2]
  auto i0 = Series::zeros({2}, ScalarType::Int64);
  auto* i0p = reinterpret_cast<std::int64_t*>(i0.raw_data());
  i0p[0]=1; i0p[1]=0;

  auto out = a.index({i0});
  // out shape = [2] + [3,2] => [2,3,2]
  REQUIRE(out.sizes()[0] == 2);
  REQUIRE(out.sizes()[1] == 3);
  REQUIRE(out.sizes()[2] == 2);

  auto* op = reinterpret_cast<const double*>(out.raw_data());
  // first block should be a[1,:,:] (rows 2nd in original)
  REQUIRE(op[0] == ap[6]);
  REQUIRE(op[1] == ap[7]);
}

TEST_CASE("index_put supports values head_total and full", "[adv_index]") {
  auto a = Series::zeros({2,3,2}, ScalarType::Float32);
  auto i0 = Series::zeros({2}, ScalarType::Int64);
  auto* i0p = reinterpret_cast<std::int64_t*>(i0.raw_data());
  i0p[0]=0; i0p[1]=1;

  // values head_total=2 => broadcast over tail(3*2)
  auto v = Series::zeros({2}, ScalarType::Float32);
  auto* vp = reinterpret_cast<float*>(v.raw_data());
  vp[0]=1.0f; vp[1]=2.0f;

  auto out = pluss::index_put(a, std::vector<Series>{i0}, v, false);
  auto* op = reinterpret_cast<const float*>(out.raw_data());
  // a[0,:,:] all ones; a[1,:,:] all twos
  REQUIRE(op[0] == 1.0f);
  REQUIRE(op[5] == 1.0f);
  REQUIRE(op[6] == 2.0f);
  REQUIRE(op[11] == 2.0f);
}

TEST_CASE("amin/argmin", "[reduction]") {
  auto a = Series::zeros({2,3}, ScalarType::Float64);
  auto* ap = reinterpret_cast<double*>(a.raw_data());
  // [[3,1,2],[6,4,5]]
  ap[0]=3; ap[1]=1; ap[2]=2; ap[3]=6; ap[4]=4; ap[5]=5;

  auto mn = a.amin(1, false);
  auto* mp = reinterpret_cast<const double*>(mn.raw_data());
  REQUIRE(mp[0] == 1.0);
  REQUIRE(mp[1] == 4.0);

  auto am = a.argmin(1, false);
  auto* ip = reinterpret_cast<const std::int64_t*>(am.raw_data());
  REQUIRE(ip[0] == 1);
  REQUIRE(ip[1] == 1);
}

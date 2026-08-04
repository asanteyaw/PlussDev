#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <pluss/config.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;


#include <vector>


TEST_CASE("config unordered map and tags", "[config]") {
  auto cfg1 = pluss::config(std::unordered_map<std::string, std::string>{
      {"dtype", "float16"},
      {"pinned", "true"},
  });
  Series a = pluss::series::zeros({2,2}, cfg1);
  REQUIRE(a.scalar_type() == ScalarType::Float16);

  Series b = pluss::series::zeros({2,2}, pluss::cfg::dtype(ScalarType::Int16), pluss::cfg::pinned(false));
  REQUIRE(b.scalar_type() == ScalarType::Int16);
}

TEST_CASE("make_series scalar and shaped factories", "[factory]") {
  Series v = pluss::make_series(2.0, ScalarType::Float64);
  REQUIRE(v.scalar_type() == ScalarType::Float64);

  std::vector<std::int64_t> data = {1,2,3,4};
  const std::vector<std::int64_t> sizes = {2,2};
  Series x = pluss::make_series(data, std::span<const std::int64_t>(sizes));
  REQUIRE(x.scalar_type() == ScalarType::Int64);
  auto* p = reinterpret_cast<const std::int64_t*>(x.raw_data());
  REQUIRE(p[3] == 4);
}

TEST_CASE("from_buffer external memory", "[from_buffer]") {
  auto* mem = (std::int32_t*)std::malloc(sizeof(std::int32_t)*6);
  for (int i=0;i<6;++i) mem[i]=i;
  bool freed=false;
  const std::vector<std::int64_t> sizes = {2,3};
  const std::vector<std::int64_t> strides = {3,1};
  Series s = pluss::from_buffer(
      mem,
      std::span<const std::int64_t>(sizes),
      std::span<const std::int64_t>(strides),
      [&](void* p){ freed=true; std::free(p); },
      ScalarType::Int32);
  REQUIRE(s.scalar_type() == ScalarType::Int32);
  auto* p = reinterpret_cast<const std::int32_t*>(s.raw_data());
  REQUIRE(p[5] == 5);
  // when s destructs, deleter called (freed becomes true) - can't reliably test here, but ensure no crash
}

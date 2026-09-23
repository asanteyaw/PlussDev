#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;


TEST_CASE("linspace values correct", "[factories]") {
  auto x = pluss::series::linspace(0.0, 1.0, 5, pluss::config({{"dtype","float64"}}));
  auto xc = x.contiguous();
  const auto* p = reinterpret_cast<const double*>(xc.raw_data());
  REQUIRE(p[0] == 0.0);
  REQUIRE(p[4] == 1.0);
}

TEST_CASE("eye diagonal correct", "[factories]") {
  auto I = pluss::series::eye(4, 4, pluss::config({{"dtype","int32"}}));
  auto Ic = I.contiguous();
  const auto* p = reinterpret_cast<const std::int32_t*>(Ic.raw_data());
  for (int r=0;r<4;++r) {
    for (int c=0;c<4;++c) {
      const auto v = p[r*4 + c];
      if (r==c) REQUIRE(v == 1);
      else REQUIRE(v == 0);
    }
  }
}

TEST_CASE("zeros_like inherits dtype by default", "[factories]") {
  auto a = pluss::series::zeros({2,3}, pluss::ScalarType::Int16);
  auto z = pluss::series::zeros_like(a);
  REQUIRE(z.scalar_type() == pluss::ScalarType::Int16);
}

TEST_CASE("like factories inherit options and strided layout", "[factories][like]") {
  auto base = pluss::series::make({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
  });
  auto like = base.transpose(0, 1);

  const auto z = pluss::series::zeros_like(like);
  REQUIRE(z.sizes() == like.sizes());
  REQUIRE(z.strides() == like.strides());
  REQUIRE(z.scalar_type() == like.scalar_type());
  REQUIRE(z.platform() == like.platform());
  REQUIRE_FALSE(z.source().has_value());
  REQUIRE(z.to_vec<double>() == std::vector<double>(6, 0.0));

  const auto o = pluss::series::ones_like(like);
  REQUIRE(o.strides() == like.strides());
  REQUIRE(o.to_vec<double>() == std::vector<double>(6, 1.0));

  const auto f = pluss::series::full_like(like, 7.0);
  REQUIRE(f.strides() == like.strides());
  REQUIRE(f.to_vec<double>() == std::vector<double>(6, 7.0));
}

TEST_CASE("like factory default-valued options remain explicit overrides", "[factories][like]") {
  auto like = pluss::series::zeros({2, 3}, pluss::ScalarType::Int16);
  like.tracks_slope_(true);
  const auto z = pluss::series::zeros_like(
      like,
      pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU));

  REQUIRE(z.scalar_type() == pluss::ScalarType::Float32);
  REQUIRE(z.platform() == pluss::pCPU);
  REQUIRE_FALSE(z.tracks_slope());
}

TEST_CASE("default-valued dtype overrides are explicit outside like factories", "[factories][config]") {
  REQUIRE(pluss::series::arange(
              0,
              4,
              pluss::Config{}.with_dtype(pluss::ScalarType::Float32))
              .scalar_type() == pluss::ScalarType::Float32);
  REQUIRE(pluss::series::randint(
              0,
              4,
              {8},
              pluss::Config{}.with_dtype(pluss::ScalarType::Float32),
              17)
              .scalar_type() == pluss::ScalarType::Float32);
}

TEST_CASE("like factories inherit and can override platform", "[factories][like][mps]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const auto like = pluss::series::zeros({2, 3}, pluss::ScalarType::Float32)
                        .to(pluss::pMPS)
                        .transpose(0, 1);
  const auto inherited = pluss::series::zeros_like(like);
  REQUIRE(inherited.platform() == pluss::pMPS);
  REQUIRE(inherited.strides() == like.strides());
  REQUIRE(inherited.to(pluss::pCPU).to_vec<float>() == std::vector<float>(6, 0.0f));
  REQUIRE(pluss::series::zeros_like(
              like,
              pluss::Config{}.with_platform(pluss::pCPU))
              .platform() == pluss::pCPU);
}

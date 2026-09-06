#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#ifdef PLUSS_ENABLE_TORCH
#include <pluss/codem/torch_interop.h>

#include <cstdint>
#include <vector>
#endif

TEST_CASE("torch interop header compiles", "[interop]") {
  REQUIRE(true);
}

#ifdef PLUSS_ENABLE_TORCH

namespace {

pluss::Series import_after_source_handle_dies() {
  auto owner = torch::arange(8, torch::TensorOptions().dtype(torch::kInt64));
  return pluss::series::from_torch(owner.slice(0, 2, 6));
}

torch::Tensor export_after_source_handle_dies() {
  auto owner = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3});
  return owner.tensor();
}

} // namespace

TEST_CASE("from_torch imports an offset view exactly once", "[interop][torch]") {
  auto imported = import_after_source_handle_dies();

  REQUIRE(imported.to_vec<std::int64_t>() == std::vector<std::int64_t>{2, 3, 4, 5});
}

TEST_CASE("torch interop handles keep shared storage alive", "[interop][torch]") {
  auto exported = export_after_source_handle_dies();

  REQUIRE(exported.accessor<double, 1>()[0] == 1.0);
  exported.accessor<double, 1>()[2] = 9.0;
  REQUIRE(exported.accessor<double, 1>()[2] == 9.0);
}

TEST_CASE("Series tensor export can begin a fresh training graph", "[interop][torch]") {
  auto values = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3});
  auto parameter = values.tensor();

  REQUIRE_FALSE(parameter.requires_grad());
  parameter.requires_grad_(true);
  auto loss = parameter.square().sum();
  loss.backward();

  REQUIRE(parameter.is_leaf());
  REQUIRE(torch::allclose(parameter.grad(), torch::tensor({2.0, 4.0, 6.0}, torch::kFloat64)));
}

TEST_CASE("from_torch import can begin a fresh Pluss reverse graph", "[interop][torch][autodiff]") {
  auto source = torch::tensor({1.0, 2.0, 3.0}, torch::kFloat64);
  auto values = pluss::series::from_torch(source);

  REQUIRE_FALSE(values.tracks_slope());
  values.tracks_slope_(true);
  auto loss = values.square().sum();
  loss.reverse();

  REQUIRE(values.has_sensitivity());
  REQUIRE(values.sensitivity().to_vec<double>() == std::vector<double>{2.0, 4.0, 6.0});
}

TEST_CASE("from_torch import can enter a Pluss CUDA reverse graph", "[interop][torch][cuda][autodiff]") {
  if (!pluss::cuda::is_ready()) {
    SUCCEED("CUDA runtime unavailable on this host");
    return;
  }

  auto source = torch::tensor({1.0F, 2.0F, 3.0F}, torch::kFloat32);
  auto values = pluss::series::from_torch(source);
  values.tracks_slope_(true);

  auto device_values = values.to(pluss::pCUDA);
  auto loss = device_values.square().sum();
  loss.reverse();

  REQUIRE(values.sensitivity().to_vec<float>() == std::vector<float>{2.0F, 4.0F, 6.0F});
}

TEST_CASE("complex dtypes round trip through torch interop", "[interop][torch]") {
  auto source = torch::zeros({2}, torch::TensorOptions().dtype(torch::kComplexFloat));
  auto round_trip = pluss::series::from_torch(source).tensor();

  REQUIRE(round_trip.scalar_type() == torch::kComplexFloat);
  REQUIRE(round_trip.data_ptr() == source.data_ptr());
}

#endif

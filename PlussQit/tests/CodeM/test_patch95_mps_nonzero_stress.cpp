#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cstdint>
#include <functional>
#include <random>
#include <span>
#include <thread>
#include <vector>

namespace {

std::vector<std::int64_t> sizes_vec(std::span<const std::int64_t> s) {
  return std::vector<std::int64_t>(s.begin(), s.end());
}

} // namespace

TEST_CASE("MPS nonzero 1D parity and stress", "[mps][nonzero][stress]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  std::mt19937 rng(20260313u);
  std::uniform_int_distribution<std::int64_t> n_dist(1, 4096);
  std::bernoulli_distribution keep_dist(0.42);

  for (std::int64_t iter = 0; iter < 160; ++iter) {
    const auto n = n_dist(rng);
    std::vector<float> data(static_cast<std::size_t>(n), 0.0f);
    for (std::int64_t i = 0; i < n; ++i) {
      data[static_cast<std::size_t>(i)] = keep_dist(rng) ? static_cast<float>((i % 31) + 1) : 0.0f;
    }

    const auto x_cpu = pluss::make_series(data, {n});
    const auto ref = x_cpu.nonzero();
    const auto got = x_cpu.to(pluss::pMPS).nonzero().to(pluss::pCPU);

    REQUIRE(got.scalar_type() == pluss::ScalarType::Int64);
    REQUIRE(sizes_vec(got.sizes()) == sizes_vec(ref.sizes()));
    REQUIRE(got.host<std::int64_t>() == ref.host<std::int64_t>());
  }

  const auto lead_zero = pluss::make_series(std::vector<float>{0.0f, 0.0f, 4.0f, 0.0f, 5.0f, 0.0f}, {6});
  const auto lead_ref = lead_zero.nonzero();
  const auto lead_got = lead_zero.to(pluss::pMPS).nonzero().to(pluss::pCPU);
  REQUIRE(sizes_vec(lead_got.sizes()) == sizes_vec(lead_ref.sizes()));
  REQUIRE(lead_got.host<std::int64_t>() == lead_ref.host<std::int64_t>());
}

TEST_CASE("MPS nonzero scalar parity", "[mps][nonzero]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const auto z_cpu = pluss::make_series(0.0f, pluss::ScalarType::Float32);
  const auto z_mps = z_cpu.to(pluss::pMPS).nonzero().to(pluss::pCPU);
  REQUIRE(sizes_vec(z_mps.sizes()) == std::vector<std::int64_t>{0, 0});

  const auto o_cpu = pluss::make_series(3.0f, pluss::ScalarType::Float32);
  const auto o_mps = o_cpu.to(pluss::pMPS).nonzero().to(pluss::pCPU);
  REQUIRE(sizes_vec(o_mps.sizes()) == std::vector<std::int64_t>{1, 0});
}

TEST_CASE("MPS nonzero repeated concurrency stability", "[mps][nonzero][stability]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const std::vector<float> data{
      0.0f, 1.0f, 0.0f, 2.0f, 3.0f, 0.0f, 4.0f, 0.0f,
      5.0f, 0.0f, 6.0f, 0.0f, 7.0f, 8.0f, 0.0f, 9.0f};
  const auto x_cpu = pluss::make_series(data, {static_cast<std::int64_t>(data.size())});
  const auto ref = x_cpu.nonzero().host<std::int64_t>();
  const auto x_mps = x_cpu.to(pluss::pMPS);

  bool ok_a = true;
  bool ok_b = true;

  auto worker = [&](bool& ok_flag) {
    try {
      for (std::int64_t i = 0; i < 200; ++i) {
        const auto got = x_mps.nonzero().to(pluss::pCPU).host<std::int64_t>();
        if (got != ref) {
          ok_flag = false;
          return;
        }
      }
    } catch (...) {
      ok_flag = false;
    }
  };

  std::thread t1(worker, std::ref(ok_a));
  std::thread t2(worker, std::ref(ok_b));
  t1.join();
  t2.join();

  REQUIRE(ok_a);
  REQUIRE(ok_b);
}

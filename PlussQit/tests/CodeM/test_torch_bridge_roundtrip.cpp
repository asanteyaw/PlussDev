#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#ifdef PLUSS_TORCH_INTEROP_AVAILABLE

#include <cstdint>
#include <vector>

// The bridge is header-only and compiled in the consumer: it needs no PLUSS_ENABLE_TORCH build,
// only Torch's headers on the include path.

TEST_CASE("torch -> Series shares memory", "[interop][torch]") {
  at::Tensor t = at::arange(6, at::TensorOptions().dtype(at::kDouble)).reshape({2, 3});
  pluss::Series s = pluss::series::from_torch(t);
  REQUIRE(s.sizes().size() == 2);
  t[0][0] = 100.0;  // visible through the Series
  REQUIRE(s.sum().item<double>() == 115.0);
}

TEST_CASE("Series -> torch shares memory", "[interop][torch]") {
  pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3});
  at::Tensor u = x.tensor();
  u[1] = 20.0;
  REQUIRE(x.mean().item<double>() == 8.0);
}

TEST_CASE("memory outlives whichever side dies first", "[interop][torch]") {
  at::Tensor keep;
  {
    pluss::Series tmp = pluss::make_series(std::vector<double>{4.0, 5.0}, {2});
    keep = tmp.tensor();
  }
  REQUIRE(keep[1].item<double>() == 5.0);

  pluss::Series view;
  {
    at::Tensor owner = at::arange(8, at::TensorOptions().dtype(at::kLong));
    view = pluss::series::from_torch(owner.slice(0, 2, 6));
  }
  REQUIRE(view.sum().item<std::int64_t>() == 2 + 3 + 4 + 5);
}

TEST_CASE("unsupported inputs are refused with a hint", "[interop][torch]") {
  REQUIRE_THROWS(pluss::series::from_torch(at::Tensor{}));
  REQUIRE_THROWS(pluss::series::from_torch(at::ones({2, 2}).to_sparse()));
}

#else

TEST_CASE("torch bridge is inactive without Torch headers", "[interop]") {
  SUCCEED();
}

#endif

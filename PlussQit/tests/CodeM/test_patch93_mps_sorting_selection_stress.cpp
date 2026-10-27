#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

namespace {

template <class Got, class Want>
void require_close_vec(const Got& got, const Want& want, float atol = 1e-4f) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(std::fabs(got[i] - want[i]) <= atol);
  }
}

} // namespace

TEST_CASE("MPS sort/topk/argsort/kthvalue edge parity", "[mps][sort][topk][argsort][kthvalue]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  auto x_cpu = pluss::make_series(
      std::vector<float>{
          5.0f, -2.0f, 8.0f, 8.0f, 1.0f,
          3.0f, 7.0f, -4.0f, 6.0f, 2.0f,
          9.0f, 0.0f, 4.0f, -1.0f, 10.0f},
      {3, 5});
  auto x_mps = x_cpu.to(pluss::pMPS);

  const auto sort_cpu = x_cpu.sort(/*dim=*/-1, /*descending=*/true, /*stable=*/false);
  const auto sort_mps = x_mps.sort(/*dim=*/-1, /*descending=*/true, /*stable=*/false);
  REQUIRE(sort_mps.platform() == pluss::pMPS);
  require_close_vec(sort_mps.to(pluss::pCPU).host<float>(), sort_cpu.host<float>());

  const auto topk0_cpu = x_cpu.topk(/*k=*/0, /*dim=*/1, /*largest=*/true, /*sorted=*/true);
  const auto topk0_mps = x_mps.topk(/*k=*/0, /*dim=*/1, /*largest=*/true, /*sorted=*/true);
  REQUIRE(topk0_mps.platform() == pluss::pMPS);
  REQUIRE(topk0_mps.numel() == 0);
  REQUIRE(topk0_cpu.numel() == 0);

  const auto topk_all_cpu = x_cpu.topk(/*k=*/5, /*dim=*/1, /*largest=*/false, /*sorted=*/true);
  const auto topk_all_mps = x_mps.topk(/*k=*/5, /*dim=*/1, /*largest=*/false, /*sorted=*/true);
  require_close_vec(topk_all_mps.to(pluss::pCPU).host<float>(), topk_all_cpu.host<float>());

  const auto kth_cpu = x_cpu.kthvalue(/*k=*/2, /*dim=*/-1, /*keepdim=*/true);
  const auto kth_mps = x_mps.kthvalue(/*k=*/2, /*dim=*/-1, /*keepdim=*/true);
  REQUIRE(kth_mps.platform() == pluss::pMPS);
  require_close_vec(kth_mps.to(pluss::pCPU).host<float>(), kth_cpu.host<float>());

  const auto argsort_cpu = x_cpu.argsort(/*dim=*/1, /*descending=*/false, /*stable=*/false);
  const auto argsort_mps = x_mps.argsort(/*dim=*/1, /*descending=*/false, /*stable=*/false);
  REQUIRE(argsort_mps.platform() == pluss::pMPS);
  REQUIRE(argsort_mps.to(pluss::pCPU).host<std::int64_t>() == argsort_cpu.host<std::int64_t>().array().data);

  const auto half_cpu = x_cpu.to(pluss::ScalarType::Float16);
  const auto half_mps = half_cpu.to(pluss::pMPS);
  const auto half_topk_mps =
      half_mps.topk(/*k=*/3, /*dim=*/1, /*largest=*/true, /*sorted=*/true).to(pluss::pCPU).to(pluss::ScalarType::Float32);
  const auto half_topk_ref =
      half_cpu.to(pluss::ScalarType::Float32).topk(/*k=*/3, /*dim=*/1, /*largest=*/true, /*sorted=*/true);
  require_close_vec(half_topk_mps.host<float>(), half_topk_ref.host<float>());
}

TEST_CASE("MPS selection stress parity random matrix sweep", "[mps][stress][topk][argsort][kthvalue]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  std::mt19937 rng(20260310u);
  std::uniform_int_distribution<std::int64_t> rows_dist(1, 12);
  std::uniform_int_distribution<std::int64_t> cols_dist(1, 16);
  std::uniform_real_distribution<float> val_dist(-50.0f, 50.0f);

  for (std::int64_t iter = 0; iter < 180; ++iter) {
    const auto rows = rows_dist(rng);
    const auto cols = cols_dist(rng);
    std::vector<float> data(static_cast<std::size_t>(rows * cols));
    for (auto& v : data) v = val_dist(rng);

    const auto x_cpu = pluss::make_series(data, {rows, cols});
    const auto x_mps = x_cpu.to(pluss::pMPS);

    const std::int64_t dim = (iter % 3 == 0) ? -1 : ((iter % 2 == 0) ? 0 : 1);
    const auto dim_size = (dim == 0) ? rows : cols;
    const std::int64_t k = iter % (dim_size + 1);
    const bool largest = (iter % 2 == 0);
    const bool sorted = (iter % 4 != 0);
    const bool keepdim = (iter % 5 == 0);

    const auto topk_cpu = x_cpu.topk(k, dim, largest, sorted);
    const auto topk_mps = x_mps.topk(k, dim, largest, sorted).to(pluss::pCPU);
    require_close_vec(topk_mps.host<float>(), topk_cpu.host<float>());

    const auto argsort_cpu = x_cpu.argsort(dim, largest, /*stable=*/false);
    const auto argsort_mps = x_mps.argsort(dim, largest, /*stable=*/false).to(pluss::pCPU);
    REQUIRE(argsort_mps.host<std::int64_t>() == argsort_cpu.host<std::int64_t>().array().data);

    const std::int64_t kth_k = std::max<std::int64_t>(1, std::min<std::int64_t>(dim_size, (k == 0 ? 1 : k)));
    const auto kth_cpu = x_cpu.kthvalue(kth_k, dim, keepdim);
    const auto kth_mps = x_mps.kthvalue(kth_k, dim, keepdim).to(pluss::pCPU);
    require_close_vec(kth_mps.host<float>(), kth_cpu.host<float>());
  }
}

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace {

class ScopedEnvVar final {
public:
  ScopedEnvVar(const char* key, const char* value) : key_(key) {
    if (const char* old = std::getenv(key_)) {
      old_ = std::string(old);
    }
#if defined(_WIN32)
    _putenv_s(key_, value);
#else
    setenv(key_, value, 1);
#endif
  }

  ~ScopedEnvVar() {
    if (old_.has_value()) {
#if defined(_WIN32)
      _putenv_s(key_, old_->c_str());
#else
      setenv(key_, old_->c_str(), 1);
#endif
    } else {
#if defined(_WIN32)
      _putenv_s(key_, "");
#else
      unsetenv(key_);
#endif
    }
  }

private:
  const char* key_;
  std::optional<std::string> old_;
};

template <class Got, class Want>
void require_equal_(const Got& got, const Want& want) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(got[i] == want[i]);
  }
}

} // namespace

TEST_CASE("MPS strict nonzero parity covers scalar/empty/discontiguous lanes", "[mps][nonzero][strict]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  const ScopedEnvVar fallback_off("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");

  const auto f_cpu = pluss::make_series(
      std::vector<float>{0.f, -1.f, 2.f, 0.f, 3.f, 0.f},
      {2, 3});
  require_equal_(
      f_cpu.to(pluss::pMPS).nonzero().to(pluss::pCPU).host<std::int64_t>(),
      f_cpu.nonzero().host<std::int64_t>());

  const auto i32_cpu = pluss::make_series(
      std::vector<std::int32_t>{0, -1, 2, 0, 3, 0},
      {2, 3});
  require_equal_(
      i32_cpu.to(pluss::pMPS).nonzero().to(pluss::pCPU).host<std::int64_t>(),
      i32_cpu.nonzero().host<std::int64_t>());

  const auto b_cpu = pluss::make_series(
      std::vector<bool>{false, true, false, true, false, true},
      {2, 3});
  require_equal_(
      b_cpu.to(pluss::pMPS).nonzero().to(pluss::pCPU).host<std::int64_t>(),
      b_cpu.nonzero().host<std::int64_t>());

  const auto scalar_true = pluss::make_series(std::vector<float>{5.f}, {}).to(pluss::pMPS);
  const auto scalar_false = pluss::make_series(std::vector<float>{0.f}, {}).to(pluss::pMPS);
  require_equal_(
      scalar_true.nonzero().to(pluss::pCPU).host<std::int64_t>(),
      std::vector<std::int64_t>{});
  REQUIRE(scalar_true.nonzero().sizes()[0] == 1);
  REQUIRE(scalar_true.nonzero().sizes()[1] == 0);
  REQUIRE(scalar_false.nonzero().sizes()[0] == 0);
  REQUIRE(scalar_false.nonzero().sizes()[1] == 0);

  const auto empty_cpu = pluss::Series::zeros({0, 3}, pluss::ScalarType::Float32);
  const auto empty_mps_nz = empty_cpu.to(pluss::pMPS).nonzero();
  REQUIRE(empty_mps_nz.platform() == pluss::pMPS);
  REQUIRE(empty_mps_nz.sizes()[0] == 0);
  REQUIRE(empty_mps_nz.sizes()[1] == 2);

  const auto discontig_cpu = pluss::make_series(
                                  std::vector<float>{
                                      0.f, 1.f, 0.f, 2.f, 3.f, 0.f, 4.f, 0.f,
                                      0.f, 5.f, 6.f, 0.f, 7.f, 0.f, 8.f, 9.f,
                                      0.f, 0.f, 1.f, 2.f, 3.f, 4.f, 0.f, 5.f},
                                  {2, 3, 4})
                                  .transpose(1, 2);
  require_equal_(
      discontig_cpu.to(pluss::pMPS).nonzero().to(pluss::pCPU).host<std::int64_t>(),
      discontig_cpu.nonzero().host<std::int64_t>());
}

TEST_CASE("MPS nonzero survives concurrent invocation", "[mps][nonzero][threading]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  const ScopedEnvVar fallback_off("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");

  const auto x = pluss::Series::ones({256, 256}, pluss::ScalarType::Float32).to(pluss::pMPS);
  std::atomic<bool> failed{false};
  std::vector<std::int64_t> r0;
  std::vector<std::int64_t> r1;

  auto worker = [&](std::vector<std::int64_t>& out) {
    try {
      out = x.nonzero().to(pluss::pCPU).host<std::int64_t>().array().data;
    } catch (...) {
      failed.store(true);
    }
  };

  std::thread t0(worker, std::ref(r0));
  std::thread t1(worker, std::ref(r1));
  t0.join();
  t1.join();
  REQUIRE_FALSE(failed.load());
  require_equal_(r0, r1);
  REQUIRE(r0.size() == static_cast<std::size_t>(256 * 256 * 2));
}

TEST_CASE("MPS strict int32 indexing/compose lanes match CPU", "[mps][strict][int32][indexing][compose]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  const ScopedEnvVar fallback_off("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar native_take_put("PLUSS_MPS_NATIVE_TAKE_PUT", "1");

  const auto x_cpu = pluss::make_series(std::vector<std::int32_t>{5, 4, 3, 2, 1, 0}, {2, 3});
  const auto idx_flat_cpu = pluss::make_series(std::vector<std::int64_t>{5, 1, 2, 0}, {2, 2});
  require_equal_(
      x_cpu.reshape({x_cpu.numel()}).to(pluss::pMPS).take(idx_flat_cpu.to(pluss::pMPS)).to(pluss::pCPU).host<std::int32_t>(),
      x_cpu.reshape({x_cpu.numel()}).take(idx_flat_cpu).host<std::int32_t>());

  auto put_fn = static_cast<pluss::Series (pluss::Series::*)(const pluss::Series&, const pluss::Series&, bool)>(&pluss::Series::put_raw_);
  auto base_put_cpu = pluss::Series::zeros({6}, pluss::ScalarType::Int32);
  auto base_put_mps = base_put_cpu.to(pluss::pMPS);
  const auto put_src_cpu = pluss::make_series(std::vector<std::int32_t>{7, 8, 9, 10}, {4});
  base_put_cpu = (base_put_cpu.*put_fn)(idx_flat_cpu.reshape({4}), put_src_cpu, /*accumulate=*/false);
  base_put_mps = (base_put_mps.*put_fn)(idx_flat_cpu.reshape({4}).to(pluss::pMPS), put_src_cpu.to(pluss::pMPS), /*accumulate=*/false);
  require_equal_(base_put_mps.to(pluss::pCPU).host<std::int32_t>(), base_put_cpu.host<std::int32_t>());

  auto base_put_acc_cpu = pluss::Series::zeros({6}, pluss::ScalarType::Int32);
  auto base_put_acc_mps = base_put_acc_cpu.to(pluss::pMPS);
  const auto put_idx_acc_cpu = pluss::make_series(std::vector<std::int64_t>{1, 1, 2, 2}, {4});
  const auto put_src_acc_cpu = pluss::make_series(std::vector<std::int32_t>{2, 3, 4, 5}, {4});
  base_put_acc_cpu = (base_put_acc_cpu.*put_fn)(put_idx_acc_cpu, put_src_acc_cpu, /*accumulate=*/true);
  base_put_acc_mps =
      (base_put_acc_mps.*put_fn)(put_idx_acc_cpu.to(pluss::pMPS), put_src_acc_cpu.to(pluss::pMPS), /*accumulate=*/true);
  require_equal_(base_put_acc_mps.to(pluss::pCPU).host<std::int32_t>(), base_put_acc_cpu.host<std::int32_t>());

  const auto gather_idx_cpu = pluss::make_series(std::vector<std::int64_t>{2, 0, 1, 1, 2, 0}, {2, 3});
  require_equal_(
      x_cpu.to(pluss::pMPS).gather(1, gather_idx_cpu.to(pluss::pMPS)).to(pluss::pCPU).host<std::int32_t>(),
      x_cpu.gather(1, gather_idx_cpu).host<std::int32_t>());

  const auto scatter_base_cpu = pluss::Series::zeros({2, 3}, pluss::ScalarType::Int32);
  require_equal_(
      scatter_base_cpu.to(pluss::pMPS).scatter(1, gather_idx_cpu.to(pluss::pMPS), x_cpu.to(pluss::pMPS)).to(pluss::pCPU).host<std::int32_t>(),
      scatter_base_cpu.scatter(1, gather_idx_cpu, x_cpu).host<std::int32_t>());

  const auto scatter_add_idx_cpu = pluss::make_series(std::vector<std::int64_t>{0, 1, 1, 2, 0, 2}, {2, 3});
  const auto scatter_add_src_cpu = pluss::make_series(std::vector<std::int32_t>{1, 2, 3, 4, 5, 6}, {2, 3});
  require_equal_(
      scatter_base_cpu.to(pluss::pMPS).scatter_add(1, scatter_add_idx_cpu.to(pluss::pMPS), scatter_add_src_cpu.to(pluss::pMPS))
          .to(pluss::pCPU)
          .host<std::int32_t>(),
      scatter_base_cpu.scatter_add(1, scatter_add_idx_cpu, scatter_add_src_cpu).host<std::int32_t>());

  const auto rr_base_cpu = pluss::Series::zeros({2, 3}, pluss::ScalarType::Int32);
  require_equal_(
      rr_base_cpu.to(pluss::pMPS).scatter_reduce(
          1,
          scatter_add_idx_cpu.to(pluss::pMPS),
          scatter_add_src_cpu.to(pluss::pMPS),
          "amax",
          true)
          .to(pluss::pCPU)
          .host<std::int32_t>(),
      rr_base_cpu.scatter_reduce(1, scatter_add_idx_cpu, scatter_add_src_cpu, "amax", true).host<std::int32_t>());
  require_equal_(
      rr_base_cpu.to(pluss::pMPS).scatter_reduce(
          1,
          scatter_add_idx_cpu.to(pluss::pMPS),
          scatter_add_src_cpu.to(pluss::pMPS),
          "amin",
          true)
          .to(pluss::pCPU)
          .host<std::int32_t>(),
      rr_base_cpu.scatter_reduce(1, scatter_add_idx_cpu, scatter_add_src_cpu, "amin", true).host<std::int32_t>());

  const auto cat0_cpu =
      pluss::make_series(std::vector<std::int32_t>{1, 2, 3, 4}, {2, 2});
  const auto cat1_cpu =
      pluss::make_series(std::vector<std::int32_t>{5, 6, 7, 8}, {2, 2});
  require_equal_(
      cat0_cpu.to(pluss::pMPS).unite({cat1_cpu.to(pluss::pMPS)}, 0, true).to(pluss::pCPU).host<std::int32_t>(),
      cat0_cpu.unite({cat1_cpu}, 0, true).host<std::int32_t>());
  require_equal_(
      cat0_cpu.to(pluss::pMPS).pile({cat1_cpu.to(pluss::pMPS)}, 0).to(pluss::pCPU).host<std::int32_t>(),
      cat0_cpu.pile({cat1_cpu}, 0).host<std::int32_t>());
}

TEST_CASE("MPS strict int32 matmul matches CPU", "[mps][strict][int32][matmul]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  const ScopedEnvVar fallback_off("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const auto a_cpu = pluss::make_series(std::vector<std::int32_t>{1, 2, 3, 4, 5, 6}, {2, 3});
  const auto b_cpu = pluss::make_series(std::vector<std::int32_t>{7, 8, 9, 10, 11, 12}, {3, 2});
  require_equal_(
      a_cpu.to(pluss::pMPS).matmul(b_cpu.to(pluss::pMPS)).to(pluss::pCPU).host<std::int32_t>(),
      a_cpu.matmul(b_cpu).host<std::int32_t>());
}

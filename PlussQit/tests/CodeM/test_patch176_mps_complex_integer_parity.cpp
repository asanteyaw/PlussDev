#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cmath>
#include <complex>
#include <cstdint>
#include <vector>

namespace {

using cf = std::complex<float>;

void require_close_c_(const pluss::Series& mps, const pluss::Series& cpu, float tol, const char* label) {
  const auto got = mps.to(pluss::pCPU).flatten(0, -1).items<cf>();
  const auto want = cpu.flatten(0, -1).items<cf>();
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    INFO(label << " i=" << i << " got=" << got[i] << " want=" << want[i]);
    REQUIRE(std::abs(got[i] - want[i]) <= tol * (1.0f + std::abs(want[i])));
  }
}

void require_close_f_(const pluss::Series& mps, const pluss::Series& cpu, float tol, const char* label) {
  const auto got = mps.to(pluss::pCPU).flatten(0, -1).items<float>();
  const auto want = cpu.flatten(0, -1).items<float>();
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    INFO(label << " i=" << i << " got=" << got[i] << " want=" << want[i]);
    REQUIRE(std::fabs(got[i] - want[i]) <= tol * (1.0f + std::fabs(want[i])));
  }
}

template <class T>
void require_equal_(const pluss::Series& mps, const pluss::Series& cpu, const char* label) {
  const auto got = mps.to(pluss::pCPU).flatten(0, -1).items<T>();
  const auto want = cpu.flatten(0, -1).items<T>();
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    INFO(label << " i=" << i);
    REQUIRE(got[i] == want[i]);
  }
}

} // namespace

TEST_CASE("MPS complex64 square, cumsum, cumprod and inverse hyperbolics match CPU", "[mps][complex]") {
  if (!pluss::mps::is_ready()) { SUCCEED("MPS runtime unavailable"); return; }
  const auto z = pluss::make_series(
      std::vector<cf>{{0.5f, 0.25f}, {1.5f, -0.5f}, {0.3f, 0.8f}, {-0.7f, 0.2f}, {1.1f, 1.2f}, {0.9f, -0.4f}},
      {2, 3});
  const auto m = z.to(pluss::pMPS);
  require_close_c_(m.square(), z.square(), 1e-5f, "square");
  require_close_c_(m.cumsum(1), z.cumsum(1), 1e-5f, "cumsum");
  require_close_c_(m.cumprod(1), z.cumprod(1), 1e-4f, "cumprod");
  require_close_c_(m.rsqrt(), z.rsqrt(), 1e-4f, "rsqrt");
  require_close_c_(m.asinh(), z.asinh(), 1e-4f, "asinh");
  require_close_c_(m.add(1.5).acosh(), z.add(1.5).acosh(), 1e-4f, "acosh");
  require_close_c_(m.mul(0.3).atanh(), z.mul(0.3).atanh(), 1e-4f, "atanh");
  REQUIRE(m.isnan().to(pluss::pCPU).flatten(0, -1).items<bool>() == z.isnan().flatten(0, -1).items<bool>());
  REQUIRE(m.isinf().to(pluss::pCPU).flatten(0, -1).items<bool>() == z.isinf().flatten(0, -1).items<bool>());
}

TEST_CASE("MPS float32 lgamma matches CPU including reflection and poles", "[mps][float32]") {
  if (!pluss::mps::is_ready()) { SUCCEED("MPS runtime unavailable"); return; }
  const auto x = pluss::make_series(std::vector<float>{0.1f, 0.5f, 1.0f, 2.5f, 7.3f, 20.0f, -0.5f, -1.5f, -2.3f}, {9});
  require_close_f_(x.to(pluss::pMPS).lgamma(), x.lgamma(), 2e-4f, "lgamma");
}

TEST_CASE("MPS integer dtypes match CPU for rounding, comparison, scan and join ops", "[mps][integer]") {
  if (!pluss::mps::is_ready()) { SUCCEED("MPS runtime unavailable"); return; }
  const auto a = pluss::make_series(std::vector<std::int64_t>{-3, 4, 0, 7, -1, 2}, {2, 3});
  const auto b = pluss::make_series(std::vector<std::int64_t>{2, -5, 1, 7, 3, -2}, {2, 3});
  const auto am = a.to(pluss::pMPS);
  const auto bm = b.to(pluss::pMPS);
  require_equal_<std::int64_t>(am.square(), a.square(), "square");
  require_equal_<std::int64_t>(am.relu(), a.relu(), "relu");
  require_equal_<std::int64_t>(am.floor(), a.floor(), "floor");
  require_equal_<std::int64_t>(am.ceil(), a.ceil(), "ceil");
  require_equal_<std::int64_t>(am.maximum(bm), a.maximum(b), "maximum");
  require_equal_<std::int64_t>(am.minimum(bm), a.minimum(b), "minimum");
  require_equal_<std::int64_t>(am.clamp_min(0.0), a.clamp_min(0.0), "clamp_min");
  require_equal_<std::int64_t>(am.cumsum(1), a.cumsum(1), "cumsum");
  require_equal_<std::int64_t>(am.unite({bm}, 1), a.unite({b}, 1), "unite");
  require_equal_<std::int64_t>(am.abs().pow(bm.abs()), a.abs().pow(b.abs()), "pow");
  require_equal_<bool>(am.isnan(), a.isnan(), "isnan");
  require_close_f_(am.div(bm.add(10)), a.div(b.add(10)), 1e-6f, "true division");
}

TEST_CASE("MPS float32 unique, median, nextafter, cov and corrcoef match CPU on the device", "[mps][float32]") {
  if (!pluss::mps::is_ready()) { SUCCEED("MPS runtime unavailable"); return; }
  const auto x = pluss::make_series(std::vector<float>{3, 1, 2, 9, 5, 4, 3, 1}, {2, 4});
  const auto m = x.to(pluss::pMPS);
  require_close_f_(m.unique(), x.unique(), 0.0f, "unique");
  require_close_f_(m.median(), x.median(), 0.0f, "median");
  require_close_f_(m.median(1), x.median(1), 0.0f, "median(dim)");

  const float nan = std::nanf("");
  const auto with_nan = pluss::make_series(std::vector<float>{3, nan, 2, 9, 5, 4, 3, 1}, {2, 4});
  const auto with_nan_mps = with_nan.to(pluss::pMPS);
  require_close_f_(with_nan_mps.nanmedian(), with_nan.nanmedian(), 0.0f, "nanmedian");
  require_close_f_(with_nan_mps.nanmedian(1), with_nan.nanmedian(1), 0.0f, "nanmedian(dim)");
  REQUIRE(std::isnan(with_nan_mps.median().to(pluss::pCPU).item<float>()));

  const auto mixed = pluss::make_series(std::vector<float>{3, -1, 2, -1, nan, 3, nan, 0.5f}, {8});
  const auto unique_cpu = mixed.unique().flatten(0, -1).items<float>();
  const auto unique_mps = mixed.to(pluss::pMPS).unique().to(pluss::pCPU).flatten(0, -1).items<float>();
  REQUIRE(unique_mps.size() == unique_cpu.size());
  REQUIRE(unique_mps.size() == 5);

  const auto from = pluss::make_series(std::vector<float>{0, 1, -1, 2.5f, -3}, {5});
  const auto toward = pluss::make_series(std::vector<float>{1, 0, 0, 2.5f, 10}, {5});
  require_close_f_(from.to(pluss::pMPS).nextafter(toward.to(pluss::pMPS)), from.nextafter(toward), 0.0f, "nextafter");

  const auto data = pluss::make_series(std::vector<float>{1, 2, 3, 4, 5, 6, 7, 8, 2, 1, 0, 4}, {3, 4});
  require_close_f_(data.to(pluss::pMPS).cov(), data.cov(), 1e-5f, "cov");
  require_close_f_(data.to(pluss::pMPS).corrcoef(), data.corrcoef(), 1e-5f, "corrcoef");
}

TEST_CASE("MPS int64 arithmetic, reductions and sorting are exact beyond 2^24 and 2^31", "[mps][integer][exact]") {
  if (!pluss::mps::is_ready()) { SUCCEED("MPS runtime unavailable"); return; }
  const auto a = pluss::make_series(
      std::vector<std::int64_t>{5000000001LL, -7000000003LL, 2LL, 9000000007LL, 5500000005LL, -4LL}, {6});
  const auto b = pluss::make_series(
      std::vector<std::int64_t>{5LL, 15000000023LL, -8LL, 2500000029LL, 7LL, 9500000031LL}, {6});
  const auto am = a.to(pluss::pMPS);
  const auto bm = b.to(pluss::pMPS);
  require_equal_<std::int64_t>(am.add(bm), a.add(b), "add");
  require_equal_<std::int64_t>(am.sub(bm), a.sub(b), "sub");
  require_equal_<std::int64_t>(am.mul(bm), a.mul(b), "mul");
  require_equal_<std::int64_t>(am.neg(), a.neg(), "neg");
  require_equal_<std::int64_t>(am.abs(), a.abs(), "abs");
  require_equal_<std::int64_t>(am.maximum(bm), a.maximum(b), "maximum");
  require_equal_<std::int64_t>(am.sort(), a.sort(), "sort");
  require_equal_<std::int64_t>(am.argsort(), a.argsort(), "argsort");
  require_equal_<std::int64_t>(am.sum(), a.sum(), "sum");
  require_equal_<std::int64_t>(am.max(), a.max(), "max");
  require_equal_<std::int64_t>(am.min(), a.min(), "min");
  require_equal_<std::int64_t>(am.cumsum(0), a.cumsum(0), "cumsum");
  require_equal_<bool>(am.gt(bm), a.gt(b), "gt");
  require_equal_<std::int64_t>(am.floor_divide(bm.abs().add(1)), a.floor_divide(b.abs().add(1)), "floor_divide");
  require_equal_<std::int64_t>(am.remainder(bm.abs().add(1)), a.remainder(b.abs().add(1)), "remainder");
  require_equal_<std::int64_t>(am.fmod(bm.abs().add(1)), a.fmod(b.abs().add(1)), "fmod");
  // 16777217 is the first int32 that float32 cannot hold.
  const auto small = pluss::make_series(std::vector<std::int32_t>{16777217, 16777219}, {2});
  require_equal_<std::int32_t>(small.to(pluss::pMPS).add(small.to(pluss::pMPS)), small.add(small), "int32 add past 2^24");
}

TEST_CASE("MPS ops on a slice of a tensor the device just wrote read the device data", "[mps][view]") {
  if (!pluss::mps::is_ready()) { SUCCEED("MPS runtime unavailable"); return; }
  const auto a = pluss::make_series(std::vector<float>{1, 2, 3, 4, 5, 6}, {6}).to(pluss::pMPS);
  const auto b = pluss::make_series(std::vector<float>{10, 20, 30, 40, 50, 60}, {6}).to(pluss::pMPS);
  const auto written = a.add(b);                       // device-written, never read back
  const auto doubled = written.slice(0, 2, 5).mul(2.0); // a view with a storage offset
  require_close_f_(doubled, pluss::make_series(std::vector<float>{66, 88, 110}, {3}), 0.0f, "slice of device-written");
}

TEST_CASE("MPS integer matmul, large-exponent pow, fractional clamp and division by zero", "[mps][integer][exact]") {
  if (!pluss::mps::is_ready()) { SUCCEED("MPS runtime unavailable"); return; }
  // int64 matmul with products far beyond float32's exact range.
  const auto a = pluss::make_series(std::vector<std::int64_t>{5000000001LL, 3, -7, 100000000003LL, 2, 9}, {2, 3});
  const auto b = pluss::make_series(std::vector<std::int64_t>{4, 5000000007LL, 6, -8, 1000003, 2}, {3, 2});
  require_equal_<std::int64_t>(a.to(pluss::pMPS).matmul(b.to(pluss::pMPS)), a.matmul(b), "int64 matmul");

  // The exponent bits are read with integer arithmetic, so an exponent past 2^24 is exact.
  const auto base = pluss::make_series(std::vector<std::int64_t>{1, -1, 2}, {3});
  const auto exponent = pluss::make_series(std::vector<std::int64_t>{16777217, 16777217, 40}, {3});
  require_equal_<std::int64_t>(base.to(pluss::pMPS).pow(exponent.to(pluss::pMPS)), base.pow(exponent), "pow exponent >= 2^24");

  // An integer tensor takes a fractional bound truncated toward zero, as on the CPU.
  const auto ints = pluss::make_series(std::vector<std::int64_t>{-3, 0, 1, 2, 5}, {5});
  const auto ints_mps = ints.to(pluss::pMPS);
  require_equal_<std::int64_t>(ints_mps.clamp_min(0.5), ints.clamp_min(0.5), "clamp_min 0.5");
  require_equal_<std::int64_t>(ints_mps.clamp_max(1.5), ints.clamp_max(1.5), "clamp_max 1.5");
  require_equal_<std::int64_t>(ints_mps.clamp(0.5, 2.5), ints.clamp(0.5, 2.5), "clamp(0.5, 2.5)");

  // Integer division by zero follows IEEE: floor(x / 0) = +-inf, 0 // 0 and any x % 0 are NaN. Integers cannot hold those,
  // so a division with a zero divisor comes back in floating point; one without a zero stays integer.
  const auto numerator = pluss::make_series(std::vector<std::int64_t>{7, -7, 0, 5}, {4});
  const auto divisor = pluss::make_series(std::vector<std::int64_t>{0, 0, 0, 3}, {4});
  const auto as_doubles = [](const pluss::Series& x) {
    return x.to(pluss::pCPU).to(pluss::ScalarType::Float64).flatten(0, -1).items<double>();
  };
  for (const auto platform : {pluss::pCPU, pluss::pMPS}) {
    const auto n = numerator.to(platform);
    const auto d = divisor.to(platform);
    const auto q = as_doubles(n.floor_divide(d));
    REQUIRE((std::isinf(q[0]) && q[0] > 0));
    REQUIRE((std::isinf(q[1]) && q[1] < 0));
    REQUIRE(std::isnan(q[2]));
    REQUIRE(q[3] == 1.0);
    const auto r = as_doubles(n.remainder(d));
    REQUIRE((std::isnan(r[0]) && std::isnan(r[1]) && std::isnan(r[2])));
    REQUIRE(r[3] == 2.0);
    const auto f = as_doubles(n.fmod(d));
    REQUIRE((std::isnan(f[0]) && std::isnan(f[1]) && std::isnan(f[2])));
    REQUIRE(f[3] == 2.0);
  }
  // An integer or a boolean is always finite: isfinite answers "yes" everywhere (it used to raise), and nothing is NaN or infinite.
  const auto whole = pluss::make_series(std::vector<std::int64_t>{1, -2, 3}, {3});
  for (const auto platform : {pluss::pCPU, pluss::pMPS}) {
    const auto w = whole.to(platform);
    REQUIRE(w.isfinite().to(pluss::pCPU).host<bool>() == std::vector<bool>({true, true, true}));
    REQUIRE(w.isnan().to(pluss::pCPU).host<bool>() == std::vector<bool>({false, false, false}));
    REQUIRE(w.isinf().to(pluss::pCPU).host<bool>() == std::vector<bool>({false, false, false}));
  }
  // With no zero divisor the result stays integer.
  REQUIRE(numerator.floor_divide(pluss::make_series(std::vector<std::int64_t>{2, 2, 2, 2}, {4})).scalar_type() ==
          pluss::ScalarType::Int64);
}

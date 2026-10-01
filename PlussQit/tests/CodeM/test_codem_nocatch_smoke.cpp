#include <pluss/pluss.h>
#include <pluss/tils.h>
#include "CodeM/core/Storage.h"
#include "CodeM/compat/Invoke.h"

using pluss::codem::Series;
using pluss::codem::ScalarType;


#include <array>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace {

int g_failures = 0;

void expect_true(bool cond, const std::string& msg) {
  if (!cond) {
    ++g_failures;
    std::cerr << "[FAIL] " << msg << "\n";
  }
}

template <class T>
void expect_eq(const std::vector<T>& got, const std::vector<T>& exp, const std::string& msg) {
  if (got != exp) {
    ++g_failures;
    std::cerr << "[FAIL] " << msg << " (size=" << got.size() << " expected=" << exp.size() << ")\n";
  }
}

void expect_close(double got, double exp, double tol, const std::string& msg) {
  if (std::abs(got - exp) > tol) {
    ++g_failures;
    std::cerr << "[FAIL] " << msg << " (got=" << got << " exp=" << exp << " tol=" << tol << ")\n";
  }
}

void expect_throws(const std::function<void()>& fn, const std::string& msg) {
  bool threw = false;
  try {
    fn();
  } catch (...) {
    threw = true;
  }
  expect_true(threw, msg);
}

static_assert(std::is_default_constructible_v<pluss::Series>,
              "A default-constructed Series must be an undefined handle.");
static_assert(!std::is_default_constructible_v<pluss::Note>,
              "Note must not be default-constructible by end users.");
static_assert(!std::is_default_constructible_v<pluss::Timeline>,
              "Timeline must not be default-constructible by end users.");
static_assert(!std::is_default_constructible_v<pluss::Duration>,
              "Duration must not be default-constructible by end users.");
static_assert(!std::is_default_constructible_v<pluss::Timeline::TimelineIndexAccessor>,
              "TimelineIndexAccessor must not be default-constructible by end users.");
static_assert(!std::is_constructible_v<pluss::Series, std::initializer_list<double>>,
              "Series must not be direct-list-constructible by end users.");
static_assert(!std::is_constructible_v<pluss::Series, std::vector<double>>,
              "Series must not be direct-constructible from host containers.");

class ScopedEnvVar final {
 public:
  ScopedEnvVar(const char* key, const char* value) : key_(key) {
    if (const char* existing = std::getenv(key_); existing != nullptr) {
      had_previous_ = true;
      previous_ = existing;
    }
    if (value) {
      ::setenv(key_, value, 1);
    } else {
      ::unsetenv(key_);
    }
  }

  ~ScopedEnvVar() {
    if (had_previous_) {
      ::setenv(key_, previous_.c_str(), 1);
    } else {
      ::unsetenv(key_);
    }
  }

 private:
  const char* key_;
  bool had_previous_{false};
  std::string previous_;
};

void test_factories() {
  std::cout << "[RUN] factories:value\n";
  auto s1 = pluss::make_series({1.0, 2.0, 3.0});
  expect_true(s1.numel() == 3, "value(init_list) works");
  expect_true(s1.platform() == pluss::Platform::CPU, "series.platform() getter reports CPU");

  std::ostringstream stream_dtype;
  stream_dtype << s1.dtype();
  expect_true(stream_dtype.str() == "float64", "dtype stream operator prints dtype name");

  std::ostringstream stream_shape;
  stream_shape << s1.sizes();
  expect_true(stream_shape.str() == "[3]", "sizes stream operator prints bracketed shape");

  std::ostringstream stream_platform;
  stream_platform << s1.platform();
  expect_true(stream_platform.str() == "cpu", "platform stream operator prints backend name");

  expect_throws(
      [&] {
        (void)pluss::codem::_compat::invoke("__definitely_unrouted__", s1);
      },
      "unrouted CPU compat op fails instead of silently returning its receiver");

  std::cout << "[RUN] factories:to_platform\n";
  auto s1_cpu = s1.to(pluss::Platform::CPU);
  expect_true(s1_cpu.platform() == pluss::Platform::CPU, "to(Platform::CPU) keeps CPU backend");

  std::cout << "[RUN] factories:to_platform_dtype\n";
  auto s1_f32 = s1.to(pluss::Platform::CPU, pluss::ScalarType::Float32);
  expect_true(s1_f32.scalar_type() == pluss::ScalarType::Float32,
              "to(Platform, dtype) applies dtype conversion");

#if defined(__APPLE__)
  if (pluss::codem::mps_backend_available()) {
  std::cout << "[RUN] factories:to_mps_roundtrip\n";
  auto s1_mps = s1.to(pluss::Platform::MPS);
  expect_true(s1_mps.platform() == pluss::Platform::MPS, "to(MPS) marks MPS backend");
  expect_eq(s1_mps.to_vec<double>(), s1.to_vec<double>(), "to(MPS) preserves values");

  std::cout << "[RUN] factories:create_on_mps\n";
  auto z_mps = pluss::series::zeros({2, 2}, pluss::cfg::platform(pluss::Platform::MPS));
  expect_true(z_mps.platform() == pluss::Platform::MPS, "factory config platform creates MPS series");
  expect_eq(z_mps.to_vec<float>(), std::vector<float>{0.0f, 0.0f, 0.0f, 0.0f}, "factory-created MPS zeros values");

  auto s1_mps_f32 = s1_mps.to(pluss::ScalarType::Float32);
  expect_true(s1_mps_f32.platform() == pluss::Platform::MPS, "dtype cast preserves MPS platform");
  expect_true(s1_mps_f32.scalar_type() == pluss::ScalarType::Float32, "dtype cast on MPS updates dtype");

  const std::array<std::int64_t, 1> reshaped_sizes{3};
  auto s1_mps_reshaped = s1_mps.reshape(std::span<const std::int64_t>(reshaped_sizes.data(), reshaped_sizes.size()));
  expect_true(s1_mps_reshaped.platform() == pluss::Platform::MPS, "view-like ops preserve MPS platform");

  auto s1_mps_back = s1_mps.to(pluss::Platform::CPU);
  expect_true(s1_mps_back.platform() == pluss::Platform::CPU, "MPS to CPU transfer works");
  expect_eq(s1_mps_back.to_vec<double>(), s1.to_vec<double>(), "MPS to CPU roundtrip preserves values");

  auto s1_mps_add = s1_mps.add(2.0);
  expect_true(s1_mps_add.platform() == pluss::Platform::MPS, "ops on MPS stay on MPS");

  auto s1_mps_chain = s1_mps.exp().log().square();
  expect_true(s1_mps_chain.platform() == pluss::Platform::MPS,
              "platform sticks across chained elementwise ops on MPS");

  auto s1_mps_via_to = s1.to(pluss::Platform::MPS);
  expect_true(s1_mps_via_to.platform() == pluss::Platform::MPS,
              "to(target) moves Series to target backend");
  expect_eq(s1_mps_via_to.to_vec<double>(), s1_mps.to_vec<double>(),
            "two to(target) calls produce equivalent values");

  expect_throws(
      [&] {
        (void)s1_mps.add(s1);
      },
      "mixed-platform arithmetic throws");

  expect_throws(
      [&] {
        auto cond = s1_mps.gt(0.0);
        (void)s1_mps.where(cond.to(pluss::Platform::CPU), s1_mps);
      },
      "mixed-platform where throws");

  expect_throws(
      [&] {
        (void)pluss::codem::_compat::invoke("__definitely_unrouted__", s1_mps);
      },
      "unrouted compat op on MPS throws instead of silently falling back");
  }
#endif

  std::cout << "[RUN] factories:make_series\n";
  std::vector<float> raw{1.0f, 2.0f, 3.0f, 4.0f};
  auto s2 = pluss::make_series(raw, {2, 2});
  expect_true(s2.numel() == 4, "make_series(lvalue data) works");

  std::cout << "[RUN] factories:make_series_integral_promotion\n";
  std::vector<int> ints{1, 2, 3, 4};
  auto s3 = pluss::make_series(ints, {2, 2});
  expect_true(s3.scalar_type() == pluss::ScalarType::Int32,
              "make_series preserves the std::vector<int> element width");

  std::cout << "[RUN] factories:rand\n";
  std::array<std::int64_t, 1> sizes{4};
  auto span = std::span<const std::int64_t>(sizes.data(), sizes.size());
  auto r0 = pluss::series::rand(span, pluss::ScalarType::Float32, 42);
  auto r1 = pluss::series::rand(span, pluss::ScalarType::Float32, 42);
  std::cout << "[RUN] factories:to_vec\n";
  expect_true(r0.to_vec<float>() == r1.to_vec<float>(), "rand seed reproducibility");

  std::cout << "[RUN] factories:size_mismatch_throws\n";
  expect_throws(
      [] {
        std::vector<float> bad{1.0f, 2.0f, 3.0f};
        (void)pluss::make_series(bad, {2, 2});
      },
      "make_series should throw on size mismatch");

  std::cout << "[RUN] factories:expanded\n";
  std::cout << "[RUN] factories:logspace\n";
  auto ls = pluss::series::logspace(0.0, 2.0, 3, 10.0, pluss::ScalarType::Float64);
  expect_true(ls.numel() == 3, "logspace works");
  std::cout << "[RUN] factories:randperm\n";
  auto rp = pluss::series::randperm(8, pluss::ScalarType::Int64, 123);
  expect_true(rp.numel() == 8, "randperm works");
  std::cout << "[RUN] factories:bernoulli\n";
  auto b01 = pluss::series::bernoulli({16}, 0.5, pluss::ScalarType::Bool, 7);
  expect_true(b01.numel() == 16, "bernoulli factory works");
  std::cout << "[RUN] factories:zeros_like\n";
  auto zlike = pluss::series::zeros_like(s2);
  expect_true(zlike.scalar_type() == s2.scalar_type(), "zeros_like inherits dtype");
  std::cout << "[RUN] factories:value_literal\n";
  auto t = pluss::make_series({1, 2, 3});
  expect_true(t.numel() == 3, "value literal works");

  std::cout << "[RUN] factories:series_print_default_policy\n";
  auto pretty_src = pluss::make_series({{0.24644905, 0.23562264}});
  const std::string pretty_default = pluss::series::print(pretty_src);
  expect_true(pretty_default.find("0.2464") != std::string::npos,
              "series::print default uses 4dp for normal magnitudes");

  pluss::PrintOptions pretty_more;
  pretty_more.number_mode = pluss::codem::PrintNumberMode::Fixed;
  pretty_more.precision = 6;
  const std::string pretty_override = pluss::series::print(pretty_src, pretty_more);
  expect_true(pretty_override.find("0.246449") != std::string::npos,
              "series::print allows user precision override");

  auto tiny_round = pluss::make_series({{0.000076, 1.0}});
  const std::string tiny_round_s = pluss::series::print(tiny_round);
  expect_true(tiny_round_s.find("0.0001") != std::string::npos && tiny_round_s.find("e-") == std::string::npos,
              "series::print keeps fixed when tiny values still survive rounding");

  auto tiny_scientific = pluss::make_series({{0.000023, 1.0}});
  const std::string tiny_scientific_s = pluss::series::print(tiny_scientific);
  expect_true(tiny_scientific_s.find("e-") != std::string::npos,
              "series::print switches to scientific when fixed would round nonzero to zero");

  auto wide = pluss::make_series({{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0}});
  const std::string wide_default = pluss::series::print(wide);
  expect_true(wide_default.find("5.0000") != std::string::npos &&
                  wide_default.find("6.0000") == std::string::npos &&
                  wide_default.find("...") != std::string::npos,
              "default summary keeps first five then tail one for wide matrices");

  pluss::PrintOptions wider_precision;
  wider_precision.precision = 6;
  const std::string wide_user_precision = pluss::series::print(wide, wider_precision);
  expect_true(wide_user_precision.find("2.000000") != std::string::npos &&
                  wide_user_precision.find("3.000000") == std::string::npos &&
                  wide_user_precision.find("...") != std::string::npos,
              "precision >4 keeps legacy head-two summary unless user overrides summary fields");
}

void test_aux_let_transmit() {
  std::cout << "[RUN] aux:let\n";
  auto lv = suppose(std::vector<int>{1, 2, 3})
                .then([](const std::vector<int>& x) {
                  std::vector<int> y = x;
                  for (auto& v : y) v *= 2;
                  return y;
                })
                .tap([](const std::vector<int>& x) {
                  if (x.size() != 3) {
                    throw std::runtime_error("let tap: unexpected size");
                  }
                })
                .value();
  expect_eq(lv, std::vector<int>{2, 4, 6}, "let pipeline");

  auto gv = suppose(5).then([](int x) { return x + 3; }).value();
  expect_true(gv == 8, "global suppose alias");

  auto la = suppose.as<double>(3).then([](double x) { return x + 0.5; }).value();
  expect_close(la, 3.5, 1e-12, "suppose typed conversion");

  auto declared = pluss::make_series({1.0, 2.0, 3.0});
  expect_true(declared.numel() == 3, "let declaration works for Series");

  std::cout << "[RUN] aux:transmit_range\n";
  std::vector<int> xs{1, 2, 3, 4};
  auto sq = pluss::tils::transmit([](int x) { return x * x; }, xs);
  expect_eq(sq, std::vector<int>{1, 4, 9, 16}, "transmit range unary");

  std::cout << "[RUN] aux:transmit_indexed\n";
  auto with_idx = pluss::tils::transmit([](int x, std::size_t i) { return x + static_cast<int>(i); }, xs);
  expect_eq(with_idx, std::vector<int>{1, 3, 5, 7}, "transmit indexed callable");

  std::cout << "[RUN] aux:transmit_iterators\n";
  auto it_res = pluss::tils::transmit([](int x) { return x - 1; }, xs.begin(), xs.end());
  expect_eq(it_res, std::vector<int>{0, 1, 2, 3}, "transmit iterator overload");

  std::cout << "[RUN] aux:transmit_binary\n";
  std::vector<int> ys{10, 20, 30, 40};
  auto sum = pluss::tils::transmit([](int a, int b) { return a + b; }, xs, ys);
  expect_eq(sum, std::vector<int>{11, 22, 33, 44}, "transmit binary zip");

  std::cout << "[RUN] aux:transmit_parallel\n";
  auto par = pluss::tils::transmit(
      [](int x) { return x * 3; },
      xs,
      pluss::tils::transmit_options{
          .exec = pluss::tils::transmit_exec::par, .grain_size = 1, .workers = 2});
  expect_eq(par, std::vector<int>{3, 6, 9, 12}, "transmit parallel");

  std::cout << "[RUN] aux:transmit_object\n";
  struct Item {
    int v;
  };
  struct Bump {
    int delta;
    Item operator()(const Item& it) const { return Item{it.v + delta}; }
  };
  std::vector<Item> items{{1}, {2}, {3}};
  auto bumped = pluss::tils::transmit(Bump{5}, items);
  expect_true(bumped.size() == 3 && bumped[0].v == 6 && bumped[2].v == 8, "transmit callable object");

  std::cout << "[RUN] aux:transmit_series\n";
  auto s = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3});
  auto ss = pluss::tils::transmit([](const pluss::Series& t) { return t.mul(t); }, s);
  expect_eq(ss.to_vec<double>(), std::vector<double>{1.0, 4.0, 9.0}, "transmit single-object apply");
}

void test_arithmetic_and_reduce() {
  auto a = pluss::Series::full({2,2}, 2.0, pluss::ScalarType::Float32);
  auto c = a.add(3.0);
  expect_eq(c.to_vec<float>(), {5.0f, 5.0f, 5.0f, 5.0f}, "add scalar");

  auto d = c.mul(2.0);
  expect_eq(d.to_vec<float>(), {10.0f, 10.0f, 10.0f, 10.0f}, "mul scalar");

  auto e = d.sub(4.0).div(3.0);
  expect_eq(e.to_vec<float>(), {2.0f, 2.0f, 2.0f, 2.0f}, "sub/div scalar chain");

  auto m = d.mean();
  expect_eq(m.to_vec<float>(), {10.0f}, "mean scalar value");
}

void test_indexing_ops() {
  auto src = pluss::make_series(std::vector<std::int64_t>{10, 20, 30, 40}, {2, 2});
  auto idx = pluss::make_series(
      std::vector<std::int64_t>{1, 0, 0, 1},
      {2, 2},
      pluss::Config{}.with_dtype(pluss::ScalarType::Int64));

  auto g = src.gather(1, idx);
  expect_eq(g.to_vec<std::int64_t>(), {20, 10, 30, 40}, "gather basic");

  auto dst = pluss::make_series(std::vector<std::int64_t>{0, 0, 0, 0}, {2, 2});
  auto sc = dst.scatter(1, idx, src);
  expect_eq(sc.to_vec<std::int64_t>(), {20, 10, 30, 40}, "scatter basic");

  auto idx_neg = pluss::make_series(
      std::vector<std::int64_t>{-1, 0, 0, -1},
      {2, 2},
      pluss::Config{}.with_dtype(pluss::ScalarType::Int64));
  auto g_neg = src.gather(1, idx_neg);
  expect_eq(g_neg.to_vec<std::int64_t>(), {20, 10, 30, 40}, "gather negative index");

  auto sc_neg = dst.scatter(1, idx_neg, src);
  expect_eq(sc_neg.to_vec<std::int64_t>(), {20, 10, 30, 40}, "scatter negative index");

  auto idx_bad = pluss::make_series(
      std::vector<std::int64_t>{2, 0, 0, 1},
      {2, 2},
      pluss::Config{}.with_dtype(pluss::ScalarType::Int64));
  expect_throws([&] { (void)src.gather(1, idx_bad); }, "gather out-of-range throws");
  expect_throws([&] { (void)dst.scatter(1, idx_bad, src); }, "scatter out-of-range throws");
}

void test_unary_math_and_activations() {
  auto x = pluss::make_series(std::vector<float>{-2.0f, -1.0f, 0.0f, 1.0f}, {4});

  auto ex = x.exp();
  auto lg = ex.log();
  expect_true(lg.numel() == 4, "exp/log pipeline");

  auto s = x.sigmoid();
  expect_true(s.scalar_type() == pluss::ScalarType::Float32, "sigmoid dtype");

  auto r = x.relu();
  expect_eq(r.to_vec<float>(), {0.0f, 0.0f, 0.0f, 1.0f}, "relu values");
}

void test_autodiff_transforms() {
  using pluss::subsp::math::diff::DualTrace;
  using pluss::subsp::math::diff::Trace;
  using pluss::subsp::math::diff::jvp;
  using pluss::subsp::math::diff::linearize;
  using pluss::subsp::math::diff::value_and_grad;
  using pluss::subsp::math::diff::vjp;

  std::cout << "[RUN] autodiff:reverse\n";
  auto scalar = pluss::make_series(1.25).to(pluss::ScalarType::Float64);
  scalar.tracks_slope_(true);
  auto scalar_loss = scalar.square().sum();
  scalar_loss.reverse();
  expect_close(scalar.sensitivity().item<double>(), 2.5, 1e-12, "reverse scalar sensitivity");

  const auto vector_objective = [](const auto& x) {
    using pluss::subsp::math::diff::sin;
    using pluss::subsp::math::diff::square;
    return sin(x) + square(x);
  };

  std::cout << "[RUN] autodiff:value_and_grad\n";
  const auto x = pluss::make_series(std::vector<double>{0.3, -0.2, 0.5}, {3});
  auto [value, g] = value_and_grad(
      [&](const Trace& z) -> Trace {
        using pluss::subsp::math::diff::exp;
        using pluss::subsp::math::diff::log;
        using pluss::subsp::math::diff::mean;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        return mean(square(z)) + sum(log(exp(z) + 1.0));
      },
      x);
  expect_true(value.numel() == 1, "value_and_grad scalar output");
  const auto gv = g.to_vec<double>();
  const auto xv = x.to_vec<double>();
  for (std::size_t i = 0; i < xv.size(); ++i) {
    const double expected = (2.0 * xv[i] / static_cast<double>(xv.size())) + 1.0 / (1.0 + std::exp(-xv[i]));
    expect_close(gv[i], expected, 1e-9, "value_and_grad analytic component");
  }

  std::cout << "[RUN] autodiff:vjp\n";
  const auto cotangent = pluss::make_series(std::vector<double>{1.0, -0.5, 0.25}, {3});
  auto [vjp_value, pullback] = vjp(
      [&](const Trace& z) -> Trace { return vector_objective(z); },
      x);
  expect_true(vjp_value.numel() == 3, "vjp primal shape");
  const auto pulled = pullback(cotangent).to_vec<double>();
  const auto cv = cotangent.to_vec<double>();
  for (std::size_t i = 0; i < xv.size(); ++i) {
    const double expected = (std::cos(xv[i]) + 2.0 * xv[i]) * cv[i];
    expect_close(pulled[i], expected, 1e-12, "vjp analytic pullback");
  }

  std::cout << "[RUN] autodiff:jvp\n";
  const auto tangent = pluss::make_series(std::vector<double>{0.1, 0.05, -0.2}, {3});
  auto [jvp_value, directional] = jvp(
      [&](const DualTrace& z) -> DualTrace { return vector_objective(z); },
      x,
      tangent);
  expect_true(jvp_value.numel() == 3, "jvp primal shape");
  const auto tv = tangent.to_vec<double>();
  const auto dv = directional.to_vec<double>();
  for (std::size_t i = 0; i < xv.size(); ++i) {
    const double expected = (std::cos(xv[i]) + 2.0 * xv[i]) * tv[i];
    expect_close(dv[i], expected, 1e-12, "jvp analytic pushforward");
  }

  std::cout << "[RUN] autodiff:linearize\n";
  auto [lin_value, pushforward] = linearize(
      [&](const DualTrace& z) -> DualTrace { return vector_objective(z); },
      x);
  expect_true(lin_value.numel() == 3, "linearize primal shape");
  const auto tangent_b = pluss::make_series(std::vector<double>{-0.3, 0.2, 0.15}, {3});
  const auto pb = pushforward(tangent_b).to_vec<double>();
  const auto tb = tangent_b.to_vec<double>();
  for (std::size_t i = 0; i < xv.size(); ++i) {
    const double expected = (std::cos(xv[i]) + 2.0 * xv[i]) * tb[i];
    expect_close(pb[i], expected, 1e-12, "linearize reusable pushforward");
  }
}

void test_storage_pool_robustness() {
  const char* env_key = "PLUSS_CODEM_STORAGE_POOL";

  std::cout << "[RUN] storage_pool:toggle\n";
  ScopedEnvVar restore(env_key, "1");

  void* pooled_first = nullptr;
  {
    pluss::codem::Storage s(4096, 64);
    pooled_first = s.data();
  }
  void* pooled_second = nullptr;
  {
    pluss::codem::Storage s(4096, 64);
    pooled_second = s.data();
  }
  expect_true(pooled_first != nullptr && pooled_second != nullptr, "storage pool returns non-null blocks");
  expect_true(pooled_first == pooled_second, "storage pool reuses same-size block when enabled");

  {
    ScopedEnvVar disable(env_key, "off");
    void* disabled_ptr = nullptr;
    {
      pluss::codem::Storage s(4096, 64);
      disabled_ptr = s.data();
    }
    expect_true(disabled_ptr != nullptr, "storage allocation works when pool disabled");
    expect_true((reinterpret_cast<std::uintptr_t>(disabled_ptr) % 64) == 0, "disabled-path allocation alignment");
  }

  // Re-enabling should continue to use the previously cached pooled block.
  void* pooled_third = nullptr;
  {
    pluss::codem::Storage s(4096, 64);
    pooled_third = s.data();
  }
  expect_true(pooled_third == pooled_first, "pool state restored after temporary disable");

  std::cout << "[RUN] storage_pool:stress\n";
  bool stress_ok = true;
  for (std::size_t i = 0; i < 20000; ++i) {
    const std::size_t bytes = ((i % 97) + 1) * 64;
    pluss::codem::Storage s(bytes, 64);
    auto* p = s.data();
    if (!p) {
      stress_ok = false;
      break;
    }
    p[0] = std::byte{static_cast<unsigned char>(i & 0xFFu)};
    p[bytes - 1] = std::byte{static_cast<unsigned char>((i >> 8) & 0xFFu)};
  }
  expect_true(stress_ok, "storage pool stress allocation loop");
}

void test_dispatch_extensions() {
  std::cout << "[RUN] dispatch_extensions:repeat\n";
  auto x = pluss::make_series(std::vector<std::int64_t>{1, 2, 3}, {3});
  auto xr = x.repeat({2});
  expect_eq(xr.to_vec<std::int64_t>(), {1, 2, 3, 1, 2, 3}, "repeat intlist");

  std::cout << "[RUN] dispatch_extensions:repeat_like\n";
  auto like_rep = pluss::series::rand({2, 6}, pluss::ScalarType::Float32, 7);
  auto x_like = x.repeat_like(like_rep);
  expect_true(x_like.sizes().size() == 2 && x_like.sizes()[0] == 2 && x_like.sizes()[1] == 6,
              "repeat_like target shape");
  expect_true(x_like.scalar_type() == pluss::ScalarType::Int64,
              "repeat_like keeps source dtype for mixed int/float inputs");
  expect_eq(x_like.to_vec<std::int64_t>(), {1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3}, "repeat_like values");

  std::cout << "[RUN] dispatch_extensions:expand_like\n";
  auto one = pluss::make_series(std::vector<std::int64_t>{9}, {1, 1});
  auto one_like = one.expand_like(like_rep);
  expect_true(one_like.sizes().size() == 2 && one_like.sizes()[0] == 2 && one_like.sizes()[1] == 6,
              "expand_like target shape");
  expect_true(one_like.scalar_type() == pluss::ScalarType::Int64,
              "expand_like keeps source dtype for mixed int/float inputs");
  expect_eq(one_like.to_vec<std::int64_t>(), {9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9}, "expand_like values");

  std::cout << "[RUN] dispatch_extensions:repeat_like_incompatible\n";
  auto bad_like = pluss::series::rand({2, 5}, pluss::ScalarType::Float32, 8);
  expect_throws([&] { (void)x.repeat_like(bad_like); }, "repeat_like throws for non-multiple target dimension");

  std::cout << "[RUN] dispatch_extensions:repeat_interleave\n";
  auto xi = x.repeat_interleave(2);
  expect_eq(xi.to_vec<std::int64_t>(), {1, 1, 2, 2, 3, 3}, "repeat_interleave flat");

  std::cout << "[RUN] dispatch_extensions:roll_flat\n";
  auto xr1 = x.roll(1);
  expect_eq(xr1.to_vec<std::int64_t>(), {3, 1, 2}, "roll flat");

  std::cout << "[RUN] dispatch_extensions:roll_dim\n";
  auto m = pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 4}, {2, 2});
  auto mr = m.roll(1, 1);
  expect_eq(mr.to_vec<std::int64_t>(), {2, 1, 4, 3}, "roll dim");

  std::cout << "[RUN] dispatch_extensions:masked_fill\n";
  auto mask = pluss::Series::ones({2, 2}, pluss::ScalarType::Bool);
  auto mf = m.masked_fill(mask, 9.0);
  expect_eq(mf.to_vec<std::int64_t>(), {9, 9, 9, 9}, "masked_fill scalar");

  auto m2 = m.clone();
  m2.masked_fill_(mask, 7.0);
  expect_eq(m2.to_vec<std::int64_t>(), {7, 7, 7, 7}, "masked_fill_ scalar");

  std::cout << "[RUN] dispatch_extensions:scatter_reduce\n";
  auto base = pluss::make_series(std::vector<std::int64_t>{0, 0, 0, 0}, {4});
  auto idx = pluss::make_series(
      std::vector<std::int64_t>{1, 1, 2},
      {3},
      pluss::Config{}.with_dtype(pluss::ScalarType::Int64));
  auto src = pluss::make_series(std::vector<std::int64_t>{3, 4, 5}, {3});
  auto sr = base.scatter_reduce(0, idx, src, "sum", true);
  expect_eq(sr.to_vec<std::int64_t>(), {0, 7, 5, 0}, "scatter_reduce sum");

  std::cout << "[RUN] dispatch_extensions:index_add_\n";
  auto t = pluss::make_series(std::vector<std::int64_t>{10, 20, 30, 40}, {4});
  t.index_add_(
      0, idx.select(0, 0).unsqueeze(0), src.select(0, 0).unsqueeze(0));
  expect_eq(t.to_vec<std::int64_t>(), {10, 23, 30, 40}, "index_add_ 1d dim0");

  std::cout << "[RUN] dispatch_extensions:sum_to_size\n";
  auto s2 = pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 4, 5, 6}, {2, 3});
  auto s2r = s2.sum_to_size({2, 1});
  expect_eq(s2r.to_vec<std::int64_t>(), {6, 15}, "sum_to_size reduce");

  std::cout << "[RUN] dispatch_extensions:unflatten_unfold\n";
  auto u = pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 4, 5, 6}, {6});
  auto uf = u.unflatten(0, {2, 3});
  expect_true(uf.sizes().size() == 2 && uf.sizes()[0] == 2 && uf.sizes()[1] == 3, "unflatten shape");
  auto uw = u.unfold(0, 3, 1);
  expect_true(uw.numel() == 12, "unfold numel");

  std::cout << "[RUN] dispatch_extensions:inplace_new_routes\n";
  auto inx = pluss::make_series(std::vector<double>{0.25, 0.75}, {2});
  inx.logit_(1e-6);
  auto inv = inx.to_vec<double>();
  expect_true(inv[0] < 0.0 && inv[1] > 0.0, "logit_ sign behavior");

  auto nx = pluss::make_series(std::vector<double>{1.0, 2.0}, {2});
  nx.nextafter_(pluss::make_series(3.0));
  auto nxv = nx.to_vec<double>();
  expect_true(nxv[0] > 1.0 && nxv[1] > 2.0, "nextafter_ toward scalar");

  auto bx = pluss::make_series(std::vector<std::int64_t>{1, 2, 3}, {3});
  bx.bitwise_left_shift_(pluss::make_series(1LL));
  expect_eq(bx.to_vec<std::int64_t>(), {2, 4, 6}, "bitwise_left_shift_ scalar");
  bx.bitwise_right_shift_(pluss::make_series(1LL));
  expect_eq(bx.to_vec<std::int64_t>(), {1, 2, 3}, "bitwise_right_shift_ scalar");

  auto rx = pluss::Series::zeros({64}, pluss::ScalarType::Int64);
  rx.random_(3, 9);
  auto rv = rx.to_vec<std::int64_t>();
  bool in_range = true;
  for (auto v : rv) {
    if (v < 3 || v >= 9) {
      in_range = false;
      break;
    }
  }
  expect_true(in_range, "random_ range [from, to)");

  std::cout << "[RUN] dispatch_extensions:new_math_routes\n";
  std::cout << "[RUN] dispatch_extensions:new_math_routes:heaviside\n";
  auto hx = pluss::make_series(std::vector<double>{-1.0, 0.0, 2.0}, {3});
  auto hv = hx.heaviside(0.25);
  expect_eq(hv.to_vec<double>(), {0.0, 0.25, 1.0}, "heaviside scalar");

  std::cout << "[RUN] dispatch_extensions:new_math_routes:sinc\n";
  auto sx = pluss::make_series(std::vector<double>{0.0, 1.0}, {2});
  auto sv = sx.sinc();
  auto svv = sv.to_vec<double>();
  expect_close(svv[0], 1.0, 1e-12, "sinc(0)");
  expect_true(std::abs(svv[1]) < 1e-9, "sinc(1) near zero");

  std::cout << "[RUN] dispatch_extensions:new_math_routes:gcd_lcm\n";
  auto gx = pluss::make_series(std::vector<std::int64_t>{12, 18}, {2});
  auto gv = gx.gcd(pluss::make_series(std::int64_t{6}));
  expect_eq(gv.to_vec<std::int64_t>(), {6, 6}, "gcd scalar");
  auto lv = gx.lcm(pluss::make_series(std::vector<std::int64_t>{6, 8}, {2}));
  expect_eq(lv.to_vec<std::int64_t>(), {12, 72}, "lcm tensor");

  std::cout << "[RUN] dispatch_extensions:new_math_routes:hardshrink\n";
  auto hs = pluss::make_series(std::vector<double>{-0.2, 0.7}, {2}).hardshrink(0.5);
  expect_eq(hs.to_vec<double>(), {0.0, 0.7}, "hardshrink scalar lambda");

  std::cout << "[RUN] dispatch_extensions:new_math_routes:cross\n";
  auto c1 = pluss::make_series(std::vector<double>{1.0, 0.0, 0.0}, {3});
  auto c2 = pluss::make_series(std::vector<double>{0.0, 1.0, 0.0}, {3});
  auto cv = c1.cross(c2);
  expect_eq(cv.to_vec<double>(), {0.0, 0.0, 1.0}, "cross 3-vector");

  std::cout << "[RUN] dispatch_extensions:new_math_routes:aminmax\n";
  auto am = pluss::make_series(std::vector<double>{3.0, -1.0, 2.0}, {3}).aminmax();
  expect_eq(am.first.to_vec<double>(), {-1.0}, "aminmax minimum");
  expect_eq(am.second.to_vec<double>(), {3.0}, "aminmax maximum");

  std::cout << "[RUN] dispatch_extensions:new_math_routes:logcumsumexp\n";
  auto lce = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0}, {3}).logcumsumexp();
  auto lcev = lce.to_vec<double>();
  expect_true(lcev[2] > lcev[1] && lcev[1] > lcev[0], "logcumsumexp increasing");
}

void test_matlab_hybrid_ops() {
  std::cout << "[RUN] matlab_hybrid:dct\n";
  auto x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4});
  auto xdct = x.dct();
  auto xidct = xdct.idct();
  expect_true(xidct.numel() == 4, "idct output shape");
  auto xrt = xidct.to_vec<double>();
  expect_close(xrt[0], 1.0, 1e-6, "dct/idct roundtrip[0]");
  expect_close(xrt[1], 2.0, 1e-6, "dct/idct roundtrip[1]");
  expect_close(xrt[2], 3.0, 1e-6, "dct/idct roundtrip[2]");
  expect_close(xrt[3], 4.0, 1e-6, "dct/idct roundtrip[3]");
  auto xdst = x.dst();
  auto xidst = xdst.idst();
  auto xri = xidst.to_vec<double>();
  expect_close(xri[0], 1.0, 1e-6, "dst/idst roundtrip[0]");
  expect_close(xri[1], 2.0, 1e-6, "dst/idst roundtrip[1]");
  expect_close(xri[2], 3.0, 1e-6, "dst/idst roundtrip[2]");
  expect_close(xri[3], 4.0, 1e-6, "dst/idst roundtrip[3]");

  std::cout << "[RUN] matlab_hybrid:nanstats\n";
  auto xn = pluss::make_series(std::vector<double>{1.0, std::nan(""), 3.0, 5.0}, {4});
  auto nv = xn.stat().nanvar();
  auto ns = xn.stat().nanstd();
  expect_true(nv.numel() == 1, "nanvar scalar");
  expect_true(ns.numel() == 1, "nanstd scalar");

  std::cout << "[RUN] matlab_hybrid:movmean\n";
  auto mv = x.stat().movmean(2);
  expect_true(mv.numel() == 4, "movmean shape");

  std::cout << "[RUN] matlab_hybrid:cdist\n";
  auto a = pluss::make_series(std::vector<double>{0.0, 0.0, 1.0, 1.0}, {2, 2});
  auto b = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 2.0}, {2, 2});
  auto cd = a.stat(b).cdist();
  expect_true(cd.numel() == 4, "cdist shape");
  auto cdv = cd.to_vec<double>();
  expect_close(cdv[0], 1.0, 1e-6, "cdist[0,0]");
  expect_close(cdv[1], std::sqrt(8.0), 1e-6, "cdist[0,1]");
  expect_close(cdv[2], 1.0, 1e-6, "cdist[1,0]");
  expect_close(cdv[3], std::sqrt(2.0), 1e-6, "cdist[1,1]");

  std::cout << "[RUN] matlab_hybrid:hilbert\n";
  auto h = x.hilbert();
  expect_true(h.scalar_type() == pluss::ScalarType::Complex128, "hilbert dtype");
  expect_true(h.numel() == x.numel(), "hilbert shape");

  std::cout << "[RUN] matlab_hybrid:bessel\n";
  auto bj = x.besselj();
  expect_true(bj.numel() == 4, "besselj shape");

  std::cout << "[RUN] matlab_hybrid:beta\n";
  auto bt = x.beta(2.0);
  expect_true(bt.numel() == 4, "beta shape");

  std::cout << "[RUN] matlab_hybrid:erfcinv\n";
  auto ec = pluss::make_series(std::vector<double>{0.5}, {1}).erfcinv();
  expect_true(ec.numel() == 1, "erfcinv shape");
  auto ecv = ec.to_vec<double>();
  expect_true(std::isfinite(ecv[0]), "erfcinv finite");

  std::cout << "[RUN] matlab_hybrid:zeta\n";
  auto z = pluss::make_series(std::vector<double>{2.0}, {1}).zeta(1.0);
  expect_close(z.to_vec<double>()[0], 1.6449, 0.1, "zeta(2) approx");

  std::cout << "[RUN] matlab_hybrid:unwrap\n";
  auto ph = pluss::make_series(std::vector<double>{0.0, 3.5, 7.0}, {3});
  auto uw = ph.unwrap();
  expect_true(uw.numel() == 3, "unwrap shape");

  std::cout << "[RUN] matlab_hybrid:shift\n";
  auto sh = x.fftshift();
  auto ish = sh.ifftshift();
  expect_true(ish.numel() == 4, "fftshift/ifftshift shape");
}

} // namespace

int main() {
  try {
    std::cout << "[RUN] factories\n";
    test_factories();
    std::cout << "[RUN] aux\n";
    test_aux_let_transmit();
    std::cout << "[RUN] arith_reduce\n";
    test_arithmetic_and_reduce();
    std::cout << "[RUN] indexing_ops\n";
    test_indexing_ops();
    std::cout << "[RUN] unary_activations\n";
    test_unary_math_and_activations();
    std::cout << "[RUN] autodiff\n";
    test_autodiff_transforms();
    std::cout << "[RUN] storage_pool\n";
    test_storage_pool_robustness();
    std::cout << "[RUN] dispatch_extensions\n";
    test_dispatch_extensions();
    std::cout << "[RUN] matlab_hybrid\n";
    test_matlab_hybrid_ops();
  } catch (const std::exception& e) {
    std::cerr << "[EXCEPTION] " << e.what() << "\n";
    return 2;
  } catch (...) {
    std::cerr << "[EXCEPTION] unknown\n";
    return 3;
  }

  if (g_failures != 0) {
    std::cerr << "No-catch CodeM smoke tests failed: " << g_failures << "\n";
    return 1;
  }

  std::cout << "No-catch CodeM smoke tests passed\n";
  return 0;
}

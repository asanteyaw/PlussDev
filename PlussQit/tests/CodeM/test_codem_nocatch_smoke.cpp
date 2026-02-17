#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
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

void expect_throws(const std::function<void()>& fn, const std::string& msg) {
  bool threw = false;
  try {
    fn();
  } catch (...) {
    threw = true;
  }
  expect_true(threw, msg);
}

static_assert(!std::is_default_constructible_v<pluss::Series>,
              "Series must not be default-constructible by end users.");
static_assert(!std::is_constructible_v<pluss::Series, std::initializer_list<double>>,
              "Series must not be direct-list-constructible by end users.");
static_assert(!std::is_constructible_v<pluss::Series, std::vector<double>>,
              "Series must not be direct-constructible from host containers.");

void test_factories() {
  std::cout << "[RUN] factories:value\n";
  auto s1 = pluss::value({1.0, 2.0, 3.0});
  expect_true(s1.numel() == 3, "value(init_list) works");

  std::cout << "[RUN] factories:make_series\n";
  std::vector<float> raw{1.0f, 2.0f, 3.0f, 4.0f};
  auto s2 = pluss::make_series<float>(raw, {2,2});
  expect_true(s2.numel() == 4, "make_series(lvalue data) works");

  std::cout << "[RUN] factories:rand\n";
  std::array<std::int64_t, 1> sizes{4};
  auto span = std::span<const std::int64_t>(sizes.data(), sizes.size());
  auto r0 = pluss::rand(span, pluss::ScalarType::Float32, 42);
  auto r1 = pluss::rand(span, pluss::ScalarType::Float32, 42);
  std::cout << "[RUN] factories:to_vec\n";
  expect_true(r0.to_vec<float>() == r1.to_vec<float>(), "rand seed reproducibility");

  std::cout << "[RUN] factories:size_mismatch_throws\n";
  expect_throws(
      [] {
        std::vector<float> bad{1.0f, 2.0f, 3.0f};
        (void)pluss::make_series<float>(bad, {2, 2});
      },
      "make_series should throw on size mismatch");
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
  auto src = pluss::value(std::vector<std::int64_t>{10, 20, 30, 40}, {2, 2});
  auto idx = pluss::value(
      std::vector<std::int64_t>{1, 0, 0, 1},
      {2, 2},
      pluss::Config{}.with_dtype(pluss::ScalarType::Int64));

  auto g = src.gather(1, idx);
  expect_eq(g.to_vec<std::int64_t>(), {20, 10, 30, 40}, "gather basic");

  auto dst = pluss::value(std::vector<std::int64_t>{0, 0, 0, 0}, {2, 2});
  auto sc = dst.scatter(1, idx, src);
  expect_eq(sc.to_vec<std::int64_t>(), {20, 10, 30, 40}, "scatter basic");
}

void test_unary_math_and_activations() {
  auto x = pluss::value(std::vector<float>{-2.0f, -1.0f, 0.0f, 1.0f}, {4});

  auto ex = x.exp();
  auto lg = ex.log();
  expect_true(lg.numel() == 4, "exp/log pipeline");

  auto s = x.sigmoid();
  expect_true(s.scalar_type() == pluss::ScalarType::Float32, "sigmoid dtype");

  auto r = x.relu();
  expect_eq(r.to_vec<float>(), {0.0f, 0.0f, 0.0f, 1.0f}, "relu values");
}

} // namespace

int main() {
  try {
    std::cout << "[RUN] factories\n";
    test_factories();
    std::cout << "[RUN] arith_reduce\n";
    test_arithmetic_and_reduce();
    std::cout << "[RUN] indexing_ops\n";
    test_indexing_ops();
    std::cout << "[RUN] unary_activations\n";
    test_unary_math_and_activations();
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

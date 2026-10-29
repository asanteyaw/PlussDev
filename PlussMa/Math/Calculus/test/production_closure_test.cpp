#include <pluss/ma.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <sstream>
#include <thread>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;

TEST_CASE("Function artifacts preserve graph constants and assumptions",
          "[calculus][production][persistence]") {
  const pluss::Series offset = pluss::make_series({1.0, 2.0, 3.0});
  const math::Function source =
      math::function({"x"}, [offset](math::Function x) {
        return x.sin() + math::function(offset);
      }).assume("x", math::Assumption::Real);
  std::ostringstream output(std::ios::binary);
  math::io::save(source, output);
  std::istringstream input(output.str(), std::ios::binary);
  const math::Function restored = math::io::load_function(input);
  const pluss::Series x = pluss::make_series({0.25, 0.5, 0.75});

  CHECK(restored.assumption("x") == math::Assumption::Real);
  CHECK(restored({x}).allclose(source({x})));
  CHECK_THROWS(source + restored);
}

TEST_CASE("Function artifacts reject corruption schema drift and opacity",
          "[calculus][production][persistence]") {
  const math::Function square =
      math::function({"x"}, [](math::Function x) { return x.pow(2); });
  std::ostringstream output(std::ios::binary);
  math::io::save(square, output);

  std::string corrupted = output.str();
  corrupted.back() ^= 0x1;
  std::istringstream corrupted_input(corrupted, std::ios::binary);
  CHECK_THROWS(math::io::load_function(corrupted_input));

  std::string incompatible = output.str();
  const std::size_t schema_offset =
      sizeof(std::uint64_t) + std::string("pluss_math_function").size();
  incompatible[schema_offset] = 2;
  std::istringstream incompatible_input(incompatible, std::ios::binary);
  CHECK_THROWS(math::io::load_function(incompatible_input));

  const math::Function opaque =
      math::function({"x"}, [](const pluss::Series &x) { return x.sin(); });
  std::ostringstream opaque_output(std::ios::binary);
  CHECK_THROWS(math::io::save(opaque, opaque_output));
}

TEST_CASE("Function artifacts preserve fixed-bound integral interfaces",
          "[calculus][production][persistence][integral]") {
  const math::Function integrand = math::function(
      {"x", "scale"}, [](math::Function x, math::Function scale) {
        return (-x.pow(2)).exp() * scale;
      });
  const math::Function area =
      operators::integral("x", 0.0, 1.0)(integrand);
  std::ostringstream output(std::ios::binary);
  math::io::save(area, output);
  std::istringstream input(output.str(), std::ios::binary);
  const math::Function restored = math::io::load_function(input);
  const pluss::Series scale = pluss::make_series({1.0, 2.0, 4.0});

  REQUIRE(restored.arity() == 1);
  REQUIRE(restored.variables().size() == 1);
  CHECK(restored.variables()[0] == "scale");
  CHECK(restored({scale}).allclose(area({scale})));
}

TEST_CASE("built-in and custom Operator descriptors roundtrip",
          "[calculus][production][persistence]") {
  const operators::Operator plan =
      operators::compose(operators::scale(2.5),
                         operators::differential("x", 2, {.max_nodes = 2048}));
  std::ostringstream output(std::ios::binary);
  math::io::save(plan, output);
  std::istringstream input(output.str(), std::ios::binary);
  const operators::Operator restored = math::io::load_operator(input);
  const math::Function quartic =
      math::function({"x"}, [](math::Function x) { return x.pow(4); });
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0});
  CHECK(restored(quartic)({x}).allclose(plan(quartic)({x})));

  const bool registered = operators::bind(
      "example.scale", 1, [](std::string_view payload, std::uint32_t) {
        const double factor = std::stod(std::string(payload));
        return operators::define("example.scale",
                                 [factor](const math::Function &function) {
                                   return function * factor;
                                 },
                                 {.version = 1,
                                  .linear = true,
                                  .serializable = true,
                                  .payload = std::string(payload)});
      });
  REQUIRE(registered);
  const operators::Operator custom = operators::define(
      "example.scale",
      [](const math::Function &function) { return function * 3.0; },
      {.version = 1, .linear = true, .serializable = true, .payload = "3"});
  std::ostringstream custom_output(std::ios::binary);
  math::io::save(custom, custom_output);
  std::istringstream custom_input(custom_output.str(), std::ios::binary);
  const operators::Operator custom_restored =
      math::io::load_operator(custom_input);
  CHECK(custom_restored(quartic)({x}).allclose(quartic({x}).mul(3.0)));
}

TEST_CASE("immutable Functions evaluate concurrently without shared mutation",
          "[calculus][production][thread]") {
  const math::Function function =
      math::function({"x"}, [](math::Function x) { return x.sin() * x.exp(); });
  std::atomic<bool> valid{true};
  std::vector<std::thread> workers;
  workers.reserve(8);
  for (int worker = 0; worker < 8; ++worker) {
    workers.emplace_back([&, worker] {
      const double value = 0.1 * static_cast<double>(worker + 1);
      const pluss::Series x = pluss::make_series(value);
      for (int repeat = 0; repeat < 100; ++repeat) {
        const double observed = function({x}).item<double>();
        const double expected = std::sin(value) * std::exp(value);
        if (std::abs(observed - expected) > 1e-12) {
          valid.store(false, std::memory_order_relaxed);
        }
      }
    });
  }
  for (std::thread &worker : workers) {
    worker.join();
  }
  CHECK(valid.load(std::memory_order_relaxed));
}

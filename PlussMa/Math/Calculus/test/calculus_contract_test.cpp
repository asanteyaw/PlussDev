#include <pluss/ma.h>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <concepts>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;

namespace {

struct TraceFormula final {
  math::Function operator()(math::Function x) const { return x; }
};

math::Function named_formula(math::Function x, math::Function) { return x; }

struct MoveOnlyFormula final {
  explicit MoveOnlyFormula(int value) : marker(std::make_unique<int>(value)) {}
  MoveOnlyFormula(const MoveOnlyFormula &) = delete;
  MoveOnlyFormula &operator=(const MoveOnlyFormula &) = delete;
  MoveOnlyFormula(MoveOnlyFormula &&) noexcept = default;
  MoveOnlyFormula &operator=(MoveOnlyFormula &&) noexcept = default;

  math::Function operator()(math::Function x) {
    REQUIRE(*marker == 7);
    return x;
  }

  std::unique_ptr<int> marker;
};

static_assert(std::is_final_v<math::Function>);
static_assert(std::copy_constructible<math::Function>);
static_assert(std::movable<math::Function>);
static_assert(std::same_as<math::Function, pluss::subsp::math::Function>);

static_assert(std::is_final_v<operators::Operator>);
static_assert(std::copy_constructible<operators::Operator>);
static_assert(std::movable<operators::Operator>);
static_assert(
    std::same_as<operators::Operator, pluss::subsp::math::operators::Operator>);

static_assert(
    std::same_as<decltype(math::function(
                     pluss::StackArray<std::string_view>{"x"}, TraceFormula{})),
                 math::Function>);
static_assert(std::constructible_from<pluss::StackArray<std::string_view>,
                                      std::vector<std::string> &>);
static_assert(std::constructible_from<pluss::StackArray<std::string_view>,
                                      std::array<std::string_view, 2> &>);
static_assert(std::same_as<decltype(math::function(1.0)), math::Function>);
static_assert(std::same_as<
              decltype(math::function(std::declval<const pluss::Series &>())),
              math::Function>);

static_assert(
    std::same_as<decltype(operators::identity()), operators::Operator>);
static_assert(std::same_as<decltype(operators::differential("x", 2)),
                           operators::Operator>);
static_assert(
    std::same_as<decltype(operators::gradient()), operators::Operator>);
static_assert(
    std::same_as<decltype(operators::jacobian()), operators::Operator>);
static_assert(
    std::same_as<decltype(operators::hessian()), operators::Operator>);
static_assert(std::same_as<decltype(operators::antiderivative("x")),
                           operators::Operator>);
static_assert(
    std::same_as<decltype(operators::integral("x", 0.0)), operators::Operator>);
static_assert(std::same_as<decltype(operators::integral("x", 0.0, 1.0)),
                           operators::Operator>);
static_assert(
    std::same_as<decltype(operators::shift("x", 1.0)), operators::Operator>);
static_assert(
    std::same_as<decltype(operators::multiply(std::declval<math::Function>())),
                 operators::Operator>);
static_assert(
    std::same_as<decltype(operators::scale(2.0)), operators::Operator>);
static_assert(std::same_as<decltype(operators::compose(
                               std::declval<const operators::Operator &>(),
                               std::declval<const operators::Operator &>())),
                           operators::Operator>);
static_assert(
    std::same_as<decltype(operators::linear_combination(
                     std::declval<pluss::RangeView<operators::Operator>>(),
                     std::declval<pluss::RangeView<double>>())),
                 operators::Operator>);
static_assert(std::same_as<decltype(operators::commutator(
                               std::declval<const operators::Operator &>(),
                               std::declval<const operators::Operator &>())),
                           operators::Operator>);
static_assert(std::same_as<decltype(std::declval<const operators::Operator &>()(
                               std::declval<const math::Function &>())),
                           math::Function>);
static_assert(std::same_as<decltype(std::declval<const math::Function &>().arg(
                               std::declval<std::string_view>())),
                           math::Function>);
static_assert(std::same_as<decltype(std::declval<const operators::Operator &>()
                                        .capabilities()),
                           operators::Capabilities>);
static_assert(
    std::same_as<decltype(operators::define([](const math::Function &input) {
                   return input;
                 })),
                 operators::Operator>);

} // namespace

TEST_CASE("calculus public handles have explicit empty states",
          "[calculus][contract]") {
  const math::Function function;
  const operators::Operator op;

  CHECK_FALSE(function.defined());
  CHECK_FALSE(static_cast<bool>(function));
  CHECK_FALSE(op.defined());
  CHECK_FALSE(static_cast<bool>(op));
}

TEST_CASE("calculus public handles preserve empty state across copies",
          "[calculus][contract]") {
  const math::Function function;
  const math::Function function_copy = function;
  const operators::Operator op;
  const operators::Operator op_copy = op;

  CHECK_FALSE(function_copy.defined());
  CHECK_FALSE(op_copy.defined());
}

TEST_CASE("owned variable lists rebuild views after copy and move",
          "[calculus][contract][range]") {
  pluss::StackArray<std::string_view> copied;
  {
    const std::vector<std::string> source{"x", "yield_rate"};
    const pluss::StackArray<std::string_view> names(source);
    copied = names;
  }

  REQUIRE(copied.size() == 2);
  CHECK(copied.get(0) == "x");
  CHECK(copied.get(1) == "yield_rate");

  pluss::StackArray<std::string_view> moved = std::move(copied);
  REQUIRE(moved.size() == 2);
  CHECK(moved.get(0) == "x");
  CHECK(moved.get(1) == "yield_rate");
}

TEST_CASE("function traces fixed callables once and owns variable names",
          "[calculus][function][trace]") {
  int calls = 0;
  math::Function traced;
  {
    std::vector<std::string> names{"x", "interest_rate"};
    traced = math::function(names, [&](math::Function x, math::Function) {
      ++calls;
      return x;
    });
  }

  REQUIRE(traced.defined());
  CHECK(calls == 1);
  CHECK(traced.arity() == 2);
  REQUIRE(traced.variables().size() == 2);
  CHECK(traced.variables()[0] == "x");
  CHECK(traced.variables()[1] == "interest_rate");
  CHECK(traced.rank() == 0);
  CHECK(traced.shape().empty());
}

TEST_CASE("function accepts supported callable ownership forms",
          "[calculus][function][callable]") {
  const auto named_lambda = [](math::Function x) { return x; };
  const std::function<math::Function(math::Function)> erased = named_lambda;

  const math::Function from_lvalue = math::function({"x"}, named_lambda);
  const math::Function from_rvalue =
      math::function({"x"}, [](math::Function x) { return x; });
  const math::Function from_object = math::function({"x"}, TraceFormula{});
  const math::Function from_pointer =
      math::function({"x", "y"}, &named_formula);
  const math::Function from_std_function = math::function({"x"}, erased);
  const math::Function from_move_only =
      math::function({"x"}, MoveOnlyFormula{7});

  CHECK(from_lvalue.arity() == 1);
  CHECK(from_rvalue.arity() == 1);
  CHECK(from_object.arity() == 1);
  CHECK(from_pointer.arity() == 2);
  CHECK(from_std_function.arity() == 1);
  CHECK(from_move_only.arity() == 1);
}

TEST_CASE("function span form supports runtime-sized variable sets",
          "[calculus][function][arity]") {
  std::vector<std::string> names;
  names.reserve(128);
  for (std::size_t i = 0; i < 128; ++i) {
    names.push_back("variable_" + std::to_string(i));
  }

  const math::Function function =
      math::function(names, [](std::span<const math::Function> variables) {
        return variables.back();
      });

  CHECK(function.arity() == 128);
  REQUIRE(function.variables().size() == 128);
  CHECK(function.variables()[0] == "variable_0");
  CHECK(function.variables()[127] == "variable_127");
}

TEST_CASE("function constants carry scalar vector and matrix output metadata",
          "[calculus][function][shape]") {
  const math::Function scalar = math::function(3.5);
  const math::Function vector =
      math::function(pluss::make_series({1.0, 2.0, 3.0}));
  const math::Function matrix =
      math::function(pluss::make_series({1.0, 2.0, 3.0, 4.0}, {2, 2}));
  const math::Function traced_scalar =
      math::function(pluss::StackArray<std::string_view>{}, [] { return 4; });
  const math::Function traced_vector =
      math::function(pluss::StackArray<std::string_view>{},
                     [] { return pluss::make_series({5.0, 6.0}); });

  CHECK(scalar.arity() == 0);
  CHECK(scalar.rank() == 0);
  CHECK(scalar.shape().empty());

  CHECK(vector.rank() == 1);
  REQUIRE(vector.shape().size() == 1);
  CHECK(vector.shape()[0] == 3);

  CHECK(matrix.rank() == 2);
  REQUIRE(matrix.shape().size() == 2);
  CHECK(matrix.shape()[0] == 2);
  CHECK(matrix.shape()[1] == 2);

  CHECK(traced_scalar.rank() == 0);
  CHECK(traced_vector.rank() == 1);
  CHECK(traced_vector.shape()[0] == 2);
}

TEST_CASE("function rejects malformed variable and callable contracts",
          "[calculus][function][validation]") {
  CHECK_THROWS_AS(math::function({"x", "x"}, [](math::Function x,
                                                math::Function) { return x; }),
                  pluss::Error);
  CHECK_THROWS_AS(math::function({""}, [](math::Function x) { return x; }),
                  pluss::Error);
  CHECK_THROWS_AS(
      math::function({"x", "y"}, [](math::Function x) { return x; }),
      pluss::Error);
  CHECK_THROWS_AS(
      math::function({"x"}, [](math::Function) { return math::Function{}; }),
      pluss::Error);
  CHECK_THROWS_AS(math::function(pluss::Series{}), pluss::Error);
  CHECK_THROWS_AS(math::function(pluss::make_series({1.0}, {1, 1, 1})),
                  pluss::Error);
}

TEST_CASE("function rejects projections from a foreign trace",
          "[calculus][function][binding]") {
  const math::Function first =
      math::function({"x"}, [](math::Function x) { return x; });

  CHECK_THROWS_AS(math::function({"y"}, [&](math::Function) { return first; }),
                  pluss::Error);
}

TEST_CASE("undefined function metadata access fails explicitly",
          "[calculus][function][validation]") {
  const math::Function undefined;
  CHECK_THROWS_AS(undefined.arity(), pluss::Error);
  CHECK_THROWS_AS(undefined.variables(), pluss::Error);
  CHECK_THROWS_AS(undefined.rank(), pluss::Error);
  CHECK_THROWS_AS(undefined.shape(), pluss::Error);
}

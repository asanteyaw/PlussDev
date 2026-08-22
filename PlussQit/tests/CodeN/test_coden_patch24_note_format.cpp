#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <string>
#include <string_view>

#include "pluss/coden/note_factory.h"

TEST_CASE("CodeN note::format resolves placeholders and numeric expressions",
          "[coden][patch24]") {
  const int weight = 175;
  const double height = 6.45;

  const auto tpl = pluss::note::format(
      "This guy has weight {weight} and height {height}, thus his BMI is {weight * height / 2} right now",
      {
          {"weight", weight},
          {"height", height},
      });

  REQUIRE(tpl.defined());
  REQUIRE(tpl.dim() == 0);
  REQUIRE(std::string(tpl.at(0)) ==
          "This guy has weight 175 and height 6.45, thus his BMI is 564.375 right now");
}

TEST_CASE("CodeN note::format supports string placeholders and escaped braces",
          "[coden][patch24]") {
  const auto tpl = pluss::note::format(
      "User {{name}} = {name}; status = {status}",
      {
          {"name", "Alex"},
          {"status", true},
      });

  REQUIRE(std::string(tpl.at(0)) == "User {name} = Alex; status = true");
}

TEST_CASE("CodeN note::format rejects missing, duplicate, and non-numeric expression bindings",
          "[coden][patch24]") {
  REQUIRE_THROWS_WITH(
      pluss::note::format("Missing {age}", {{"name", "Alex"}}),
      Catch::Matchers::ContainsSubstring("missing binding 'age'"));

  REQUIRE_THROWS_WITH(
      pluss::note::format("Duplicate {name}", {{"name", "Alex"}, {"name", "Ada"}}),
      Catch::Matchers::ContainsSubstring("duplicate binding 'name'"));

  REQUIRE_THROWS_WITH(
      pluss::note::format("{name * 2}", {{"name", "Alex"}}),
      Catch::Matchers::ContainsSubstring("is not numeric"));
}

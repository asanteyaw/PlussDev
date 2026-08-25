#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <string>
#include <string_view>
#include <vector>

#include "pluss/coden/note_factory.h"

namespace {

double note_format_calc_tip(double cost, double percentage) {
  return cost * percentage / 100.0;
}

} // namespace

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

TEST_CASE("CodeN note::format evaluates richer f-string-style expressions",
          "[coden][patch24]") {
  const std::vector<double> prices{12.5, 18.0, 21.25};
  const int discount = 2;
  const std::string label = "ticket";

  const auto tpl = pluss::note::format(
      "These are my prices: {pluss::join(prices)}; total={std::accumulate(prices, 0.0)}; "
      "avg={avg(prices)}; first adjusted={std::min(prices[0], prices[1], prices[2]) - discount}; "
      "label={label + \"s\"}; verdict={std::accumulate(prices, 0.0) > 50 ? \"high\" : \"low\"}",
      {
          {"prices", prices},
          {"discount", discount},
          {"label", label},
          {"avg", [](const std::vector<double>& xs) {
             double total = 0.0;
             for (const double x : xs) total += x;
             return total / static_cast<double>(xs.size());
           }},
      });

  REQUIRE(std::string(tpl.at(0)) ==
          "These are my prices: 12.5, 18, 21.25; total=51.75; "
          "avg=17.25; first adjusted=10.5; label=tickets; verdict=high");
}

TEST_CASE("CodeN note::format separates ternary colons from qualified names",
          "[coden][patch24]") {
  const int weight = 175;
  const double height = 6.45;

  const auto tpl = pluss::note::format(
      "Well well, ain't that a beaut {weight == 150 ? weight: height}; root={std::sqrt(81)}",
      {
          {"weight", weight},
          {"height", height},
      });

  REQUIRE(std::string(tpl.at(0)) == "Well well, ain't that a beaut 6.45; root=9");
}

TEST_CASE("CodeN note::format invokes bound free functions", "[coden][patch24]") {
  const double cost = 85.0;
  const double tip_percent = 18.0;

  const auto tpl = pluss::note::format(
      "For cost {cost} and percentage {tip_percent}, the tip will be {calc_tip(cost, tip_percent)}",
      {
          {"cost", cost},
          {"tip_percent", tip_percent},
          {"calc_tip", note_format_calc_tip},
      });

  REQUIRE(std::string(tpl.at(0)) ==
          "For cost 85 and percentage 18, the tip will be 15.3");
}

TEST_CASE("CodeN note::format supports C++-style member access, indexing, and lambdas",
          "[coden][patch24]") {
  const std::vector<int> prices{10, 20, 30};

  const auto tpl = pluss::note::format(
      "first={prices[0]}; last={prices.at(2)}; compact={pluss::join(prices, \" | \")}; "
      "n={prices.size()}; empty={prices.empty()}; doubled={double_all(prices)}",
      {
          {"prices", prices},
          {"double_all", [](const std::vector<int>& xs) {
             std::string out;
             for (std::size_t i = 0; i < xs.size(); ++i) {
               if (i != 0) out += ", ";
               out += std::to_string(xs[i] * 2);
             }
             return out;
           }},
      });

  REQUIRE(std::string(tpl.at(0)) ==
          "first=10; last=30; compact=10 | 20 | 30; n=3; empty=false; doubled=20, 40, 60");
}

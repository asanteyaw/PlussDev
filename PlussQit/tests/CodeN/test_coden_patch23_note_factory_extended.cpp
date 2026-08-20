#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/coden/note_factory.h"

TEST_CASE("CodeN extended Note factories cover repetition, random letters, ranges, and word selection",
          "[coden][patch23]") {
  const auto repeated = pluss::note::rep("Alex", {2, 2});
  REQUIRE(repeated.dim() == 2);
  REQUIRE(repeated.size(0) == 2);
  REQUIRE(repeated.size(1) == 2);
  REQUIRE(repeated.matrix() ==
          std::vector<std::vector<std::string_view>>({{"Alex", "Alex"}, {"Alex", "Alex"}}));

  const auto random_upper = pluss::note::rand({6}, true, 7);
  REQUIRE(random_upper.dim() == 1);
  REQUIRE(random_upper.size(0) == 6);
  for (const auto& token : random_upper.vector()) {
    REQUIRE(token.size() == 1);
    REQUIRE(token[0] >= 'A');
    REQUIRE(token[0] <= 'Z');
  }

  const auto random_between = pluss::note::rand_between("D", "H", {6}, 7);
  for (const auto& token : random_between.vector()) {
    REQUIRE(token.size() == 1);
    REQUIRE(token[0] >= 'D');
    REQUIRE(token[0] <= 'H');
  }

  const auto alpha_range = pluss::note::arange("A", "H", 2, true);
  REQUIRE(alpha_range.vector() == std::vector<std::string_view>({"A", "C", "E", "G"}));

  const auto combos = pluss::note::perm("abcd", 2);
  REQUIRE(combos.vector() ==
          std::vector<std::string_view>({"ab", "ac", "ad", "bc", "bd", "cd"}));

  const auto perms = pluss::note::perm(
      "aba", 2, pluss::note::WordSelectMode::Permutation, false, true);
  REQUIRE(perms.vector() == std::vector<std::string_view>({"ab", "aa", "ba"}));
}

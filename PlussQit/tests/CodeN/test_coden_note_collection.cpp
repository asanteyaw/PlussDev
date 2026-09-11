#include <pluss/pluss.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

namespace {

pluss::Note matrix(std::initializer_list<const char*> values,
                   std::int64_t rows, std::int64_t cols) {
  std::vector<std::string> owned;
  owned.reserve(values.size());
  for (const auto* value : values) owned.emplace_back(value);
  return pluss::note::make(owned, std::array<std::int64_t, 2>{rows, cols});
}

} // namespace

TEST_CASE("Note sort orders complete lanes along the selected axis",
          "[coden][note-collection]") {
  const auto values = matrix({"b", "2", "a", "3", "b", "1"}, 3, 2);

  const auto last = values.sort();
  REQUIRE(last.sizes() == std::vector<std::int64_t>{3, 2});
  REQUIRE(last.vector() ==
          std::vector<std::string_view>{"2", "b", "3", "a", "1", "b"});
  REQUIRE(values.sort(-1).vector() == last.vector());

  const auto rows = values.sort(0);
  REQUIRE(rows.vector() ==
          std::vector<std::string_view>{"a", "1", "b", "2", "b", "3"});
  REQUIRE(pluss::note::make("x").sort().scalar() == "x");
  const auto empty = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 2>{2, 0});
  REQUIRE(empty.sort().sizes() == std::vector<std::int64_t>{2, 0});
  REQUIRE_FALSE(pluss::Note::undefined().sort().defined());
  REQUIRE_THROWS(values.sort(2));
}

TEST_CASE("Note join and join_flat differ in result ownership and defaults",
          "[coden][note-collection]") {
  const auto words = pluss::note::make({"red", "green", "blue"});
  const auto joined = words.join(" / ");
  REQUIRE(joined.is_scalar());
  REQUIRE(joined.scalar() == "red / green / blue");
  REQUIRE(words.join().scalar() == "redgreenblue");
  REQUIRE(words.join_flat() == "red green blue");
  REQUIRE(words.join_flat("|") == "red|green|blue");

  const auto grid = matrix({"a", "b", "c", "d"}, 2, 2);
  REQUIRE(grid.join("/").scalar() == "a/b/c/d");
  REQUIRE(grid.join_flat("/") == "a/b/c/d");
  REQUIRE(pluss::note::make({"", "x", ""}).join("|").scalar() == "|x|");

  const auto empty = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 2>{2, 0});
  REQUIRE(empty.join(",").scalar().empty());
  REQUIRE(empty.join_flat(",").empty());
  REQUIRE_FALSE(pluss::Note::undefined().join(",").defined());
  REQUIRE_THROWS(pluss::Note::undefined().join_flat(","));

  const auto raw = pluss::note::bytes(
      std::vector<std::string>{std::string("A\0", 2), "B"},
      std::array<std::int64_t, 1>{2});
  REQUIRE_THROWS(raw.join("|"));
  const auto flat = raw.join_flat("|");
  REQUIRE(flat.size() == 4);
  REQUIRE(flat[0] == 'A');
  REQUIRE(flat[1] == '\0');
  REQUIRE(flat[2] == '|');
  REQUIRE(flat[3] == 'B');
}

TEST_CASE("Note unique distinguishes flattened values from sorted axis slices",
          "[coden][note-collection]") {
  const auto rows = matrix({"b", "2", "a", "1", "b", "2"}, 3, 2);

  const auto flat = rows.unique();
  REQUIRE(flat.sizes() == std::vector<std::int64_t>{4});
  REQUIRE(flat.vector() ==
          std::vector<std::string_view>{"1", "2", "a", "b"});

  const auto by_row = rows.unique(0);
  REQUIRE(by_row.sizes() == std::vector<std::int64_t>{2, 2});
  REQUIRE(by_row.vector() ==
          std::vector<std::string_view>{"a", "1", "b", "2"});

  const auto columns = matrix({"b", "a", "b", "2", "1", "2"}, 2, 3);
  const auto by_column = columns.unique(-1);
  REQUIRE(by_column.sizes() == std::vector<std::int64_t>{2, 2});
  REQUIRE(by_column.vector() ==
          std::vector<std::string_view>{"a", "b", "1", "2"});

  REQUIRE(pluss::note::make("x").unique().sizes() ==
          std::vector<std::int64_t>{1});
  const auto empty_axis = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 2>{0, 2});
  REQUIRE(empty_axis.unique(0).sizes() ==
          std::vector<std::int64_t>{0, 2});
  REQUIRE_THROWS(pluss::note::make("x").unique(0));
}

TEST_CASE("Note deduplicate preserves first-seen axis slices",
          "[coden][note-collection]") {
  const auto rows = matrix({"b", "2", "a", "1", "b", "2"}, 3, 2);

  const auto flat = rows.deduplicate();
  REQUIRE(flat.sizes() == std::vector<std::int64_t>{4});
  REQUIRE(flat.vector() ==
          std::vector<std::string_view>{"b", "2", "a", "1"});

  const auto by_row = rows.deduplicate(0);
  REQUIRE(by_row.sizes() == std::vector<std::int64_t>{2, 2});
  REQUIRE(by_row.vector() ==
          std::vector<std::string_view>{"b", "2", "a", "1"});

  const auto columns = matrix({"b", "a", "b", "2", "1", "2"}, 2, 3);
  const auto by_column = columns.deduplicate(-1);
  REQUIRE(by_column.sizes() == std::vector<std::int64_t>{2, 2});
  REQUIRE(by_column.vector() ==
          std::vector<std::string_view>{"b", "a", "2", "1"});

  const auto empty = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 2>{2, 0});
  REQUIRE(empty.deduplicate(-1).sizes() ==
          std::vector<std::int64_t>{2, 0});
  REQUIRE_FALSE(pluss::Note::undefined().deduplicate().defined());
  REQUIRE_THROWS(rows.deduplicate(2));

  const auto raw = pluss::note::bytes(
      std::vector<std::string>{"b", "a", "b"},
      std::array<std::int64_t, 1>{3});
  REQUIRE(raw.sort().is_bytes());
  REQUIRE(raw.unique().is_bytes());
  REQUIRE(raw.deduplicate().is_bytes());
}

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

namespace {

std::vector<std::int64_t> integers(const pluss::Note& note) {
  return {note.integers().begin(), note.integers().end()};
}

std::vector<std::string> text(const pluss::Note& note) {
  std::vector<std::string> out;
  out.reserve(static_cast<std::size_t>(note.numel()));
  for (std::int64_t i = 0; i < note.numel(); ++i) {
    out.emplace_back(note.at(i));
  }
  return out;
}

} // namespace

TEST_CASE("Note argsort orders every closed dtype", "[coden][argsort]") {
  const auto words = pluss::note::make({"b", "a", "b", ""});
  REQUIRE(integers(words.argsort()) ==
          std::vector<std::int64_t>{3, 1, 0, 2});
  REQUIRE(integers(words.argsort(true, true)) ==
          std::vector<std::int64_t>{0, 2, 1, 3});
  REQUIRE(text(words.take(words.argsort())) ==
          std::vector<std::string>{"", "a", "b", "b"});

  const auto raw = pluss::note::bytes(
      std::vector<std::string>{"\xFF", "\x01", "\x7F"},
      std::array<std::int64_t, 1>{3});
  REQUIRE(integers(raw.argsort()) == std::vector<std::int64_t>{1, 2, 0});

  const auto flags = pluss::note::make({true, false, true, false});
  REQUIRE(integers(flags.argsort()) ==
          std::vector<std::int64_t>{1, 3, 0, 2});

  const auto ints = pluss::note::make(
      std::vector<std::int64_t>{10, -2, 10, 0});
  REQUIRE(integers(ints.argsort()) ==
          std::vector<std::int64_t>{1, 3, 0, 2});

  const auto reals = pluss::note::make(std::vector<double>{
      2.0, std::numeric_limits<double>::quiet_NaN(), -1.0,
      std::numeric_limits<double>::quiet_NaN()});
  REQUIRE(integers(reals.argsort()) ==
          std::vector<std::int64_t>{2, 0, 1, 3});
  REQUIRE(integers(reals.argsort(true, true)) ==
          std::vector<std::int64_t>{1, 3, 0, 2});

  REQUIRE(integers(pluss::note::make("scalar").argsort()) ==
          std::vector<std::int64_t>{0});
  REQUIRE(integers(pluss::note::make(std::vector<std::string>{}).argsort()).empty());
  REQUIRE_THROWS(words.reshape({2, 2}).argsort());
  REQUIRE_THROWS(pluss::Note::undefined().argsort());
}

TEST_CASE("Note repeat copies complete dimensional patterns", "[coden][repeat]") {
  const auto vector = pluss::note::make({"a", "b"});
  REQUIRE(text(vector.repeat({2})) ==
          std::vector<std::string>{"a", "b", "a", "b"});

  const auto matrix = pluss::note::make(
      std::vector<std::string>{"a", "b", "c", "d"},
      std::array<std::int64_t, 2>{2, 2});
  const auto rows = matrix.repeat({2, 1});
  REQUIRE(rows.sizes() == std::vector<std::int64_t>{4, 2});
  REQUIRE(text(rows) ==
          std::vector<std::string>{"a", "b", "c", "d", "a", "b", "c", "d"});
  const auto columns = matrix.repeat({1, 2});
  REQUIRE(columns.sizes() == std::vector<std::int64_t>{2, 4});
  REQUIRE(text(columns) ==
          std::vector<std::string>{"a", "b", "a", "b", "c", "d", "c", "d"});

  const auto leading = vector.repeat({2, 2});
  REQUIRE(leading.sizes() == std::vector<std::int64_t>{2, 4});
  REQUIRE(text(leading) ==
          std::vector<std::string>{"a", "b", "a", "b", "a", "b", "a", "b"});

  const auto scalar = pluss::note::make("x");
  REQUIRE(scalar.repeat({}).is_scalar());
  REQUIRE(scalar.repeat({}).scalar() == "x");
  REQUIRE(scalar.repeat({2, 3}).sizes() ==
          std::vector<std::int64_t>{2, 3});

  const auto zero = matrix.repeat({0, 2});
  REQUIRE(zero.sizes() == std::vector<std::int64_t>{0, 4});
  REQUIRE(zero.empty());

  const auto ints = pluss::note::make(std::vector<std::int64_t>{1, 2});
  REQUIRE(integers(ints.repeat({2})) ==
          std::vector<std::int64_t>{1, 2, 1, 2});

  REQUIRE_FALSE(pluss::Note::undefined().repeat({2}).defined());
  REQUIRE_THROWS(matrix.repeat({2}));
  REQUIRE_THROWS(vector.repeat({-1}));
  REQUIRE_THROWS(vector.repeat({1, 1, 1, 1}));
  REQUIRE_THROWS(scalar.repeat(
      {std::numeric_limits<std::int64_t>::max(), 2}));
}

TEST_CASE("Note repeat_interleave expands one selected axis",
          "[coden][repeat_interleave]") {
  const auto matrix = pluss::note::make(
      std::vector<std::string>{"a", "b", "c", "d"},
      std::array<std::int64_t, 2>{2, 2});

  const auto rows = matrix.repeat_interleave(2, 0);
  REQUIRE(rows.sizes() == std::vector<std::int64_t>{4, 2});
  REQUIRE(text(rows) ==
          std::vector<std::string>{"a", "b", "a", "b", "c", "d", "c", "d"});

  const auto columns = matrix.repeat_interleave(2, -1);
  REQUIRE(columns.sizes() == std::vector<std::int64_t>{2, 4});
  REQUIRE(text(columns) ==
          std::vector<std::string>{"a", "a", "b", "b", "c", "c", "d", "d"});

  const auto scalar = pluss::note::make("x");
  REQUIRE(text(scalar.repeat_interleave(3, 0)) ==
          std::vector<std::string>{"x", "x", "x"});
  REQUIRE(text(scalar.repeat_interleave(2, -1)) ==
          std::vector<std::string>{"x", "x"});

  const auto zero = matrix.repeat_interleave(0, 1);
  REQUIRE(zero.sizes() == std::vector<std::int64_t>{2, 0});
  REQUIRE(zero.empty());

  const auto reals = pluss::note::make(std::vector<double>{1.5, 2.5});
  const auto repeated = reals.repeat_interleave(2, 0);
  REQUIRE(std::vector<double>(repeated.reals().begin(), repeated.reals().end()) ==
          std::vector<double>{1.5, 1.5, 2.5, 2.5});

  REQUIRE_FALSE(pluss::Note::undefined().repeat_interleave(2, 0).defined());
  REQUIRE_THROWS(matrix.repeat_interleave(-1, 0));
  REQUIRE_THROWS(matrix.repeat_interleave(2, 2));
  REQUIRE_THROWS(scalar.repeat_interleave(2, 1));
  REQUIRE_THROWS(pluss::note::make({"a", "b"}).repeat_interleave(
      std::numeric_limits<std::int64_t>::max(), 0));
}

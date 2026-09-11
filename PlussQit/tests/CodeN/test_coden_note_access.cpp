#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <pluss/coden.h>
#include <pluss/coden/note_factory.h>

TEST_CASE("Note borrowed access follows row-major geometry", "[coden][note-access]") {
  const auto grid = pluss::note::make(
      std::vector<std::string>{"r0c0", "r0c1", "r0c2", "r1c0", "r1c1", "r1c2"},
      std::array<std::int64_t, 2>{2, 3});

  REQUIRE(grid.at(0) == "r0c0");
  REQUIRE(grid.at(4) == "r1c1");
  REQUIRE(grid.at_indices(std::array<std::int64_t, 2>{1, 2}) == "r1c2");
  REQUIRE(grid.cell(3) == std::string_view("r1c0"));
  REQUIRE(grid.cell_at(std::array<std::int64_t, 2>{0, 2}) == std::string_view("r0c2"));

  const auto scalar = pluss::note::make("solo");
  REQUIRE(scalar.at_indices(std::array<std::int64_t, 0>{}) == "solo");
  REQUIRE(scalar.cell_at(std::array<std::int64_t, 0>{}) == std::string_view("solo"));
}

TEST_CASE("TextRef keeps text storage alive", "[coden][note-access]") {
  pluss::TextRef kept;
  {
    const auto source = pluss::note::make({"alpha", "beta"});
    kept = source.cell(1);
  }

  REQUIRE(kept.defined());
  REQUIRE(kept.size_bytes() == 4);
  REQUIRE(kept.view() == "beta");
  REQUIRE(kept.str() == "beta");

  const auto empty = pluss::note::make("").owned();
  REQUIRE(empty.defined());
  REQUIRE(empty.empty());
  REQUIRE(empty.size() == 0);

  const auto singleton_matrix = pluss::note::make(
      std::vector<std::string>{"one"}, std::array<std::int64_t, 2>{1, 1});
  REQUIRE(singleton_matrix.owned() == std::string_view("one"));

  const std::array<std::byte, 3> raw{
      std::byte{'A'}, std::byte{0}, std::byte{'B'}};
  const auto binary = pluss::note::bytes(raw);
  REQUIRE(binary.at(0).size() == 3);
  REQUIRE(binary.at(0)[1] == '\0');
  REQUIRE(binary.cell(0).size_bytes() == 3);
}

TEST_CASE("Note host extraction owns rank-aware copies", "[coden][note-access]") {
  auto scalar_host = pluss::note::make("alpha").host();
  REQUIRE(scalar_host.is_scalar());
  REQUIRE(scalar_host.scalar() == "alpha");

  auto vector_host = pluss::note::make({"a", "b"}).host();
  REQUIRE(vector_host.is_vector());
  REQUIRE(vector_host.vector() == std::vector<std::string>{"a", "b"});

  auto matrix_host = pluss::note::make(
      std::vector<std::string>{"a", "b", "c", "d"},
      std::array<std::int64_t, 2>{2, 2}).host();
  REQUIRE(matrix_host.is_array());
  REQUIRE(matrix_host.array().sizes == std::vector<std::int64_t>{2, 2});
  REQUIRE(matrix_host.array().data == std::vector<std::string>{"a", "b", "c", "d"});
  REQUIRE(matrix_host.array().contiguous);

  const auto empty_matrix = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 2>{3, 0}).host();
  REQUIRE(empty_matrix.is_array());
  REQUIRE(empty_matrix.array().sizes == std::vector<std::int64_t>{3, 0});
  REQUIRE(empty_matrix.array().data.empty());

  REQUIRE_THROWS(scalar_host.vector());
  REQUIRE_THROWS(vector_host.scalar());
  REQUIRE_THROWS(matrix_host.vector());
}

TEST_CASE("Note access rejects invalid geometry", "[coden][note-access]") {
  const auto grid = pluss::note::make({{"a", "b"}, {"c", "d"}});
  const auto undefined = pluss::Note::undefined();

  REQUIRE_THROWS(grid.at(-1));
  REQUIRE_THROWS(grid.at(4));
  REQUIRE_THROWS(grid.cell(-1));
  REQUIRE_THROWS(grid.cell(4));
  REQUIRE_THROWS(grid.at_indices(std::array<std::int64_t, 1>{0}));
  REQUIRE_THROWS(grid.at_indices(std::array<std::int64_t, 2>{2, 0}));
  REQUIRE_THROWS(grid.cell_at(std::array<std::int64_t, 2>{0, -1}));
  REQUIRE_THROWS(grid.owned());
  REQUIRE_THROWS(undefined.at(0));
  REQUIRE_THROWS(undefined.host());
}

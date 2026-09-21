#include <pluss/pluss.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <catch2/catch_test_macros.hpp>

static_assert(std::is_same_v<decltype(std::declval<const pluss::Note&>().sizes_view()),
                             std::span<const std::int64_t>>);

template <class T>
concept HasTensor3 = requires(const T& value) { value.tensor3(); };

template <class T>
concept HasIsTensor3 = requires(const T& value) { value.is_tensor3(); };

static_assert(!HasTensor3<pluss::Note>);
static_assert(!HasIsTensor3<pluss::Note>);

TEST_CASE("Note scalar and vector projections distinguish cardinality from rank",
          "[coden][note-projection]") {
  const auto scalar = pluss::note::make("alpha");
  const auto one_vector = pluss::note::make({"beta"});
  const auto one_matrix = pluss::note::make(
      std::vector<std::string>{"gamma"}, std::array<std::int64_t, 2>{1, 1});
  const auto one_tensor = pluss::note::make(
      std::vector<std::string>{"delta"}, std::array<std::int64_t, 3>{1, 1, 1});

  REQUIRE(scalar.scalar() == "alpha");
  REQUIRE(one_vector.scalar() == "beta");
  REQUIRE(one_matrix.scalar() == "gamma");
  REQUIRE(one_tensor.scalar() == "delta");
  REQUIRE(pluss::note::make("").scalar().empty());
  REQUIRE_THROWS(pluss::note::make({"a", "b"}).scalar());
  REQUIRE_THROWS(pluss::Note::undefined().scalar());

  const auto grid = pluss::note::make(
      std::vector<std::string>{"a", "b", "c", "d"},
      std::array<std::int64_t, 2>{2, 2});
  REQUIRE(grid.vector() == std::vector<std::string_view>{"a", "b", "c", "d"});
  REQUIRE(scalar.vector() == std::vector<std::string_view>{"alpha"});

  const auto empty = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 3>{2, 0, 4});
  REQUIRE(empty.vector().empty());
  REQUIRE_THROWS(pluss::Note::undefined().vector());
}

TEST_CASE("Note matrix projection promotes lower ranks and preserves empty rows",
          "[coden][note-projection]") {
  REQUIRE(pluss::note::make("x").matrix()[0].scalar() == "x");
  REQUIRE(pluss::note::make({"a", "b"}).matrix()[0].vector() ==
          std::vector<std::string_view>{"a", "b"});

  const auto grid = pluss::note::make(
      std::vector<std::string>{"a", "b", "c", "d", "e", "f"},
      std::array<std::int64_t, 2>{2, 3});
  const auto rows = grid.matrix();
  REQUIRE(rows.size() == 2);
  REQUIRE(rows[0].vector() == std::vector<std::string_view>{"a", "b", "c"});
  REQUIRE(rows[1].vector() == std::vector<std::string_view>{"d", "e", "f"});

  const auto empty_rows = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 2>{2, 0});
  const auto projected = empty_rows.matrix();
  REQUIRE(projected.size() == 2);
  REQUIRE(projected[0].empty());
  REQUIRE(projected[0].sizes() == std::vector<std::int64_t>{0});
  REQUIRE(projected[1].empty());

  const auto tensor = pluss::note::make(
      std::vector<std::string>{"x"}, std::array<std::int64_t, 3>{1, 1, 1});
  REQUIRE_THROWS(tensor.matrix());
  REQUIRE_THROWS(pluss::Note::undefined().matrix());
  REQUIRE(pluss::note::make(std::vector<std::string>{},
                            std::array<std::int64_t, 1>{0}).matrix().size() == 1);
}

TEST_CASE("Note slabs projection requires exact rank and preserves empty slabs",
          "[coden][note-projection]") {
  const auto tensor = pluss::note::make(
      std::vector<std::string>{"000", "001", "010", "011", "100", "101", "110", "111"},
      std::array<std::int64_t, 3>{2, 2, 2});
  const auto projected = tensor.slabs();
  REQUIRE(projected.size() == 2);
  REQUIRE(projected[0].sizes() == std::vector<std::int64_t>{2, 2});
  REQUIRE(projected[1].at(1) == "101");
  REQUIRE(projected[1].at(2) == "110");

  const auto empty = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 3>{2, 0, 3});
  const auto empty_projected = empty.slabs();
  REQUIRE(empty_projected.size() == 2);
  REQUIRE(empty_projected[0].empty());
  REQUIRE(empty_projected[0].sizes() == std::vector<std::int64_t>{0, 3});
  REQUIRE(empty_projected[1].empty());

  REQUIRE_THROWS(pluss::note::make({"a"}).slabs());
  REQUIRE_THROWS(pluss::Note::undefined().slabs());
}

TEST_CASE("Note borrowed projections expose exact bytes and source-owned geometry",
          "[coden][note-projection]") {
  const std::array<std::byte, 3> raw{std::byte{'A'}, std::byte{0}, std::byte{'B'}};
  const auto bytes = pluss::note::bytes(raw);
  const auto cells = bytes.vector();
  REQUIRE(cells.size() == 1);
  REQUIRE(cells[0].size() == 3);
  REQUIRE(cells[0][1] == '\0');

  const auto grid = pluss::note::make(
      std::vector<std::string>{"a", "b", "c", "d", "e", "f"},
      std::array<std::int64_t, 2>{2, 3});
  const auto shape = grid.sizes_view();
  REQUIRE(shape.size() == 2);
  REQUIRE(shape[0] == 2);
  REQUIRE(shape[1] == 3);
  REQUIRE(shape.data() == grid.sizes_view().data());

  REQUIRE(pluss::note::make("x").sizes_view().empty());
  REQUIRE_THROWS(pluss::Note::undefined().sizes_view());
}

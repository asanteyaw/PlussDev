#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <vector>

using pluss::Series;
using pluss::ScalarType;

TEST_CASE("block returns expected 2D submatrix with view-like indexing", "[indexing][block]") {
  const Series x = pluss::make_series(
      std::vector<std::int64_t>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
      {3, 4});

  const Series b = x.block(/*row_start=*/1, /*row_count=*/2, /*col_start=*/1, /*col_count=*/3);
  REQUIRE(b.dim() == 2);
  REQUIRE(b.sizes()[0] == 2);
  REQUIRE(b.sizes()[1] == 3);
  REQUIRE(b.host<std::int64_t>() == std::vector<std::int64_t>{6, 7, 8, 10, 11, 12});

  // Negative starts are interpreted from the end, like other indexing APIs.
  const Series b2 = x.block(/*row_start=*/-2, /*row_count=*/1, /*col_start=*/-3, /*col_count=*/2);
  REQUIRE(b2.dim() == 2);
  REQUIRE(b2.sizes()[0] == 1);
  REQUIRE(b2.sizes()[1] == 2);
  REQUIRE(b2.host<std::int64_t>() == std::vector<std::int64_t>{6, 7});

  // Boundary-empty blocks are valid.
  const Series empty = x.block(/*row_start=*/3, /*row_count=*/0, /*col_start=*/4, /*col_count=*/0);
  REQUIRE(empty.dim() == 2);
  REQUIRE(empty.sizes()[0] == 0);
  REQUIRE(empty.sizes()[1] == 0);
}

TEST_CASE("block enforces 2D and strict bounds", "[indexing][block]") {
  const Series v = pluss::make_series(std::vector<std::int64_t>{1, 2, 3}, {3});
  REQUIRE_THROWS_AS(v.block(0, 1, 0, 1), pluss::Error);

  const Series x = pluss::make_series(
      std::vector<std::int64_t>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
      {3, 4});
  REQUIRE_THROWS_AS(x.block(0, 4, 0, 1), pluss::Error);
  REQUIRE_THROWS_AS(x.block(0, 1, 0, 5), pluss::Error);
  REQUIRE_THROWS_AS(x.block(4, 0, 0, 0), pluss::Error);
  REQUIRE_THROWS_AS(x.block(0, 1, 5, 0), pluss::Error);
  REQUIRE_THROWS_AS(x.block(0, -1, 0, 1), pluss::Error);
}

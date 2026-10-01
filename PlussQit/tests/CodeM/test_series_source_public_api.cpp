#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <sstream>

using pluss::Series;
using pluss::make_series;

template <class T>
concept HasStaticUndefinedFactory = requires { T::undefined(); };

static_assert(!HasStaticUndefinedFactory<Series>,
              "absence construction belongs to pluss::series, not Series");

// Series::source() -- the public, non-leaking grad_fn equivalent. Wraps
// pluss::codem::series_tape::source_info() (an internal engine namespace
// end users shouldn't have to reach into) so `x.source()` is the only thing
// a caller needs to know about.
TEST_CASE("Series::source() reports the reverse-mode node, or nullopt", "[autodiff][public-api]") {
  Series x = make_series(std::vector<double>{1.0, -2.0, 3.0}, {3});

  SECTION("untracked value has no source") {
    REQUIRE_FALSE(x.source().has_value());
  }

  SECTION("a leaf being tracked still has no source (it wasn't produced by an op)") {
    x.tracks_slope_(true);
    REQUIRE_FALSE(x.source().has_value());
    REQUIRE(x.is_leaf());
  }

  SECTION("an op applied to a tracked input reports its op name and parent count") {
    x.tracks_slope_(true);
    Series y = x.relu();
    const auto src = y.source();
    REQUIRE(src.has_value());
    REQUIRE(src->op_name == "relu");
    REQUIRE(src->parent_count == 1);
    REQUIRE_FALSE(y.is_leaf());
  }

  SECTION("a binary op reports two parents") {
    x.tracks_slope_(true);
    Series mate = make_series(std::vector<double>{0.5, 0.5, 0.5}, {3});
    mate.tracks_slope_(true);
    Series z = x.add(mate);
    const auto src = z.source();
    REQUIRE(src.has_value());
    REQUIRE(src->op_name == "add");
    REQUIRE(src->parent_count == 2);
  }

  SECTION("undefined Series has no source") {
    Series undef = pluss::series::undefined();
    REQUIRE_FALSE(undef.source().has_value());
  }
}

TEST_CASE("SeriesSource display_name()/operator<< match torch's AddBackward0-style convention",
          "[autodiff][public-api]") {
  Series x = make_series(std::vector<double>{1.0, -2.0, 3.0}, {3});
  x.tracks_slope_(true);

  SECTION("single-word op: relu -> ReluReverse") {
    REQUIRE(x.relu().source()->display_name() == "ReluReverse");
  }

  SECTION("snake_case op: grouped_mode -> GroupedModeReverse") {
    Series gid = make_series(std::vector<std::int64_t>{0, 0, 1}, {3}).to(pluss::ScalarType::Int64);
    REQUIRE(x.grouped_mode(gid, 2).source()->display_name() == "GroupedModeReverse");
  }

  SECTION("streaming a present source prints the display name") {
    std::ostringstream oss;
    oss << x.relu().source();
    REQUIRE(oss.str() == "ReluReverse");
  }

  SECTION("streaming an absent source prints None") {
    std::ostringstream oss;
    oss << x.source();  // x itself is a leaf: no source
    REQUIRE(oss.str() == "None");
  }

  SECTION("op_name itself is untouched by display formatting") {
    const auto src = x.relu().source();
    REQUIRE(src->op_name == "relu");  // stable identifier, not "Relu" or "ReluReverse"
  }
}

#include <catch2/catch_test_macros.hpp>
#include <pluss/coden.h>

using pluss::coden::Note;
using pluss::coden::TokenBatch;

TEST_CASE("Scalar Note basic ops (ASCII)", "[coden][patch03]") {
  Note a = pluss::make_note("  HeLLo  ");
  REQUIRE(a.defined());
  REQUIRE(a.dim() == 0);
  REQUIRE(a.strip().at(0) == "HeLLo");
  REQUIRE(a.lower().at(0) == "  hello  ");
  REQUIRE(a.strip().lower().at(0) == "hello");
  REQUIRE(a.contains("LL").bool_item());
  REQUIRE(a.replace("HeL", "Ye").strip().at(0) == "YeLo");
}

TEST_CASE("Note split_whitespace TokenBatch offsets", "[coden][patch03]") {
  std::vector<std::string_view> flat = {"a bb", "   c", ""};
  const std::array<std::int64_t, 1> shape{3};
  Note n = pluss::make_note(flat, std::span<const std::int64_t>(shape));
  auto tb = n.split_whitespace();
  // offsets is a closed Int64 Note (pass 6); length numel+1.
  REQUIRE(tb.offsets().is_int64());
  const auto offs = tb.offsets().integers();
  REQUIRE(offs.size() == 4);
  // element0: 2 tokens, element1: 1 token, element2: 0 tokens
  REQUIRE(offs[0] == 0);
  REQUIRE(offs[1] == 2);
  REQUIRE(offs[2] == 3);
  REQUIRE(offs[3] == 3);

  // tokens should be ["a","bb","c"]
  REQUIRE(tb.tokens().defined());
  REQUIRE(tb.tokens().numel() == 3);
  REQUIRE(tb.tokens().at(0) == "a");
  REQUIRE(tb.tokens().at(1) == "bb");
  REQUIRE(tb.tokens().at(2) == "c");
}

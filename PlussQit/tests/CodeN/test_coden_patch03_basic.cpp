#include <catch2/catch_test_macros.hpp>
#include <pluss/coden.h>

using pluss::coden::Note;
using pluss::coden::TokenBatch;

TEST_CASE("Scalar Note basic ops (ASCII)", "[coden][patch03]") {
  Note a = pluss::content("  HeLLo  ");
  REQUIRE(a.defined());
  REQUIRE(a.dim() == 0);
  REQUIRE(a.strip().at(0) == "HeLLo");
  REQUIRE(a.lower().at(0) == "  hello  ");
  REQUIRE(a.strip().lower().at(0) == "hello");
  REQUIRE(a.contains("LL").at(0) == 1);
  REQUIRE(a.replace("HeL", "Ye").strip().at(0) == "Yeo");
}

TEST_CASE("Note split_whitespace TokenBatch offsets", "[coden][patch03]") {
  std::vector<std::string_view> flat = {"a bb", "   c", ""};
  Note n = pluss::make_note(flat, std::span<const std::int64_t>({3}));
  auto tb = n.split_whitespace();
  // offsets length = numel+1
  REQUIRE(tb.offsets().size() == 4);
  // element0: 2 tokens, element1: 1 token, element2: 0 tokens
  REQUIRE(tb.offsets()[0] == 0);
  REQUIRE(tb.offsets()[1] == 2);
  REQUIRE(tb.offsets()[2] == 3);
  REQUIRE(tb.offsets()[3] == 3);

  // tokens should be ["a","bb","c"]
  REQUIRE(tb.tokens().defined());
  REQUIRE(tb.tokens().numel() == 3);
  REQUIRE(tb.tokens().at(0) == "a");
  REQUIRE(tb.tokens().at(1) == "bb");
  REQUIRE(tb.tokens().at(2) == "c");
}

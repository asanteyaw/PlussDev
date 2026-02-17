#include <catch2/catch_test_macros.hpp>

#include "pluss/coden.h"

using pluss::coden::Note;

TEST_CASE("CodeN Note structural ops: transpose/permute/select/narrow") {
  // 2x3
  std::vector<std::string_view> flat = {"a","b","c","d","e","f"};
  std::array<std::int64_t,2> sz = {2,3};
  Note x = pluss::make_note(flat, std::span<const std::int64_t>(sz.data(), sz.size()));
  REQUIRE(x.dim() == 2);
  REQUIRE(x.numel() == 6);
  REQUIRE(x.at(0).utf8() == "a");
  REQUIRE(x.at(5).utf8() == "f");

  // transpose -> 3x2, row-major reorder.
  Note t = x.transpose(0,1);
  auto ts = t.sizes();
  REQUIRE(ts.size() == 2);
  REQUIRE(ts[0] == 3);
  REQUIRE(ts[1] == 2);
  // (0,0) maps to (0,0)
  REQUIRE(t.at(0).utf8() == "a");
  // (0,1) maps to (1,0) => d
  REQUIRE(t.at(1).utf8() == "d");

  // permute same as transpose for 2D
  std::array<std::int64_t,2> p = {1,0};
  Note p2 = x.permute(std::span<const std::int64_t>(p.data(), p.size()));
  REQUIRE(p2.at(1).utf8() == "d");

  // select along dim0 index1 => 1D of length3: d e f
  Note s0 = x.select(0, 1);
  auto s0s = s0.sizes();
  REQUIRE(s0.dim() == 1);
  REQUIRE(s0s[0] == 3);
  REQUIRE(s0.at(0).utf8() == "d");
  REQUIRE(s0.at(2).utf8() == "f");

  // narrow along dim1 start1 length2 => 2x2: b c / e f
  Note n = x.narrow(1, 1, 2);
  auto ns = n.sizes();
  REQUIRE(ns.size() == 2);
  REQUIRE(ns[0] == 2);
  REQUIRE(ns[1] == 2);
  REQUIRE(n.at(0).utf8() == "b");
  REQUIRE(n.at(1).utf8() == "c");
  REQUIRE(n.at(2).utf8() == "e");
  REQUIRE(n.at(3).utf8() == "f");
}

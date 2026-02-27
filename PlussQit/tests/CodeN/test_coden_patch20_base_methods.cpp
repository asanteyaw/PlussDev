#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "CodeN/core/Chord.h"
#include "CodeN/core/OpsInternal.h"
#include "pluss/coden.h"
#include "pluss/coden/note_factory.h"

TEST_CASE("CodeN NoteBase inherited convenience methods", "[coden][patch20]") {
  SECTION("scalar/vector/matrix/tensor projections") {
    auto scalar = pluss::make_note("alpha");
    REQUIRE(scalar.defined());
    REQUIRE(scalar.is_scalar());
    REQUIRE(scalar.scalar() == "alpha");
    REQUIRE(scalar.vector().size() == 1);
    REQUIRE(scalar.matrix().size() == 1);

    std::vector<std::string> v = {"a", "b", "c"};
    auto vec = pluss::make_note(v);
    REQUIRE(vec.is_vector());
    const auto vv = vec.vector();
    REQUIRE(vv.size() == 3);
    REQUIRE(vv[0] == "a");
    REQUIRE(vv[2] == "c");
    REQUIRE(vec.join_flat("-") == "a-b-c");

    std::vector<std::string> mflat = {"m00", "m01", "m10", "m11"};
    const std::array<std::int64_t, 2> msz{2, 2};
    auto mat = pluss::make_note(mflat, msz);
    REQUIRE(mat.is_matrix());
    const auto mm = mat.matrix();
    REQUIRE(mm.size() == 2);
    REQUIRE(mm[0].size() == 2);
    REQUIRE(mm[1].size() == 2);
    REQUIRE(mm[0][1] == "m01");
    REQUIRE(mm[1][0] == "m10");

    std::vector<std::string> tflat = {"000", "001", "010", "011", "100", "101", "110", "111"};
    const std::array<std::int64_t, 3> tsz{2, 2, 2};
    auto t3 = pluss::make_note(tflat, tsz);
    REQUIRE(t3.is_tensor3());
    const auto tt = t3.tensor3();
    REQUIRE(tt.size() == 2);
    REQUIRE(tt[1][0][1] == "101");
    REQUIRE(t3.at_indices(std::array<std::int64_t, 3>{1, 1, 0}) == "110");
  }

  SECTION("linear index, copy, for_each, iterator and debug") {
    std::vector<std::string> flat = {"q", "w", "e", "r", "t", "y"};
    const std::array<std::int64_t, 2> sz{2, 3};
    auto note = pluss::make_note(flat, sz);

    REQUIRE(note.linear_index(std::array<std::int64_t, 2>{0, 0}) == 0);
    REQUIRE(note.linear_index(std::array<std::int64_t, 2>{1, 2}) == 5);
    REQUIRE(note.flat_at(4) == "t");

    std::vector<std::string_view> copied;
    note.copy_flat_to(std::back_inserter(copied));
    REQUIRE(copied.size() == 6);
    REQUIRE(copied[1] == "w");
    REQUIRE(copied[5] == "y");

    std::size_t count = 0;
    std::size_t chars = 0;
    note.for_each_flat([&](std::string_view s) {
      ++count;
      chars += s.size();
    });
    REQUIRE(count == 6);
    REQUIRE(chars == 6);

    std::string joined;
    for (auto it = note.begin(); it != note.end(); ++it) {
      joined.append(*it);
    }
    REQUIRE(joined == "qwerty");

    const auto dbg = note.debug(3);
    REQUIRE(dbg.find("shape=[2, 3]") != std::string::npos);
    REQUIRE(dbg.find("values=[\"q\", \"w\", \"e\", ...]") != std::string::npos);
  }
}

TEST_CASE("CodeN ChordBase iterator and byte helpers", "[coden][patch20]") {
  const auto chord = pluss::coden::ops_internal::chord_from_utf8("Ab1");
  REQUIRE(chord.defined());
  REQUIRE(!chord.empty());
  REQUIRE(chord.size() == 3);
  REQUIRE(chord.as_string() == "Ab1");

  std::string out;
  for (char c : chord) {
    out.push_back(c);
  }
  REQUIRE(out == "Ab1");

  int ascii_sum = 0;
  chord.for_each_byte([&](char c) { ascii_sum += static_cast<unsigned char>(c); });
  REQUIRE(ascii_sum == (static_cast<int>('A') + static_cast<int>('b') + static_cast<int>('1')));
}

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>

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
    REQUIRE(mm[0].numel() == 2);
    REQUIRE(mm[1].numel() == 2);
    REQUIRE(mm[0].at(1) == "m01");
    REQUIRE(mm[1].at(0) == "m10");

    std::vector<std::string> tflat = {"000", "001", "010", "011", "100", "101", "110", "111"};
    const std::array<std::int64_t, 3> tsz{2, 2, 2};
    auto t3 = pluss::make_note(tflat, tsz);
    REQUIRE(t3.is_tensor3());
    const auto tt = t3.tensor3();
    REQUIRE(tt.size() == 2);
    REQUIRE(tt[1].at(1) == "101");
    REQUIRE(t3.at_indices(std::array<std::int64_t, 3>{1, 1, 0}) == "110");
  }

  SECTION("linear index, copy, for_each, iterator and debug") {
    std::vector<std::string> flat = {"q", "w", "e", "r", "t", "y"};
    const std::array<std::int64_t, 2> sz{2, 3};
    auto note = pluss::make_note(flat, sz);

    REQUIRE(note.at_indices(std::array<std::int64_t, 2>{0, 0}) == "q");
    REQUIRE(note.at_indices(std::array<std::int64_t, 2>{1, 2}) == "y");
    REQUIRE(note.at(4) == "t");

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

TEST_CASE("CodeN TextRef owns its cell past the parent Note", "[coden][patch20]") {
  pluss::TextRef ref;
  {
    const auto n = pluss::make_note({"Ab1", "xyz"});
    ref = n.cell(0);
  }  // n destroyed; ref keeps the arena alive

  REQUIRE(ref.defined());
  REQUIRE(!ref.empty());
  REQUIRE(ref.size() == 3);
  REQUIRE(ref.str() == "Ab1");
  REQUIRE(ref == std::string_view("Ab1"));

  std::string out;
  for (char c : ref) {
    out.push_back(c);
  }
  REQUIRE(out == "Ab1");

  // Implicit conversion to std::string_view.
  const std::string_view sv = ref;
  REQUIRE(sv == "Ab1");

  // owned() is the keep-past-lifetime form of scalar().
  REQUIRE(pluss::make_note("solo").owned().str() == "solo");

  // cell_at mirrors at_indices.
  const auto grid = pluss::make_note(std::vector<std::string>{"p", "q", "r", "s"},
                                     std::array<std::int64_t, 2>{2, 2});
  REQUIRE(grid.cell_at(std::array<std::int64_t, 2>{1, 0}).str() == "r");
}

TEST_CASE("CodeN Note metadata distinguishes rank absence and empty extents",
          "[coden][note-metadata]") {
  const auto undefined = pluss::Note::undefined();
  REQUIRE_FALSE(undefined.defined());
  REQUIRE_FALSE(undefined.is_scalar());
  REQUIRE_FALSE(undefined.is_vector());
  REQUIRE_FALSE(undefined.is_matrix());
  REQUIRE_FALSE(undefined.is_tensor3());
  REQUIRE_THROWS(undefined.size(0));
  REQUIRE_THROWS(undefined.sizes());

  const auto scalar = pluss::make_note("one");
  const auto singleton = pluss::make_note({"one"});
  const auto matrix = pluss::make_note(
      std::vector<std::string>{"a", "b", "c", "d", "e", "f"},
      std::array<std::int64_t, 2>{2, 3});
  const auto tensor = pluss::make_note(
      std::vector<std::string>{"a", "b", "c", "d", "e", "f", "g", "h"},
      std::array<std::int64_t, 3>{2, 2, 2});

  REQUIRE(scalar.is_scalar());
  REQUIRE_FALSE(singleton.is_scalar());
  REQUIRE(singleton.is_vector());
  REQUIRE(matrix.is_matrix());
  REQUIRE(tensor.is_tensor3());
  REQUIRE(matrix.dim() == 2);
  REQUIRE(matrix.sizes() == std::vector<std::int64_t>{2, 3});
  REQUIRE(matrix.numel() == 6);
  REQUIRE(matrix.size(0) == 2);
  REQUIRE(matrix.size(1) == 3);
  REQUIRE(matrix.size(-1) == 3);
  REQUIRE(matrix.size(-2) == 2);
  REQUIRE_THROWS(matrix.size(2));
  REQUIRE_THROWS(matrix.size(-3));

  const auto empty = pluss::make_note(
      std::vector<std::string>{}, std::array<std::int64_t, 2>{2, 0});
  REQUIRE(empty.defined());
  REQUIRE(empty.empty());
  REQUIRE(empty.is_matrix());
  REQUIRE(empty.sizes() == std::vector<std::int64_t>{2, 0});
  REQUIRE(empty.numel() == 0);
}

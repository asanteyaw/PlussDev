#include <catch2/catch_test_macros.hpp>
#include <pluss/coden.h>

using pluss::coden::Note;

TEST_CASE("Bytes-mode Note basic behavior", "[coden][patch10]") {
  // raw bytes including a NUL
  const unsigned char raw[] = {0x41, 0x00, 0x42};
  auto b = pluss::bytes_content(std::span<const std::byte>(reinterpret_cast<const std::byte*>(raw), 3));

  REQUIRE(b.defined());
  REQUIRE(b.dim() == 0);
  REQUIRE(static_cast<const pluss::coden::NoteBase&>(b).is_bytes());

  auto v = b.at(0);
  REQUIRE(v.size() == 3);
  REQUIRE(static_cast<unsigned char>(v[0]) == 0x41);
  REQUIRE(static_cast<unsigned char>(v[1]) == 0x00);
  REQUIRE(static_cast<unsigned char>(v[2]) == 0x42);

  // Byte-wise ops that are safe / intentional.
  REQUIRE(b.lengths_bytes().at(0) == 3);
  REQUIRE(b.contains(std::string_view("A", 1)).at(0) == 1);

  // Text-assuming ops should reject bytes-mode.
  REQUIRE_THROWS(b.lower());
  REQUIRE_THROWS(b.strip());
  REQUIRE_THROWS(b.split_whitespace());
}

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("CodeN TeX delimiter/spacing cleanup family is concrete", "[coden][patch15]") {
  pluss::Note tex = pluss::make_note("\\left( x \\right) + \\Bigl[ y \\Bigr] + a\\,b\\quad c");
  auto out = tex.tex_remove_left_right()
                 .tex_remove_big_delims()
                 .tex_remove_spacing_cmds();
  const std::string s = out.at(0);
  REQUIRE(s.find("\\left") == std::string::npos);
  REQUIRE(s.find("\\right") == std::string::npos);
  REQUIRE(s.find("\\Bigl") == std::string::npos);
  REQUIRE(s.find("\\Bigr") == std::string::npos);
  REQUIRE(s.find("\\,") == std::string::npos);
  REQUIRE(s.find("\\quad") == std::string::npos);
  REQUIRE(s.find("a b") != std::string::npos);
}

TEST_CASE("CodeN TeX replace symbol family routes through concrete impl", "[coden][patch15]") {
  const std::array<std::int64_t, 1> sizes{2};
  pluss::Note n = pluss::make_note(
      std::vector<std::string>{"\\alpha + \\beta + \\omega", "\\gamma + \\varphi"},
      std::span<const std::int64_t>(sizes.data(), sizes.size()));

  auto replaced = n.tex_replace_alpha("A")
                   .tex_replace_beta("B")
                   .tex_replace_omega("W")
                   .tex_replace_varphi("VP")
                   .tex_replace_command("gamma", "G");

  REQUIRE(replaced.at(0).find("\\alpha") == std::string::npos);
  REQUIRE(replaced.at(0).find("\\beta") == std::string::npos);
  REQUIRE(replaced.at(0).find("\\omega") == std::string::npos);
  REQUIRE(replaced.at(0).find("A + B + W") != std::string::npos);
  REQUIRE(replaced.at(1).find("\\gamma") == std::string::npos);
  REQUIRE(replaced.at(1).find("\\varphi") == std::string::npos);
  REQUIRE(replaced.at(1).find("G + VP") != std::string::npos);
}

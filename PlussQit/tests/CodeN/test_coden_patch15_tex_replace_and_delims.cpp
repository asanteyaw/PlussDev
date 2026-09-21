#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("CodeN TeX delimiter/spacing cleanup family is concrete", "[coden][patch15]") {
  pluss::Note tex = pluss::make_note("\\left( x \\right) + \\Bigl[ y \\Bigr] + a\\,b\\quad c");
  auto out = tex.tex().remove_left_right()
                 .tex().remove_big_delims()
                 .tex().remove_spacing_cmds();
  const std::string s = std::string(out.at(0));
  REQUIRE(s.find("\\left") == std::string::npos);
  REQUIRE(s.find("\\right") == std::string::npos);
  REQUIRE(s.find("\\Bigl") == std::string::npos);
  REQUIRE(s.find("\\Bigr") == std::string::npos);
  REQUIRE(s.find("\\,") == std::string::npos);
  REQUIRE(s.find("\\quad") == std::string::npos);
  REQUIRE(s.find("a b") != std::string::npos);
}

TEST_CASE("CodeN TeX replace_command handles every Greek symbol name", "[coden][patch15]") {
  const std::array<std::int64_t, 1> sizes{2};
  pluss::Note n = pluss::make_note(
      std::vector<std::string>{"\\alpha + \\beta + \\omega", "\\gamma + \\varphi"},
      std::span<const std::int64_t>(sizes.data(), sizes.size()));

  // The 15 per-letter tex_replace_<name>() convenience methods were removed
  // as pure aliases: each forwarded, with zero unique logic, to
  // replace_command(name, repl). replace_command is the one surface now.
  auto replaced = n.tex().replace_command("alpha", "A")
                   .tex().replace_command("beta", "B")
                   .tex().replace_command("omega", "W")
                   .tex().replace_command("varphi", "VP")
                   .tex().replace_command("gamma", "G");

  REQUIRE(replaced.at(0).find("\\alpha") == std::string::npos);
  REQUIRE(replaced.at(0).find("\\beta") == std::string::npos);
  REQUIRE(replaced.at(0).find("\\omega") == std::string::npos);
  REQUIRE(replaced.at(0).find("A + B + W") != std::string::npos);
  REQUIRE(replaced.at(1).find("\\gamma") == std::string::npos);
  REQUIRE(replaced.at(1).find("\\varphi") == std::string::npos);
  REQUIRE(replaced.at(1).find("G + VP") != std::string::npos);
}

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("CodeN split limit overloads are concrete", "[coden][patch14]") {
  const std::array<std::int64_t, 1> sizes{2};
  pluss::Note n = pluss::make_note(
      std::vector<std::string>{"a,b,c", "d,e"},
      std::span<const std::int64_t>(sizes.data(), sizes.size()));

  auto by_comma = n.split(",", 1);
  REQUIRE(by_comma.offsets() == std::vector<std::int64_t>({0, 2, 4}));
  REQUIRE(by_comma.tokens().at(0) == "a");
  REQUIRE(by_comma.tokens().at(1) == "b,c");
  REQUIRE(by_comma.tokens().at(2) == "d");
  REQUIRE(by_comma.tokens().at(3) == "e");

  pluss::Note ws = pluss::make_note(
      std::vector<std::string>{"x  y   z", "m n"},
      std::span<const std::int64_t>(sizes.data(), sizes.size()));
  auto by_ws = ws.split_whitespace(1);
  REQUIRE(by_ws.offsets() == std::vector<std::int64_t>({0, 2, 4}));
  REQUIRE(by_ws.tokens().at(0) == "x");
  REQUIRE(by_ws.tokens().at(1) == "y z");
  REQUIRE(by_ws.tokens().at(2) == "m");
  REQUIRE(by_ws.tokens().at(3) == "n");
}

TEST_CASE("CodeN TeX remover variants are concrete", "[coden][patch14]") {
  pluss::Note tex = pluss::make_note(
      "\\usepackage{graphicx}\n"
      "Text \\label{l1} \\ref{l1} \\cite{key}\n"
      "\\includegraphics{img.png}\n"
      "\\href{https://example.com}{link}\n"
      "\\textcolor{red}{colored}\n"
      "\\begin{equation}x+y\\end{equation}\n");

  auto cleaned = tex.tex_remove_packages()
                     .tex_remove_labels()
                     .tex_remove_refs()
                     .tex_remove_citations()
                     .tex_remove_graphics()
                     .tex_remove_urls()
                     .tex_remove_color()
                     .tex_remove_env_equation();

  const std::string out = cleaned.at(0);
  REQUIRE(out.find("usepackage") == std::string::npos);
  REQUIRE(out.find("\\label") == std::string::npos);
  REQUIRE(out.find("\\ref") == std::string::npos);
  REQUIRE(out.find("\\cite") == std::string::npos);
  REQUIRE(out.find("includegraphics") == std::string::npos);
  REQUIRE(out.find("https://") == std::string::npos);
  REQUIRE(out.find("equation") == std::string::npos);
  REQUIRE(out.find("link") != std::string::npos);
  REQUIRE(out.find("colored") != std::string::npos);
}

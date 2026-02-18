#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("CodeN replace/remove family has concrete behavior", "[coden][patch13]") {
  pluss::Note s = pluss::content("alpha beta beta");
  REQUIRE(s.replace_first("beta", "X").at(0) == "alpha X beta");
  REQUIRE(s.replace_last("beta", "X").at(0) == "alpha beta X");
  REQUIRE(s.replace_n("beta", "X", 1).at(0) == "alpha X beta");
  REQUIRE(s.remove("beta").at(0) == "alpha  beta");
  REQUIRE(s.remove_all("beta").at(0) == "alpha  ");

  pluss::Note t = pluss::content("payload");
  REQUIRE(t.ensure_prefix("pre_").at(0) == "pre_payload");
  REQUIRE(t.ensure_suffix("_suf").at(0) == "payload_suf");
  REQUIRE(t.surround("[", "]").at(0) == "[payload]");
  REQUIRE(t.quote().at(0) == "\"payload\"");
  REQUIRE(t.quote().unquote().at(0) == "payload");
}

TEST_CASE("CodeN tokenization families return structured TokenBatch", "[coden][patch13]") {
  const std::array<std::int64_t, 1> sizes{2};
  pluss::Note n = pluss::content(
      std::vector<std::string>{"One.\nTwo!", "A,b;c"},
      std::span<const std::int64_t>(sizes.data(), sizes.size()));

  auto lines = n.split_lines();
  REQUIRE(lines.offsets() == std::vector<std::int64_t>({0, 2, 3}));
  REQUIRE(lines.tokens().numel() == 3);
  REQUIRE(lines.tokens().at(0) == "One.");
  REQUIRE(lines.tokens().at(1) == "Two!");
  REQUIRE(lines.tokens().at(2) == "A,b;c");

  auto words = n.tokenize_words();
  REQUIRE(words.tokens().numel() >= 4);
  REQUIRE(words.tokens().at(0) == "One");
  REQUIRE(words.tokens().at(1) == "Two");

  auto punct = n.tokenize_punct();
  REQUIRE(punct.tokens().numel() >= 4);

  auto findall = n.regex_findall("[A-Za-z]+");
  REQUIRE(findall.tokens().numel() >= 4);
  REQUIRE(findall.tokens().at(0) == "One");

  auto split_re = n.regex_split("[,;]");
  REQUIRE(split_re.tokens().numel() >= 4);
}

TEST_CASE("CodeN TeX cleanup family has concrete behavior", "[coden][patch13]") {
  pluss::Note tex = pluss::content(
      "\\documentclass{article}\n"
      "% header comment\n"
      "\\begin{document}\n"
      "Text \\textbf{bold} and $x+y$.\n"
      "\\end{document}\n");

  auto stripped = tex.tex_strip_comments();
  REQUIRE(stripped.at(0).find("% header comment") == std::string::npos);

  auto body = stripped.tex_strip_preamble();
  REQUIRE(body.at(0).find("\\documentclass") == std::string::npos);

  auto plain = body.tex_to_text();
  REQUIRE(plain.at(0).find('\\') == std::string::npos);
  REQUIRE(plain.at(0).find("Text") != std::string::npos);
  REQUIRE(plain.at(0).find("bold") != std::string::npos);
}

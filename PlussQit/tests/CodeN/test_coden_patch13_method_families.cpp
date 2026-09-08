#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("CodeN replace/remove family has concrete behavior", "[coden][patch13]") {
  pluss::Note s = pluss::make_note("alpha beta beta");
  REQUIRE(s.replace_first("beta", "X").at(0) == "alpha X beta");
  REQUIRE(s.replace_last("beta", "X").at(0) == "alpha beta X");
  REQUIRE(s.replace_n("beta", "X", 1).at(0) == "alpha X beta");
  REQUIRE(s.remove("beta").at(0) == "alpha  beta");
  REQUIRE(s.remove_all("beta").at(0) == "alpha  ");

  pluss::Note t = pluss::make_note("payload");
  REQUIRE(t.ensure_prefix("pre_").at(0) == "pre_payload");
  REQUIRE(t.ensure_suffix("_suf").at(0) == "payload_suf");
  REQUIRE(t.surround("[", "]").at(0) == "[payload]");
  REQUIRE(t.quote().at(0) == "\"payload\"");
  REQUIRE(t.quote().unquote().at(0) == "payload");
}

TEST_CASE("CodeN tokenization families return structured TokenBatch", "[coden][patch13]") {
  const std::array<std::int64_t, 1> sizes{2};
  pluss::Note n = pluss::make_note(
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
  pluss::Note tex = pluss::make_note(
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

TEST_CASE("CodeN tokenize/regex/tex parameterized overloads take effect", "[coden][patch13]") {
  // regex_findall(pattern, flags): bit 0 = case-insensitive.
  pluss::Note s = pluss::make_note("AbCdE");
  REQUIRE(s.regex_findall("[a-z]+").tokens().numel() == 2);     // "b", "d"
  REQUIRE(s.regex_findall("[a-z]+", 0).tokens().numel() == 2);
  auto ci = s.regex_findall("[a-z]+", 1);
  REQUIRE(ci.tokens().numel() == 1);
  REQUIRE(ci.tokens().at(0) == "AbCdE");

  // tokenize_words(flags): bit 0 = case-fold each token.
  pluss::Note w = pluss::make_note("One TWO three");
  REQUIRE(w.tokenize_words().tokens().at(1) == "TWO");
  auto wl = w.tokenize_words(1);
  REQUIRE(wl.tokens().at(0) == "one");
  REQUIRE(wl.tokens().at(1) == "two");
  REQUIRE(wl.tokens().at(2) == "three");

  // tex_to_text(mode): mode >= 1 also case-folds the recovered text.
  pluss::Note t = pluss::make_note("\\textbf{Hello} World");
  REQUIRE(t.tex_to_text().at(0) == "Hello World");
  REQUIRE(t.tex_to_text(1).at(0) == "hello world");
}

TEST_CASE("CodeN capitalize matches Python str.capitalize semantics", "[coden][patch13]") {
  // First character upper-cased in place; the rest lower-cased. A leading
  // digit or space is left as-is (no "first letter" skipping).
  REQUIRE(pluss::make_note("heLLo wORLd").capitalize().at(0) == "Hello world");
  REQUIRE(pluss::make_note("3rd place").capitalize().at(0) == "3rd place");
  REQUIRE(pluss::make_note("ALL_CAPS").capitalize().at(0) == "All_caps");
}

TEST_CASE("CodeN lengths_graphemes clusters combining marks", "[coden][patch13]") {
  // "e" + U+0301 (combining acute) is one grapheme but two code points.
  pluss::Note n = pluss::make_note(std::string("e\xCC\x81"));
  REQUIRE(n.lengths_chars() == std::vector<std::int64_t>({2}));
  REQUIRE(n.lengths_graphemes() == std::vector<std::int64_t>({1}));
  // Pure ASCII: graphemes == chars == bytes.
  REQUIRE(pluss::make_note("abc").lengths_graphemes() == std::vector<std::int64_t>({3}));
}

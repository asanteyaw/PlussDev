#include <cstdint>
#include <string>

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

namespace {
bool has_token(const pluss::Note& toks, const std::string& needle) {
  for (std::int64_t i = 0; i < toks.numel(); ++i) {
    if (toks.at(i) == needle) return true;
  }
  return false;
}
} // namespace

TEST_CASE("CodeN token postprocess split/filter family is concrete", "[coden][patch18]") {
  pluss::Note n = pluss::content("foo_bar CamelCase baz qux 2025");

  auto len_ge = n.nlp_filter_tokens_len_ge(4);
  REQUIRE(len_ge.tokens().numel() > 0);
  REQUIRE(has_token(len_ge.tokens(), "foo_bar"));
  REQUIRE(has_token(len_ge.tokens(), "2025"));

  auto uniq = n.nlp_unique_tokens();
  REQUIRE(uniq.tokens().numel() > 0);

  auto sorted = n.nlp_sort_tokens();
  REQUIRE(sorted.tokens().numel() == n.nlp_tokens().tokens().numel());

  auto removed_numbers = n.nlp_remove_number_tokens();
  REQUIRE(!has_token(removed_numbers.tokens(), "2025"));
}

TEST_CASE("CodeN TeX alias extraction family is concrete", "[coden][patch18]") {
  pluss::Note tex = pluss::content(
      "\\begin{theorem}A\\end{theorem}\n"
      "\\begin{lemma}B\\end{lemma}\n"
      "\\caption{Figure cap}\n");

  auto th = tex.tex_theorems();
  REQUIRE(th.tokens().numel() >= 1);
  REQUIRE(th.tokens().at(0).find("theorem") != std::string::npos);

  auto le = tex.tex_lemmas();
  REQUIRE(le.tokens().numel() >= 1);
  REQUIRE(le.tokens().at(0).find("lemma") != std::string::npos);

  auto caps = tex.tex_captions();
  REQUIRE(has_token(caps.tokens(), "Figure cap"));
}

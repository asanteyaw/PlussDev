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
  pluss::Note n = pluss::make_note("foo_bar CamelCase baz qux 2025");

  auto len_ge = n.nlp().min_length(4);
  REQUIRE(len_ge.tokens().numel() > 0);
  REQUIRE(has_token(len_ge.tokens(), "foo_bar"));
  REQUIRE(has_token(len_ge.tokens(), "2025"));

  auto uniq = n.nlp().unique_tokens();
  REQUIRE(uniq.tokens().numel() > 0);

  auto sorted = n.nlp().sort_tokens();
  REQUIRE(sorted.tokens().numel() == n.nlp().tokens().tokens().numel());

  auto removed_numbers = n.nlp().without_numbers();
  REQUIRE(!has_token(removed_numbers.tokens(), "2025"));
}

TEST_CASE("CodeN TeX named-environment extraction family is concrete", "[coden][patch18]") {
  pluss::Note tex = pluss::make_note(
      "\\begin{theorem}A\\end{theorem}\n"
      "\\begin{lemma}B\\end{lemma}\n"
      "\\caption{Figure cap}\n");

  // theorem/lemma no longer have dedicated methods (they were pure aliases
  // of env(name) with no unique logic of their own); the
  // generic form is the one, closed-TokenBatch-returning surface for any
  // environment name.
  auto th = tex.tex().env("theorem");
  REQUIRE(th.tokens().numel() >= 1);
  REQUIRE(th.tokens().at(0).find("theorem") != std::string::npos);

  auto le = tex.tex().env("lemma");
  REQUIRE(le.tokens().numel() >= 1);
  REQUIRE(le.tokens().at(0).find("lemma") != std::string::npos);

  auto caps = tex.tex().captions();
  REQUIRE(has_token(caps.tokens(), "Figure cap"));
}

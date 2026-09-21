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

TEST_CASE("CodeN TeX extract/token command family is concrete", "[coden][patch17]") {
  pluss::Note tex = pluss::make_note(
      "\\section{Intro}\n"
      "\\begin{equation} E = mc^2 \\end{equation}\n"
      "See \\ref{eq:1} and \\cite{einstein1905}.\n");

  auto cmds = tex.tex().commands();
  REQUIRE(has_token(cmds.tokens(), "\\section"));
  REQUIRE(has_token(cmds.tokens(), "\\begin"));
  REQUIRE(has_token(cmds.tokens(), "\\ref"));

  // extract_environment("equation") is the sole survivor of what used to be
  // two names (the removed tex_extract_env_equation() convenience method was
  // a pure alias): it now always returns the full closed TokenBatch, not a
  // bare flattened Note.
  auto env_eq = tex.tex().extract_environment("equation");
  REQUIRE(env_eq.tokens().numel() >= 1);
  REQUIRE(env_eq.tokens().at(0).find("\\begin{equation}") != std::string::npos);

  auto refs = tex.tex().refs();
  REQUIRE(has_token(refs.tokens(), "eq:1"));
}

TEST_CASE("CodeN TeX command and conversion parsing respects lexical boundaries",
          "[coden][tex]") {
  const auto source = pluss::make_note({
      R"(\section* [Short] {A {nested} title} \alpha)",
      R"(\\beta \$ \frac{one)"});
  const auto bare = source.tex().commands(false);
  const auto complete = source.tex().commands(true);

  REQUIRE(bare.tokens().numel() == 3);
  REQUIRE(bare.tokens().at(0) == R"(\section*)");
  REQUIRE(complete.tokens().at(0) ==
          R"(\section* [Short] {A {nested} title})");
  const auto offsets = complete.offsets().integers();
  REQUIRE(offsets.size() == 3);
  REQUIRE(offsets[0] == 0);
  REQUIRE(offsets[1] == 2);
  REQUIRE(offsets[2] == 3);

  const auto expressions = pluss::make_note({
      R"(\cfrac{\frac{x}{y}}{z})", R"(\sqrt[3]{a+\sqrt{b}})",
      R"(\pm x \mp y \pmatrix \\pm)",
      R"(\frac*{x}{y} \sqrt*{x} \pm*)"});
  REQUIRE(expressions.tex().frac_slash().at(0) ==
          "((x)/(y))/(z)");
  REQUIRE(expressions.tex().sqrt_pow().at(1) ==
          "(a+(b)^(1/2))^(1/3)");
  REQUIRE(expressions.tex().pm_signs().at(2) ==
          R"(+/- x -/+ y \pmatrix \\pm)");
  REQUIRE(expressions.tex().frac_slash().at(3) == expressions.at(3));
  REQUIRE(expressions.tex().sqrt_pow().at(3) == expressions.at(3));
  REQUIRE(expressions.tex().pm_signs().at(3) == expressions.at(3));

  const auto lines = pluss::make_note("a\r\n \r\n\t\r\nb");
  REQUIRE(lines.tex().dedup_blank_lines().scalar() == "a\r\n\r\nb");
  REQUIRE(pluss::make_note("a\n\n\n  b").tex().dedup_blank_lines().scalar() ==
          "a\n\n  b");
}

TEST_CASE("CodeN NLP/tokenization extract family is concrete", "[coden][patch17]") {
  pluss::Note n = pluss::make_note(
      "OpenAI shipped GPT in 2025. Visit https://openai.com and email team@openai.com.\n"
      "NASA builds rockets.");

  auto urls = n.nlp().urls();
  REQUIRE(has_token(urls.tokens(), "https://openai.com"));

  auto acr = n.nlp().acronyms();
  REQUIRE(has_token(acr.tokens(), "GPT"));
  REQUIRE(has_token(acr.tokens(), "NASA"));

  auto grams = n.nlp().word_ngrams(2);
  REQUIRE(grams.tokens().numel() > 0);

  auto sents = n.nlp().sentences();
  REQUIRE(sents.tokens().numel() >= 2);

  auto found = n.regex().findall("[A-Z]{2,}");
  REQUIRE(found.tokens().numel() >= 2);

  auto joined = pluss::make_note({"OpenAI", "shipped", "GPT"}).join(" ");
  REQUIRE(joined.defined());
  REQUIRE(joined.is_scalar());
  REQUIRE(joined.scalar() == "OpenAI shipped GPT");
}

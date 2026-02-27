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

  auto cmds = tex.tex_commands();
  REQUIRE(has_token(cmds.tokens(), "\\section"));
  REQUIRE(has_token(cmds.tokens(), "\\begin"));
  REQUIRE(has_token(cmds.tokens(), "\\ref"));

  auto env_eq = tex.tex_extract_env_equation();
  REQUIRE(env_eq.tokens().numel() >= 1);
  REQUIRE(env_eq.tokens().at(0).find("\\begin{equation}") != std::string::npos);

  auto refs = tex.tex_refs();
  REQUIRE(has_token(refs.tokens(), "eq:1"));
}

TEST_CASE("CodeN NLP/tokenization extract family is concrete", "[coden][patch17]") {
  pluss::Note n = pluss::make_note(
      "OpenAI shipped GPT in 2025. Visit https://openai.com and email team@openai.com.\n"
      "NASA builds rockets.");

  auto urls = n.nlp_urls();
  REQUIRE(has_token(urls.tokens(), "https://openai.com"));

  auto acr = n.nlp_extract_acronyms();
  REQUIRE(has_token(acr.tokens(), "GPT"));
  REQUIRE(has_token(acr.tokens(), "NASA"));

  auto grams = n.nlp_word_ngrams(2);
  REQUIRE(grams.tokens().numel() > 0);

  auto sents = n.split_sentences();
  REQUIRE(sents.tokens().numel() >= 2);

  auto found = n.find_all_regex("[A-Z]{2,}");
  REQUIRE(found.tokens().numel() >= 2);
}

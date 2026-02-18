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

TEST_CASE("CodeN diff family uses concrete sequence diff", "[coden][patch19]") {
  pluss::Note a = pluss::content("alpha beta gamma");
  pluss::Note b = pluss::content("alpha theta gamma delta");

  auto dw = a.diff_words(b);
  REQUIRE(has_token(dw.tokens(), "-beta"));
  REQUIRE(has_token(dw.tokens(), "+theta"));
  REQUIRE(has_token(dw.tokens(), "+delta"));
}

TEST_CASE("CodeN align family emits tagged LCS alignment", "[coden][patch19]") {
  pluss::Note a = pluss::content("One. Two!");
  pluss::Note b = pluss::content("One. Three! Two!");

  auto sent = a.align_sentences(b);
  REQUIRE(has_token(sent.tokens(), "=One."));
  REQUIRE(has_token(sent.tokens(), "=Two!"));
  REQUIRE(has_token(sent.tokens(), "+Three!"));
}


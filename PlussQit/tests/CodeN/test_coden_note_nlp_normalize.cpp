#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

namespace {

std::vector<std::string> text(const pluss::Note& note) {
  std::vector<std::string> out;
  out.reserve(static_cast<std::size_t>(note.numel()));
  for (std::int64_t i = 0; i < note.numel(); ++i) {
    out.emplace_back(note.at(i));
  }
  return out;
}

} // namespace

TEST_CASE("Note NLP cleaning composes only its documented transforms",
          "[coden][nlp_normalize]") {
  const std::string dirty =
      " \t\xE2\x80\x9CShip\xE2\x80\x9D\xE2\x80\x94now\xE2\x80\xA6"
      "\xE2\x80\x8B\nnext\x01 ";
  REQUIRE(text(pluss::note::make({dirty}).nlp().clean()) ==
          std::vector<std::string>{"\"Ship\"-now... next"});

  const auto source = pluss::note::make({"  I CAN'T pay 1,250  "});
  REQUIRE(text(source.nlp().clean()) ==
          std::vector<std::string>{"I CAN'T pay 1,250"});
  REQUIRE(text(source.nlp().clean().nlp().clean()) == text(source.nlp().clean()));
}

TEST_CASE("Note contraction expansion handles boundaries casing and whitespace",
          "[coden][nlp_normalize]") {
  const auto source = pluss::note::make({
      "I can't; You're ready, but they didn't.",
      "CAN'T Won't I'M we're shell'code",
      "  it's\tstill\nthere  ",
  });
  REQUIRE(text(source.nlp().expand_contractions()) ==
          std::vector<std::string>{
              "I can not; You are ready, but they did not.",
              "CAN NOT Will not I AM we are shell'code",
              "  it is\tstill\nthere  ",
          });
  REQUIRE(text(source.nlp().expand_contractions().nlp().expand_contractions()) ==
          text(source.nlp().expand_contractions()));
}

TEST_CASE("Note number normalization uses complete ASCII numeric runs",
          "[coden][nlp_normalize]") {
  const auto source = pluss::note::make({
      "42 1,234.50 2024-01-02 +7 x8y",
      "1/2 3:45 6_000 1..2 and no digits",
  });
  REQUIRE(text(source.nlp().normalize_numbers()) ==
          std::vector<std::string>{
              "0 0 0 +0 x0y",
              "0 0 0 0..0 and no digits",
          });
  REQUIRE(text(source.nlp().normalize_numbers().nlp().normalize_numbers()) ==
          text(source.nlp().normalize_numbers()));
}

TEST_CASE("Note punctuation normalization exposes both call forms",
          "[coden][nlp_normalize]") {
  const auto source = pluss::note::make({
      "\xE2\x80\x9Cna\xC3\xAFve\xE2\x80\x9D\xE2\x80\x94wait\xE2\x80\xA6",
  });
  REQUIRE(text(source.nlp().normalize_punct()) ==
          std::vector<std::string>{"\"na\xC3\xAFve\"-wait..."});
  REQUIRE(text(source.nlp().normalize_punct(false)) ==
          text(source.nlp().normalize_punct()));
  REQUIRE(text(source.nlp().normalize_punct(true)) ==
          std::vector<std::string>{"\"na?ve\"-wait..."});
  REQUIRE(text(source.nlp().normalize_punct().nlp().normalize_punct()) ==
          text(source.nlp().normalize_punct()));
}

TEST_CASE("Note NLP normalizers preserve shape absence and Text dtype",
          "[coden][nlp_normalize]") {
  const auto matrix = pluss::note::make(
      std::vector<std::string>{" can't ", "12", "\xE2\x80\x9Cyes\xE2\x80\x9D", "plain"},
      std::array<std::int64_t, 2>{2, 2});
  REQUIRE(matrix.nlp().clean().sizes() == std::vector<std::int64_t>{2, 2});
  REQUIRE(matrix.nlp().expand_contractions().sizes() ==
          std::vector<std::int64_t>{2, 2});
  REQUIRE(matrix.nlp().normalize_numbers().sizes() ==
          std::vector<std::int64_t>{2, 2});
  REQUIRE(matrix.nlp().normalize_punct().sizes() ==
          std::vector<std::int64_t>{2, 2});

  REQUIRE_FALSE(pluss::Note::undefined().nlp().clean().defined());
  REQUIRE_FALSE(pluss::Note::undefined().nlp().expand_contractions().defined());
  REQUIRE_FALSE(pluss::Note::undefined().nlp().normalize_numbers().defined());
  REQUIRE_FALSE(pluss::Note::undefined().nlp().normalize_punct().defined());

  const auto empty = pluss::note::make(std::vector<std::string>{});
  REQUIRE(empty.nlp().clean().sizes() == std::vector<std::int64_t>{0});
  REQUIRE(empty.nlp().expand_contractions().sizes() ==
          std::vector<std::int64_t>{0});
  REQUIRE(empty.nlp().normalize_numbers().sizes() ==
          std::vector<std::int64_t>{0});
  REQUIRE(empty.nlp().normalize_punct().sizes() ==
          std::vector<std::int64_t>{0});

  const auto bytes = pluss::note::bytes(
      std::vector<std::string>{"12"}, std::array<std::int64_t, 1>{1});
  REQUIRE_THROWS(bytes.nlp().clean());
  REQUIRE_THROWS(bytes.nlp().expand_contractions());
  REQUIRE_THROWS(bytes.nlp().normalize_numbers());
  REQUIRE_THROWS(bytes.nlp().normalize_punct());
  REQUIRE_THROWS(pluss::note::make({true}).nlp().clean());
  REQUIRE_THROWS(pluss::note::make(std::vector<std::int64_t>{1}).nlp().clean());
  REQUIRE_THROWS(pluss::note::make(std::vector<double>{1.0}).nlp().clean());
}

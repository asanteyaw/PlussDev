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

TEST_CASE("Note boilerplate removal is line-oriented and boundary-aware",
          "[coden][nlp_remove_structure]") {
  const auto source = pluss::note::make({
      "Project note\nCopyright 2026 Pluss\nPrivacy Policy\n"
      "This copyrighted study remains.\nTerms of service apply\n"
      "Disclaimer: draft\nAll rights reserved",
  });
  REQUIRE(text(source.nlp().remove_boilerplate()) ==
          std::vector<std::string>{
              "Project note This copyrighted study remains."});

  const auto ordinary = pluss::note::make({
      " Privacy engineering\nand contractual terms remain unchanged. "});
  REQUIRE(text(ordinary.nlp().remove_boilerplate()) == text(ordinary));
  REQUIRE(text(source.nlp().remove_boilerplate().nlp().remove_boilerplate()) ==
          text(source.nlp().remove_boilerplate()));
}

TEST_CASE("Note bracket removal deletes balanced spans and retains malformed text",
          "[coden][nlp_remove_structure]") {
  const auto source = pluss::note::make({
      "Keep (draft [old]) final {meta}; literal unmatched (kept",
      "Nested [one [two] three] end; closing ] stays",
  });
  REQUIRE(text(source.nlp().remove_brackets()) ==
          std::vector<std::string>{
              "Keep final ; literal unmatched (kept",
              "Nested end; closing ] stays",
          });
  const auto malformed = pluss::note::make({"  unmatched ( text  "});
  REQUIRE(text(malformed.nlp().remove_brackets()) == text(malformed));
}

TEST_CASE("Note bullet removal recognizes markers without damaging prose",
          "[coden][nlp_remove_structure]") {
  const auto source = pluss::note::make({
      "- alpha\n* beta\n+ gamma\n1. first\n2) second\n"
      "• round\n‣ triangle\n⁃ hyphen\n-word\n3.14 value\nÉclair",
  });
  REQUIRE(text(source.nlp().remove_bullets()) ==
          std::vector<std::string>{
              "alpha beta gamma first second round triangle hyphen -word 3.14 value Éclair"});
  REQUIRE(text(pluss::note::make({"• standalone"}).nlp().remove_bullets()) ==
          std::vector<std::string>{"standalone"});
  const auto prose = pluss::note::make({"First line.\n-word stays"});
  REQUIRE(text(prose.nlp().remove_bullets()) == text(prose));
}

TEST_CASE("Note code removal handles inline fenced and indented forms",
          "[coden][nlp_remove_structure]") {
  const auto source = pluss::note::make({
      "Keep `inline()` here.\n```cpp\nint x;\n```\n"
      "    indented();\n\talso_indented();\nplain",
  });
  REQUIRE(text(source.nlp().remove_code()) ==
          std::vector<std::string>{"Keep here. plain"});
  REQUIRE(text(pluss::note::make({"    hidden();\nvisible"})
                   .nlp().remove_code()) ==
          std::vector<std::string>{"visible"});
  REQUIRE(text(pluss::note::make({"~~~\nhidden();\n~~~\nvisible"})
                   .nlp().remove_code()) ==
          std::vector<std::string>{"visible"});
  const auto unmatched = pluss::note::make({"  an unmatched `\nremains  "});
  REQUIRE(text(unmatched.nlp().remove_code()) == text(unmatched));
}

TEST_CASE("Note structural removers preserve shape absence and Text dtype",
          "[coden][nlp_remove_structure]") {
  const auto matrix = pluss::note::make(
      std::vector<std::string>{"Copyright 2026", "a (draft)",
                               "- item", "use `x` now"},
      std::array<std::int64_t, 2>{2, 2});
  REQUIRE(matrix.nlp().remove_boilerplate().sizes() ==
          std::vector<std::int64_t>{2, 2});
  REQUIRE(matrix.nlp().remove_brackets().sizes() ==
          std::vector<std::int64_t>{2, 2});
  REQUIRE(matrix.nlp().remove_bullets().sizes() ==
          std::vector<std::int64_t>{2, 2});
  REQUIRE(matrix.nlp().remove_code().sizes() ==
          std::vector<std::int64_t>{2, 2});

  const auto empty = pluss::note::make(std::vector<std::string>{});
  REQUIRE(empty.nlp().remove_boilerplate().sizes() ==
          std::vector<std::int64_t>{0});
  REQUIRE(empty.nlp().remove_brackets().sizes() ==
          std::vector<std::int64_t>{0});
  REQUIRE(empty.nlp().remove_bullets().sizes() ==
          std::vector<std::int64_t>{0});
  REQUIRE(empty.nlp().remove_code().sizes() ==
          std::vector<std::int64_t>{0});

  REQUIRE_FALSE(pluss::Note::undefined().nlp().remove_boilerplate().defined());
  REQUIRE_FALSE(pluss::Note::undefined().nlp().remove_brackets().defined());
  REQUIRE_FALSE(pluss::Note::undefined().nlp().remove_bullets().defined());
  REQUIRE_FALSE(pluss::Note::undefined().nlp().remove_code().defined());

  const auto bytes = pluss::note::bytes(
      std::vector<std::string>{"- item"},
      std::array<std::int64_t, 1>{1});
  REQUIRE_THROWS(bytes.nlp().remove_boilerplate());
  REQUIRE_THROWS(bytes.nlp().remove_brackets());
  REQUIRE_THROWS(bytes.nlp().remove_bullets());
  REQUIRE_THROWS(bytes.nlp().remove_code());
  REQUIRE_THROWS(pluss::note::make({true}).nlp().remove_code());
  REQUIRE_THROWS(pluss::note::make(std::vector<std::int64_t>{1})
                     .nlp().remove_code());
  REQUIRE_THROWS(pluss::note::make(std::vector<double>{1.0})
                     .nlp().remove_code());
}

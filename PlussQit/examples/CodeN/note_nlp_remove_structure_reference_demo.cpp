#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void show(std::string_view label, const pluss::Note& note) {
  std::cout << label << '\n' << pluss::note::print(note) << '\n';
}

template <typename Fn>
void rejected(std::string_view label, Fn&& fn) {
  try {
    fn();
    std::cout << label << ": accepted\n";
  } catch (const pluss::Error&) {
    std::cout << label << ": rejected\n";
  }
}

} // namespace

int main() {
  std::cout << "=== boilerplate ===\n";
  const auto boilerplate = pluss::note::make({
      "Project note\nCopyright 2026 Pluss\nPrivacy Policy\n"
      "This copyrighted study remains.\nTerms of service apply\n"
      "Disclaimer: draft\nAll rights reserved",
  });
  show("recognized lines", boilerplate.nlp().remove_boilerplate());
  show("ordinary vocabulary",
       pluss::note::make({
           " Privacy engineering\nand contractual terms remain unchanged. "})
           .nlp().remove_boilerplate());

  std::cout << "\n=== brackets ===\n";
  show("balanced and unmatched",
       pluss::note::make({
           "Keep (draft [old]) final {meta}; literal unmatched (kept",
           "Nested [one [two] three] end; closing ] stays"})
           .nlp().remove_brackets());
  show("only unmatched",
       pluss::note::make({"  unmatched ( text  "}).nlp().remove_brackets());

  std::cout << "\n=== bullets ===\n";
  show("unordered ordered and Unicode",
       pluss::note::make({
           "- alpha\n* beta\n+ gamma\n1. first\n2) second\n"
           "• round\n‣ triangle\n⁃ hyphen\n-word\n3.14 value\nÉclair"})
           .nlp().remove_bullets());
  show("standalone Unicode",
       pluss::note::make({"• standalone"}).nlp().remove_bullets());
  show("ordinary lines",
       pluss::note::make({"First line.\n-word stays"})
           .nlp().remove_bullets());

  std::cout << "\n=== code ===\n";
  show("inline fenced and indented",
       pluss::note::make({
           "Keep `inline()` here.\n```cpp\nint x;\n```\n"
           "    indented();\n\talso_indented();\nplain"})
           .nlp().remove_code());
  show("first-line indent",
       pluss::note::make({"    hidden();\nvisible"}).nlp().remove_code());
  show("tilde fence",
       pluss::note::make({"~~~\nhidden();\n~~~\nvisible"})
           .nlp().remove_code());
  show("unmatched delimiter",
       pluss::note::make({"  an unmatched `\nremains  "}).nlp().remove_code());

  std::cout << "\n=== shared contract ===\n";
  const auto matrix = pluss::note::make(
      std::vector<std::string>{"Copyright 2026", "a (draft)",
                               "- item", "use `x` now"},
      std::array<std::int64_t, 2>{2, 2});
  show("matrix boilerplate", matrix.nlp().remove_boilerplate());
  show("matrix brackets", matrix.nlp().remove_brackets());
  show("matrix bullets", matrix.nlp().remove_bullets());
  show("matrix code", matrix.nlp().remove_code());

  const auto once = boilerplate.nlp().remove_boilerplate();
  std::cout << "boilerplate idempotent=" << std::boolalpha
            << (once.nlp().remove_boilerplate() == once).all().bool_item()
            << '\n';
  show("empty", pluss::note::make(std::vector<std::string>{})
                    .nlp().remove_code());
  std::cout << "undefined defined=" << std::boolalpha
            << pluss::Note::undefined().nlp().remove_code().defined() << '\n';

  const auto bytes = pluss::note::bytes(
      std::vector<std::string>{"- item"},
      std::array<std::int64_t, 1>{1});
  rejected("Bytes", [&] { (void)bytes.nlp().remove_code(); });
  rejected("Bool", [&] {
    (void)pluss::note::make({true}).nlp().remove_code();
  });
  rejected("Int64", [&] {
    (void)pluss::note::make(std::vector<std::int64_t>{1})
        .nlp().remove_code();
  });
  rejected("Float64", [&] {
    (void)pluss::note::make(std::vector<double>{1.0})
        .nlp().remove_code();
  });
}

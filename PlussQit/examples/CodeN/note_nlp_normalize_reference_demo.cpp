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
  std::cout << "=== clean ===\n";
  const std::string dirty =
      " \t\xE2\x80\x9CShip\xE2\x80\x9D\xE2\x80\x94now\xE2\x80\xA6"
      "\xE2\x80\x8B\nnext\x01 ";
  show("composed cleanup", pluss::note::make({dirty}).nlp().clean());
  show("content retained",
       pluss::note::make({"  I CAN'T pay 1,250  "}).nlp().clean());

  std::cout << "\n=== contractions ===\n";
  show("grammar and case",
       pluss::note::make({"I can't; You're ready, but they didn't.",
                          "CAN'T Won't I'M we're shell'code"})
           .nlp().expand_contractions());
  show("whitespace retained",
       pluss::note::make({"  it's\tstill\nthere  "})
           .nlp().expand_contractions());

  std::cout << "\n=== numbers ===\n";
  show("numeric runs",
       pluss::note::make({"42 1,234.50 2024-01-02 +7 x8y"})
           .nlp().normalize_numbers());
  show("separator boundaries",
       pluss::note::make({"1/2 3:45 6_000 1..2 and no digits"})
           .nlp().normalize_numbers());
  const auto canonical_number =
      pluss::note::make({"Build 1847 costs $12.50"})
          .nlp().normalize_numbers();
  std::cout << "numbers idempotent=" << std::boolalpha
            << (canonical_number.nlp().normalize_numbers() == canonical_number)
                   .all().bool_item()
            << '\n';

  std::cout << "\n=== punctuation ===\n";
  const auto punct = pluss::note::make({
      "\xE2\x80\x9Cna\xC3\xAFve\xE2\x80\x9D\xE2\x80\x94wait\xE2\x80\xA6"});
  show("default", punct.nlp().normalize_punct());
  show("explicit false", punct.nlp().normalize_punct(false));
  show("ASCII only", punct.nlp().normalize_punct(true));

  std::cout << "\n=== shared contract ===\n";
  const auto matrix = pluss::note::make(
      std::vector<std::string>{" can't ", "12", "\xE2\x80\x9Cyes\xE2\x80\x9D", "plain"},
      std::array<std::int64_t, 2>{2, 2});
  show("matrix clean", matrix.nlp().clean());
  show("matrix contractions", matrix.nlp().expand_contractions());
  show("matrix numbers", matrix.nlp().normalize_numbers());
  show("matrix punctuation", matrix.nlp().normalize_punct());

  const auto normalized = matrix.nlp().clean();
  std::cout << "clean idempotent=" << std::boolalpha
            << (normalized.nlp().clean() == normalized).all().bool_item() << '\n';
  show("empty", pluss::note::make(std::vector<std::string>{}).nlp().clean());
  std::cout << "undefined defined=" << std::boolalpha
            << pluss::Note::undefined().nlp().clean().defined() << '\n';

  const auto bytes = pluss::note::bytes(
      std::vector<std::string>{"12"}, std::array<std::int64_t, 1>{1});
  rejected("Bytes", [&] { (void)bytes.nlp().clean(); });
  rejected("Bool", [&] { (void)pluss::note::make({true}).nlp().clean(); });
  rejected("Int64", [&] {
    (void)pluss::note::make(std::vector<std::int64_t>{1}).nlp().clean();
  });
  rejected("Float64", [&] {
    (void)pluss::note::make(std::vector<double>{1.0}).nlp().clean();
  });
}

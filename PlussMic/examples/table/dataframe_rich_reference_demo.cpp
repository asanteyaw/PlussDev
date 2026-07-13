#include <iostream>
#include <string_view>

#include <pluss/pluss.h>

namespace {

void print_block(std::string_view label, const std::string& text) {
  std::cout << label << "\n" << text << "\n\n";
}

} // namespace

int main() {
  using namespace pluss::table;

  auto flags = DataFrame({
      {"quiz_ok", pluss::make_series(std::vector<bool>{true, true, false})},
      {"exam_ok", pluss::make_series(std::vector<bool>{true, false, false})},
      {"bonus_ok", pluss::make_series(std::vector<bool>{true, true, true})},
  }).title("flags");

  auto sales = DataFrame({
      {"city", pluss::make_note({"Accra", "Tema", "Kumasi"})},
      {"region", pluss::make_note({"south", "south", "middle"})},
      {"units", pluss::make_series({10.0, 7.0, 12.0})},
      {"price", pluss::make_series({3.0, 4.0, 2.5})},
  }, pluss::make_note({"s1", "s2", "s3"})).title("sales");

  auto people = DataFrame({
      {"First Name", pluss::make_note({"Laurel", "Bola", "Laurel"})},
      {"Last Name", pluss::make_note({"Ork", "Mensah", "Kirk"})},
      {"Age", pluss::make_series({21.0, 19.0, 23.0})},
  }).title("people");

  print_block("ANY_FEATURE", flags.any().to_string());
  print_block("ANY_CASE", flags.any(Axis::Case).to_string());

  print_block("QUERY_EXPR",
              sales.query("units >= 10 and region == 'south'").to_string());
  print_block("QUERY_BACKTICK",
              people.query("`First Name` == 'Laurel'").to_string());
  print_block("QUERY_FEATURE_COMPARE",
              sales.query("units > price").to_string());

  return 0;
}

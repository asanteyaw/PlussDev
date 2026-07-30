#include <iostream>
#include <limits>
#include <string>

#include <pluss/pluss.h>

namespace {

void print_block(std::string_view label, const std::string& text) {
  std::cout << label << "\n" << text << "\n\n";
}

} // namespace

int main() {
  using namespace pluss::table;
  using pluss::ScalarType;
  using pluss::make_note;
  using pluss::make_series;
  using pluss::make_timeline;

  auto names = Thread(make_note({"Ada", "BOLA", "cHeN"}));
  auto code_names = Thread(make_note({"A-001", "B-002", "C-003"}));
  auto scores = Thread(make_series({10.0, 20.0, 30.0, 40.0}));
  auto scores_other = Thread(make_series({10.0, 10.0, 35.0, 45.0}));
  auto with_labels = scores.label().set(Thread(make_note({"a", "b", "c", "d"})));
  auto with_name = scores.name("scores");
  auto missing = Thread(make_series({
      10.0,
      std::numeric_limits<double>::quiet_NaN(),
      30.0,
      std::numeric_limits<double>::quiet_NaN(),
      50.0,
  }));
  auto duplicated = Thread(make_note({"Accra", "Tema", "Accra", "Kumasi", "Tema"}));
  auto dates = Thread(make_timeline({
      "2024-01-01T09:10:00Z",
      "2024-02-01T11:00:00Z",
      "2024-03-05T15:30:00Z",
  }));

  print_block("CASEFOLD", names.casefold().to_string());

  print_block("CAST_SERIES_DTYPE",
              Thread(make_note({"1.0", "2.0", "3.0"}))
                  .cast({.series_dtype = ScalarType::Float64})
                  .series()
                  .to_string());

  ThreadCastOptions cast_options;
  cast_options.series_dtype = ScalarType::Float64;
  cast_options.invalid = ParseInvalid::Null;
  print_block("CAST_OPTIONS",
              Thread(make_note({"4.0", "oops", "6.0"}))
                  .cast(cast_options)
                  .series()
                  .to_string());

  print_block("CHARS_RANGE", code_names.chars(0, 1).to_string());
  print_block("CHARS_STEP", code_names.chars(0, std::nullopt, 2).to_string());

  print_block("CLEAR_CASES", with_labels.label().reset().to_string());
  print_block("UNNAME", with_name.unname().to_string());
  print_block("CLIP", scores.clip(15.0, 35.0).to_string());

  print_block("COMPARE_SCALAR",
              scores.compare(20.0, ThreadBinaryCompareOp::Ge).to_string());
  print_block("COMPARE_STRING",
              names.compare("BOLA", ThreadBinaryCompareOp::Eq).to_string());
  print_block("COMPARE_THREAD",
              scores.compare(scores_other, ThreadBinaryCompareOp::Ne).to_string());

  print_block("CONTAINS", names.nt().has("A").to_string());

  std::cout << "CORR_PEARSON\n" << scores.corr(scores_other, ThreadCorrMethod::Pearson) << "\n\n";
  std::cout << "CORR_SPEARMAN\n" << scores.corr(scores_other, ThreadCorrMethod::Spearman) << "\n\n";
  std::cout << "CORR_WITH\n" << scores.corr_with(scores_other, ThreadCorrMethod::Pearson, 1).to_str() << "\n\n";

  std::cout << "COUNT_TRUE\n" << Thread(make_series(std::vector<bool>{true, false, true, true})).count(true).to_str() << "\n\n";
  std::cout << "COUNT_FALSE\n" << Thread(make_series(std::vector<bool>{true, false, true, true})).count(false).to_str() << "\n\n";
  std::cout << "COVAR\n" << scores.covar(scores_other).to_str() << "\n\n";

  print_block("DAY", dates.tl().day().to_string());
  std::cout << "DEFINED\n" << (scores.defined() ? "true" : "false") << "\n\n";
  print_block("DESCRIBE_SERIES", scores.describe(false).to_string());
  print_block("DESCRIBE_NOTE", duplicated.describe(false).to_string());

  print_block("CASES", with_labels.label().values().to_string());
  std::cout << "NAME\n" << with_name.name() << "\n\n";

  print_block("DISTINCT", duplicated.distinct(true).to_string());
  print_block("DROPNULL", missing.dropnull().to_string());
  std::cout << "DTYPE\n" << scores.dtype() << "\n\n";

  print_block("DUPLICATED_DEFAULT", duplicated.duplicated(true, false).to_string());
  print_block("DUPLICATED_KEEP_LAST", duplicated.duplicated(false, true).to_string());
  print_block("REPEATED", duplicated.repeated().to_string());

  return 0;
}

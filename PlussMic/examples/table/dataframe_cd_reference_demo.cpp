#include <iostream>
#include <limits>
#include <string>

#include <pluss/pluss.h>

namespace {

void print_block(std::string_view label, const std::string& text) {
  std::cout << label << "\n" << text << "\n\n";
}

void print_dtypes(std::string_view label, const pluss::table::DataFrame::DTypes& dtypes) {
  std::cout << label << "\n";
  for (const auto& [feature, dtype] : dtypes.entries()) {
    std::cout << feature << ": " << dtype << "\n";
  }
  std::cout << "\n";
}

} // namespace

int main() {
  using namespace pluss::table;
  using pluss::ScalarType;
  using pluss::make_note;
  using pluss::make_series;
  using pluss::make_timeline;

  auto sales = DataFrame({
      {"city", make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
      {"region", make_note({"south", "south", "middle", "north"})},
      {"units", make_series({10.0, 7.0, 12.0, 5.0})},
      {"price", make_series({3.0, 4.0, 2.5, 5.0})},
  }).title("sales");

  auto labeled = sales.labels().set(Slate(make_note({"A100", "B200", "B200", "D400"})));

  auto numeric_cases = DataFrame({
      {"student", make_note({"Ada", "Bola", "Chen"})},
      {"score", make_series({88.0, 79.0, 91.0})},
  }, make_series({101.0, 102.0, 103.0})).title("numeric_cases");

  auto raw = DataFrame({
      {"student_id", make_series({101.0, 102.0, 103.0})},
      {"passed", make_series({1.0, 0.0, 1.0})},
      {"raw_name", make_note({"Ada", "Bola", "Chen"})},
  }).title("raw");

  auto extra = DataFrame({
      {"region", make_note({"south", "south", "middle", "north"})},
      {"discount", make_series({0.5, 0.75, 0.25, 1.0})},
  }, make_note({"A100", "B200", "B200", "D400"})).title("extra");

  auto patch = DataFrame({
      {"units", make_series({70.0, 50.0})},
      {"price", make_series({6.0, 9.0})},
  }, make_note({"B200", "D400"})).title("patch");

  auto left = DataFrame({
      {"math", make_series({80.0, 90.0, 70.0})},
      {"science", make_series({75.0, 88.0, 92.0})},
  }, make_note({"s1", "s2", "s3"})).title("left");

  auto right = DataFrame({
      {"science", make_series({92.0, 75.0})},
      {"math", make_series({70.0, 80.0})},
  }, make_note({"s3", "s1"})).title("right");

  auto corr_base = DataFrame({
      {"x", make_series({1.0, 2.0, 3.0, 4.0})},
      {"y", make_series({2.0, 4.0, 6.0, 8.0})},
      {"z", make_series({4.0, 1.0, 3.0, 2.0})},
      {"city", make_note({"A", "B", "C", "D"})},
  }).title("corr_base");

  auto corr_peer = DataFrame({
      {"x", make_series({2.0, 3.0, 4.0, 5.0})},
      {"y", make_series({4.0, 5.0, 7.0, 9.0})},
      {"z", make_series({3.0, 2.0, 4.0, 1.0})},
  }).title("corr_peer");

  auto corr_peer_2 = DataFrame({
      {"x", make_series({5.0, 4.0, 3.0, 2.0})},
      {"y", make_series({8.0, 6.0, 4.0, 2.0})},
      {"z", make_series({1.0, 4.0, 2.0, 3.0})},
  }).title("corr_peer_2");

  auto cities = DataFrame({
      {"city", make_note({"Accra", "Tema", "Accra", "Kumasi", "Accra", "Tema"})},
      {"score", make_series({10.0, 20.0, 11.0, 30.0, 12.0, 21.0})},
  }).title("cities");

  auto levels = DataFrame({
      {"campus", make_note({"north", "north", "south", "south"})},
      {"term", make_note({"fall", "spring", "fall", "spring"})},
      {"score", make_series({80.0, 82.0, 78.0, 88.0})},
  }).labels().set({"campus", "term"}, true).title("levels");

  auto duplicates = DataFrame({
      {"student", make_note({"Ada", "Bola", "Ada", "Chen", "Bola"})},
      {"quiz", make_series({10.0, 9.0, 10.0, 7.0, 9.0})},
      {"final", make_series({80.0, 75.0, 80.0, 90.0, 76.0})},
  }).title("duplicates");

  auto missing = DataFrame({
      {"student", make_note({"Ada", "Bola", "Chen", "Dede"})},
      {"quiz", make_series({10.0, std::numeric_limits<double>::quiet_NaN(), 8.0,
                             std::numeric_limits<double>::quiet_NaN()})},
      {"final", make_series({80.0, 75.0, std::numeric_limits<double>::quiet_NaN(), 90.0})},
  }).title("missing");

  print_block("CASE_STUDY_POS", labeled.case_study().at(1).to_string());
  print_block("CASE_STUDY_LABELS", labeled.case_study().labels({"B200", "A100"}).to_string());
  print_block("CASE_STUDY_MASK_FEATURES",
              labeled.case_study().where(labeled.features("region") == "south")
                  .features({"city", "price"}).to_string());

  print_block("CASES", labeled.labels().values().to_string());
  print_block("LABEL_SET_NOTE",
              sales.labels().set(make_note({"row-a", "row-b", "row-c", "row-d"})).to_string());
  try {
    (void)sales.labels().set(make_note({"row-a", "row-b"}));
  } catch (const std::exception& e) {
    const std::string message(e.what());
    std::cout << "LABEL_SET_NOTE_SIZE_ERROR\n"
              << message.substr(0, message.find('\n')) << "\n\n";
  }
  print_block("CASES_AS_SERIES", Slate(numeric_cases.labels().values().as_series()).to_string());

  print_block("CAST_DTYPES", raw.astype({
      {"student_id", ScalarType::Int64},
      {"passed", ScalarType::Bool},
  }).to_string());

  print_block("CAST_SPEC", raw.features({"raw_name"}).cast().note().to_string());

  print_block("COMBINE_FEATURES", labeled.combine(extra).to_string());
  DataFrame::CombineSpec combine_update;
  combine_update.op = DataFrame::CombineOp::Update;
  combine_update.overwrite = false;
  combine_update.cases = FrameMatchRule::KeepLeft;
  print_block("COMBINE_UPDATE", labeled.combine(patch, combine_update).to_string());

  print_block("COMPARE_SCALAR", labeled.features({"units", "price"}).compare(5.0).to_string());
  print_block("COMPARE_FRAME", left.compare(DataFrame({
      {"math", make_series({80.0, 91.0, 70.0})},
      {"science", make_series({75.0, 80.0, 92.0})},
  }, make_note({"s1", "s2", "s3"}))).to_string());

  print_block("CONFORM", left.conform(right).to_string());
  print_block("COPY", left.copy().to_string());

  print_block("CORR_DEFAULT", corr_base.corr(false).to_string());
  print_block("CORR_METHOD", corr_base.corr(DataFrame::CorrMethod::Spearman, false).to_string());
  print_block("CORR_FEATURES", corr_base.features({"x", "z"}).corr(false).to_string());
  print_block("CORR_FEATURES_METHOD",
              corr_base.features({"x", "z"}).corr(DataFrame::CorrMethod::KendallTau, false).to_string());

  print_block("CORR_WITH_ONE",
              corr_base.corr_with(corr_peer, DataFrame::CorrMethod::Pearson, 1).to_string());
  print_block("CORR_WITH_MANY",
              corr_base.corr_with({corr_peer, corr_peer_2},
                                  DataFrame::CorrMethod::Spearman,
                                  1).to_string());

  print_block("COUNT_FEATURE_AXIS", corr_base.count(Axis::Feature).to_string());
  print_block("COUNT_CASE_AXIS", corr_base.features({"x", "y", "z"}).count(Axis::Case).to_string());

  const auto city_counts = cities.group({.dropnull = true})
                               .by({"city"})
                               .agg({
                                   {"count", "", GroupOp::Count},
                               })
                               .arrange({"count"}, false);
  print_block("COUNT_BY_ALL", city_counts.to_string());
  print_block("COUNT_BY_TOP2", city_counts.top("count", 2).to_string());

  print_block("COVAR_DEFAULT", corr_base.covar({}, false).to_string());
  print_block("CSMAX_CASE", corr_base.features({"x", "z"}).csmax(Axis::Case).to_string());
  print_block("CSMIN_FEATURE", corr_base.features({"x", "z"}).csmin(Axis::Feature).to_string());

  std::cout << "DEFINED\n" << (corr_base.defined() ? "true" : "false") << "\n\n";
  print_block("DESCRIBE", corr_base.describe(false).to_string());

  print_block("DIV_SCALAR", labeled.features({"units", "price"}).div(2.0).to_string());
  print_block("DIV_FRAME", left.div(DataFrame({
      {"math", make_series({10.0, 10.0, 10.0})},
      {"science", make_series({5.0, 8.0, 4.0})},
  }, make_note({"s1", "s2", "s3"}))).to_string());

  print_block("DROP", labeled.drop({"region"}).to_string());
  print_block("DROP_DUPLICATES_DEFAULT", duplicates.features({"student", "quiz"}).drop_duplicates().to_string());
  print_block("DROP_DUPLICATES_KEEP_LAST",
              duplicates.features({"student"}).drop_duplicates(false, true).to_string());

  print_block("DROP_LEVELS", levels.labels().drop({"term"}).to_string());
  print_block("DROPNULL", missing.dropnull({"quiz", "final"}).to_string());

  print_dtypes("DTYPES", raw.dtypes());

  print_block("DUPLICATED_DEFAULT", duplicates.features({"student", "quiz"}).duplicated().to_string());
  print_block("DUPLICATED_KEEP_LAST", duplicates.features({"student"}).duplicated(false, true).to_string());

  return 0;
}

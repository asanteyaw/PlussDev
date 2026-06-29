#include <iostream>
#include <limits>

#include <pluss/pluss.h>

namespace {

pluss::table::Thread total_grade(const pluss::table::DataFrame& df) {
  return df.features("assignment") + df.features("final");
}

} // namespace

int main() {
  using pluss::codem::ScalarType;
  using pluss::make_note;
  using pluss::make_series;
  using pluss::table::DataFrame;

  auto sales = DataFrame({
      {"city", make_note({"Accra", "Tema", "Kumasi"})},
      {"units", make_series({10.0, 7.0, 12.0})},
      {"price", make_series({3.0, 4.0, 2.5})},
  }).title("sales");

  auto left = DataFrame({
      {"midterm_1", make_series({15.0, 18.0, 12.0})},
      {"midterm_2", make_series({16.0, 14.0, 13.0})},
  }).title("left");

  auto curve = DataFrame({
      {"midterm_1", make_series({1.0, 1.0, 1.0})},
      {"midterm_2", make_series({0.5, 0.5, 0.5})},
  }).title("curve");

  auto grades = DataFrame({
      {"student", make_note({"Ada", "Bola", "Chen"})},
      {"assignment", make_series({9.5, 8.0, 7.5})},
      {"final", make_series({28.0, 24.0, 21.5})},
  }, make_note({"s1", "s2", "s3"})).title("grades");

  auto scratch = DataFrame({
      {"score", make_series({88.0, 79.0, 91.0})},
  }, make_note({"s2", "s3", "s1"})).title("scratch");

  auto flags = DataFrame({
      {"quiz_ok", make_series(std::vector<bool>{true, true, false})},
      {"exam_ok", make_series(std::vector<bool>{true, false, false})},
      {"bonus_ok", make_series(std::vector<bool>{true, true, true})},
  }).title("flags");

  auto raw = DataFrame({
      {"student_id", make_series({101.0, 102.0, 103.0})},
      {"passed", make_series({1.0, 0.0, 1.0})},
  }).title("raw");

  auto missing_scores = DataFrame({
      {"score", make_series({72.0, std::numeric_limits<double>::quiet_NaN(), 81.0, 90.0})},
  }).title("missing_scores");

  auto ticks = DataFrame({
      {"stamp", pluss::make_timeline({
          "2024-01-01T00:10:00Z",
          "2024-01-01T00:20:00Z",
          "2024-01-01T01:10:00Z",
          "2024-01-02T00:15:00Z",
      })},
      {"load", make_series({1.0, std::numeric_limits<double>::quiet_NaN(), 3.0, 4.0})},
      {"city", make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
  }).title("ticks");

  std::cout << "ADD_SCALAR\n" << sales.add(1.0).to_string() << "\n\n";
  std::cout << "ADD_FRAME\n" << left.add(curve).to_string() << "\n\n";
  std::cout << "ADD_PREFIX\n" << sales.features({"units", "price"}).add_prefix("exam_").to_string() << "\n\n";
  std::cout << "ADD_SUFFIX\n" << sales.features({"units", "price"}).add_suffix("_winter").to_string() << "\n\n";

  std::cout << "AGGREGATE_ONE\n";
  std::cout << grades.aggregate({
      {"mean_final", "final", DataFrame::SummaryOp::Mean},
  }, false).to_string() << "\n\n";

  std::cout << "AGGREGATE_MULTI\n";
  std::cout << grades.aggregate({
      {"n_students", "final", DataFrame::SummaryOp::Count},
      {"max_final", "final", DataFrame::SummaryOp::Max},
      {"min_final", "final", DataFrame::SummaryOp::Min},
  }, false).to_string() << "\n\n";

  std::cout << "AGGREGATE_AUTO\n";
  std::cout << grades.aggregate({
      {"", "assignment", DataFrame::SummaryOp::Mean},
      {"", "assignment", DataFrame::SummaryOp::Std},
  }, false).to_string() << "\n\n";

  std::cout << "ALIGN_CASES\n" << scratch.align_cases(grades.cases()).to_string() << "\n\n";

  std::cout << "ALL_AXIS0\n" << flags.all().to_string() << "\n\n";
  std::cout << "ALL_AXIS1\n" << flags.all(1).to_string() << "\n\n";
  std::cout << "ANY_AXIS0\n" << flags.any().to_string() << "\n\n";
  std::cout << "ANY_AXIS1\n" << flags.any(1).to_string() << "\n\n";

  std::cout << "ASSIGN_THREAD\n";
  std::cout << grades.assign({
      std::pair<std::string, pluss::table::Thread>{"total", total_grade(grades)},
  }).to_string() << "\n\n";

  std::cout << "ASSIGN_SPEC\n";
  std::cout << grades.assign({
      DataFrame::DeriveSpec{
          .feature = "total",
          .expr = total_grade,
      },
      DataFrame::DeriveSpec{
          .feature = "passed",
          .expr = [](const DataFrame& df) {
            return df.features("total") >= 30.0;
          },
      },
  }).to_string() << "\n\n";

  std::cout << "ASTYPE\n" << raw.astype({"passed"}, ScalarType::Bool).to_string() << "\n\n";
  std::cout << "AT\n" << grades.at(1, "student") << "\n\n";

  std::cout << "BFILL\n" << missing_scores.bfill().to_string() << "\n\n";
  std::cout << "BOTTOM\n" << grades.bottom("final", 2).to_string() << "\n";
  std::cout << "\nRESHUFFLE_ROWS\n" << grades.reshuffle(0, 7).to_string() << "\n\n";
  std::cout << "RESHUFFLE_FEATURES\n" << grades.reshuffle(1, 7).to_string() << "\n\n";
  std::cout << "RESAMPLE_COUNT\n" << ticks.resample("stamp", {"load", "city"}, "D", "count", 1, false).to_string() << "\n\n";
  std::cout << "RESAMPLE_SIZE\n" << ticks.resample("stamp", {"load", "city"}, "D", "size", 1, false).to_string() << "\n\n";
  std::cout << "RESAMPLE_MEAN\n" << ticks.resample("stamp", {"load"}, "D", "mean", 1, false).to_string() << "\n";

  return 0;
}

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <pluss/pluss.h>
#include <pluss/tils.h>

namespace {

std::filesystem::path data_file(const std::string& name) {
  return std::filesystem::path(PLUSSMIC_SOURCE_DIR) / "data" / name;
}

} // namespace

int main() {
  pluss::loader::ReadCsvOptions options;
  options.sep = ';';
  options.ncases = 32;
  options.usefeats = {"age", "job", "balance", "duration"};
  auto df = pluss::table::DataFrame::load_csv(data_file("bank-full.csv").string(), options);

  std::cout << "=== versatile families demo ===\n";
  std::cout << "input\n";
  df.head(8);

  const auto ages = df.features("age").as_series().to_vec<std::int64_t>();
  auto mask = pluss::Series::zeros({static_cast<std::int64_t>(ages.size())}, pluss::codem::ScalarType::Bool);
  auto* keep = reinterpret_cast<bool*>(mask.raw_data());
  for (std::size_t i = 0; i < ages.size(); ++i) {
    keep[i] = ages[i] >= 45;
  }

  std::cout << "\nquery(mask): age >= 45\n";
  auto older = df.filter(mask);
  older.head(8);

  std::cout << "\nquery(string): age >= 45 and job != 'unknown'\n";
  auto older_known = df.query("age >= 45 and job != 'unknown'");
  older_known.head(8);

  std::cout << "\nquery(string): age between 45 and 50\n";
  auto mid_band = df.query("age between 45 and 50");
  mid_band.head(8);

  std::cout << "\nquery(string): job in ('management', 'technician')\n";
  auto core_jobs = df.query("job in ('management', 'technician')");
  core_jobs.head(8);

  std::cout << "\nderive(spec): add is_senior\n";
  auto flagged = df.assign({{
      "is_senior",
      [](const pluss::table::DataFrame& frame) {
        const auto values = frame.features("age").as_series().to_vec<std::int64_t>();
        auto out = pluss::Series::zeros({static_cast<std::int64_t>(values.size())}, pluss::codem::ScalarType::Bool);
        auto* p = reinterpret_cast<bool*>(out.raw_data());
        for (std::size_t i = 0; i < values.size(); ++i) {
          p[i] = values[i] >= 50;
        }
        return pluss::table::Thread(out);
      },
  }});
  flagged.head(8);

  std::cout << "\ntransform: all numeric +1 (Note features pass-through)\n";
  auto transformed_all = df.transform([](const pluss::table::Thread& feature) {
    if (feature.kind() == pluss::table::ThreadKind::Series) {
      return feature + 1.0;
    }
    return feature;
  });
  transformed_all.head(8);

  std::cout << "\ntransform: targeted per-feature transforms\n";
  auto transformed_targeted = df.transform({
      {"age", [](const pluss::table::Thread& feature) { return feature * 2.0; }},
      {"balance", [](const pluss::table::Thread& feature) { return feature + 100.0; }},
  });
  transformed_targeted.head(8);

  std::cout << "\nsummarize specs\n";
  flagged.aggregate(
      {
          {"cases", "age", pluss::table::DataFrame::SummaryOp::Count},
          {"age_mean", "age", pluss::table::DataFrame::SummaryOp::Mean},
          {"job_cardinality", "job", pluss::table::DataFrame::SummaryOp::Nunique},
      },
      true);

  std::cout << "\naggregate\n";
  flagged.aggregate({{"balance_max", "balance", pluss::table::DataFrame::SummaryOp::Max}}, true);

  auto numeric = df.features({"age", "balance", "duration"});
  std::cout << "\narithmetic: add/subtract/multiply/divide\n";
  numeric.add(1.0).head(5);
  numeric.subtract(1.0).head(5);
  numeric.multiply(2.0).head(5);
  numeric.divide(2.0).head(5);

  std::cout << "\nreverse arithmetic: rsub/rpow\n";
  numeric.rsub(100.0).head(5);
  numeric.rpow(2.0).head(5);

  std::cout << "\ncomparisons: ge/eq/compare\n";
  numeric.ge(45.0).head(5);
  numeric.eq(numeric).head(5);
  numeric.compare(numeric.add(1.0)).head(5);

  std::cout << "\narrange: single + multi-feature sort\n";
  df.sort_values({"age"}).head(8);
  df.sort_values({
      {"age", true, true},
      {"duration", false, true},
  }).head(8);

  std::cout << "\ncast / astype\n";
  df.astype({{"age", pluss::codem::ScalarType::Float64}}).head(8);
  df.cast({{
      "age",
      pluss::table::ThreadCastTarget::Note,
      std::nullopt,
  }}).head(8);

  std::cout << "\nwindow / rolling\n";
  numeric.rolling(
             {{
                 "age_roll_mean",
                 "age",
                 pluss::table::ThreadWindowOp::RollingMean,
                 3,
                 2,
             }},
             false)
      .head(8);
  numeric.rolling(
             {{
                 "",
                 "duration",
                 pluss::table::ThreadWindowOp::RollingMax,
                 5,
                 1,
             }},
             true)
      .head(8);

  std::cout << "\nmissing: isnull / fill / ffill / bfill / drop\n";
  auto missing_demo = pluss::table::DataFrame::load_csv(
      data_file("messy_data.csv").string(),
      {
          {"ncases", 16},
          {"features", {"Business Case ID", "Agency Name", "Lifecycle Cost"}},
      });
  missing_demo.head(8);
  missing_demo.isna().head(8);
  missing_demo.fillna(0.0, {"Lifecycle Cost"}).head(8);
  missing_demo.ffill().head(8);
  missing_demo.bfill().head(8);
  missing_demo.dropna().head(8);

  std::cout << "\ntranspose\n";
  df.features({"age", "job", "balance"}).transpose(true);

  return 0;
}

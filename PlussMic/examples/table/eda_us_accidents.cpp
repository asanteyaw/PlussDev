#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

#include <pluss/pluss.h>

namespace {

std::filesystem::path data_file(const std::string& name) {
  return std::filesystem::path(PLUSSMIC_SOURCE_DIR) / "data" / name;
}

} // namespace

int main(int argc, char** argv) {
  auto csv_path = data_file("US_Accidents_Dec20.csv").string();

  pluss::table::DataFrame df;
  if (argc > 1) {
    const auto n = std::stoll(argv[1]);
    df = n > 0
        ? pluss::table::DataFrame::load_csv(csv_path, {{"ncases", static_cast<std::int64_t>(n)}})
        : pluss::table::DataFrame::load_csv(csv_path);
  } else {
    df = pluss::table::DataFrame::load_csv(csv_path);
  }

  std::cout << "Loaded: [" << df.ncases() << " cs x " << df.nfeats() << " feat]\n";
  std::cout << "\n=== Initial Exploration ===\n";
  df.info();
  std::cout << "\n";
  df.head();
  std::cout << "\n";
  df.tail();
  std::cout << "\n";
  df.describe();

  auto work = df
      .cast_timeline({"Start_Time", "End_Time"})
      .astype({
          "Severity",
          "Distance(mi)",
          "Start_Lat",
          "Start_Lng",
          "Visibility(mi)",
          "Wind_Speed(mph)",
          "Humidity(%)",
          "Pressure(in)",
          "Precipitation(in)",
      }, pluss::codem::ScalarType::Float64)
      .assign({
          pluss::table::DataFrame::DeriveSpec{"Start_Year", [](const pluss::table::DataFrame& f) {
             return pluss::table::Thread(f.features("Start_Time").as_timeline().year());
           }},
          pluss::table::DataFrame::DeriveSpec{"Start_Month", [](const pluss::table::DataFrame& f) {
             return pluss::table::Thread(f.features("Start_Time").as_timeline().month());
           }},
          pluss::table::DataFrame::DeriveSpec{"Start_Day", [](const pluss::table::DataFrame& f) {
             return pluss::table::Thread(f.features("Start_Time").as_timeline().day());
           }},
          pluss::table::DataFrame::DeriveSpec{"Start_Hour", [](const pluss::table::DataFrame& f) {
             return pluss::table::Thread(f.features("Start_Time").as_timeline().hour());
           }},
          pluss::table::DataFrame::DeriveSpec{"Start_DOW", [](const pluss::table::DataFrame& f) {
             return pluss::table::Thread(f.features("Start_Time").as_timeline().to_note_weekday());
           }},
          pluss::table::DataFrame::DeriveSpec{"Duration_min", [](const pluss::table::DataFrame& f) {
             auto end_ns = f.features("End_Time")
                               .cast(pluss::table::ThreadCastTarget::Series, pluss::codem::ScalarType::Int64)
                               .as_series()
                               .to(pluss::codem::ScalarType::Float64);
             auto start_ns = f.features("Start_Time")
                                 .cast(pluss::table::ThreadCastTarget::Series, pluss::codem::ScalarType::Int64)
                                 .as_series()
                                 .to(pluss::codem::ScalarType::Float64);
             return pluss::table::Thread(end_ns.sub(start_ns).div(6.0e10));
           }},
          pluss::table::DataFrame::DeriveSpec{"Sev_ge_3", [](const pluss::table::DataFrame& f) {
             return pluss::table::Thread(f.features("Severity").as_series().ge(3.0));
           }},
          pluss::table::DataFrame::DeriveSpec{"Is_RushHour", [](const pluss::table::DataFrame& f) {
             const auto h = f.features("Start_Hour").as_series().to_vec<std::int64_t>();
             auto out = pluss::Series::zeros({static_cast<std::int64_t>(h.size())}, pluss::codem::ScalarType::Bool);
             auto* p = reinterpret_cast<bool*>(out.raw_data());
             for (std::size_t i = 0; i < h.size(); ++i) {
               const auto hh = h[i];
               p[i] = ((hh >= 7 && hh <= 9) || (hh >= 16 && hh <= 18));
             }
             return pluss::table::Thread(out);
           }},
      });

  std::cout << "\nSplit by dtype groups:\n";
  work.note_features().head(3);
  work.series_features().head(3);
  work.timeline_features().head(3);

  std::cout << "\n// =========== Question One==========\n";
  std::cout << "// What are the top 5 states with the most accidents?\n";
  work.count_by("State", 5);

  std::cout << "\n// =========== Question Two==========\n";
  std::cout << "// What are the top 10 cities with the most accidents?\n";
  work.count_by("City", 10);

  std::cout << "\n// =========== Question Three==========\n";
  std::cout << "// What counties have the highest accident rates?\n";
  work.count_by("County", 10);

  std::cout << "\n// =========== Question Four==========\n";
  std::cout << "// Which states have the highest accident severity on average?\n";
  work.mean_by("State", "Severity", 10, 1000);

  std::cout << "\n// =========== Question Five==========\n";
  std::cout << "// What is the distribution of accidents across time zones?\n";
  work.count_by("Timezone", 20);

  std::cout << "\n// =========== Question Six==========\n";
  std::cout << "// Which zip codes have the highest accident density?\n";
  work.count_by("Zipcode", 20);

  std::cout << "\n// =========== Question Seven==========\n";
  std::cout << "// Are accidents more frequent in urban or rural areas?\n";
  std::cout << "Urban proxy (City notnull): " << work.query("City notnull").ncases() << "\n";
  std::cout << "Rural proxy (City isnull): " << work.query("isnull(City)").ncases() << "\n";

  std::cout << "\n// =========== Question Eight==========\n";
  std::cout << "// Which airports are associated with the most accidents nearby?\n";
  work.count_by("Airport_Code", 10);

  std::cout << "\n// =========== Question Nine==========\n";
  std::cout << "// What is the geographic clustering pattern (heatmap) of accidents?\n";
  std::cout << "Not concise yet (needs first-class binning helper for geo heatmaps).\n";

  std::cout << "\n// =========== Question Ten==========\n";
  std::cout << "// Are accidents concentrated along certain highways (Street analysis)?\n";
  work.count_by("Street", 20);

  std::cout << "\n// =========== Question Eleven==========\n";
  std::cout << "// What year had the most accidents?\n";
  work.count_by("Start_Year", 20);

  std::cout << "\n// =========== Question Twelve==========\n";
  std::cout << "// What month has the highest accident frequency?\n";
  work.count_by("Start_Month", 12);

  std::cout << "\n// =========== Question Thirteen==========\n";
  std::cout << "// Which day of the week has the most accidents?\n";
  work.count_by("Start_DOW", 7);

  std::cout << "\n// =========== Question Fourteen==========\n";
  std::cout << "// What time of day sees the most accidents?\n";
  work.count_by("Start_Hour", 24);

  std::cout << "\n// =========== Question Fifteen==========\n";
  std::cout << "// Are accidents more common during rush hours?\n";
  work.count_by("Is_RushHour", 2);

  std::cout << "\n// =========== Question Eighteen==========\n";
  std::cout << "// What is the average accident duration (minutes)?\n";
  work.aggregate({{"avg_duration_min", "Duration_min", pluss::table::DataFrame::SummaryOp::Mean}});

  std::cout << "\n// =========== Question Twenty==========\n";
  std::cout << "// Are nighttime accidents more severe than daytime accidents?\n";
  work.mean_by("Sunrise_Sunset", "Severity", 8, 1);

  std::cout << "\n// =========== Question Twenty One==========\n";
  std::cout << "// What is the distribution of accident severity levels?\n";
  work.count_by("Severity", 10);

  std::cout << "\n// =========== Question Twenty Two==========\n";
  std::cout << "// Which states have the highest proportion of severity >= 3 accidents?\n";
  work.mean_by("State", "Sev_ge_3", 10, 1000);

  std::cout << "\n// =========== Question Thirty One==========\n";
  std::cout << "// What weather condition is most associated with accidents?\n";
  work.count_by("Weather_Condition", 10);

  std::cout << "\n// =========== Question Thirty Nine==========\n";
  std::cout << "// Are accidents more frequent at Night vs Day (Sunrise_Sunset)?\n";
  work.count_by("Sunrise_Sunset", 10);

  std::cout << "\n// =========== Question Forty Two==========\n";
  std::cout << "// Are accidents more common near traffic signals?\n";
  work.count_by("Traffic_Signal", 10);

  std::cout << "\n// =========== Question Fifty==========\n";
  std::cout << "// Can we predict accident severity using weather + time + infrastructure variables?\n";
  std::cout << "Not yet: needs ML/train-test/predict verbs integration.\n";

  return 0;
}

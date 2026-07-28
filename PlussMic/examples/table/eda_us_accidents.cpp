#include <algorithm>
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

  std::cout << "Loaded: [" << df.ncases() << " cs x " << df.nfeatures() << " feat]\n";
  std::cout << "\n=== Initial Exploration ===\n";
  df.info();
  std::cout << "\n";
  df.head();
  std::cout << "\n";
  df.tail();
  std::cout << "\n";
  df.describe();

  auto work = df;
  work.set({"Start_Time", "End_Time"}) =
      df.features({"Start_Time", "End_Time"}).cast().timeline();
  work = work.features({
          "Severity",
          "Distance(mi)",
          "Start_Lat",
          "Start_Lng",
          "Visibility(mi)",
          "Wind_Speed(mph)",
          "Humidity(%)",
          "Pressure(in)",
          "Precipitation(in)",
      }).astype(pluss::codem::ScalarType::Float64)
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
             return pluss::table::Thread(f.features("Start_Time").as_timeline().strftime("%A"));
           }},
          pluss::table::DataFrame::DeriveSpec{"Duration_min", [](const pluss::table::DataFrame& f) {
             auto end_ns = f.features("End_Time")
                               .cast({.series_dtype = pluss::codem::ScalarType::Int64})
                               .series()
                               .as_series()
                               .to(pluss::codem::ScalarType::Float64);
             auto start_ns = f.features("Start_Time")
                                 .cast({.series_dtype = pluss::codem::ScalarType::Int64})
                                 .series()
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
  work.feataxis().note().head(3);
  work.feataxis().series().head(3);
  work.feataxis().timeline().head(3);

  const auto show_counts = [&](std::string_view key, std::int64_t top_n, bool dropnull = true) {
    auto grouped = work.group({.dropnull = dropnull})
                       .by({key})
                       .agg({
                           {"count", "", pluss::table::DataFrame::GroupOp::Count},
                       })
                       .arrange({"count"}, false);
    const auto render_n = top_n > 0 ? std::min<std::int64_t>(top_n, grouped.ncases()) : grouped.ncases();
    grouped.head(render_n, true);
  };

  const auto show_mean = [&](std::string_view key,
                             std::string_view value,
                             std::int64_t top_n,
                             std::int64_t min_count,
                             bool dropnull = true) {
    const auto mean_name = std::string("mean_") + std::string(value);
    auto grouped = work.group({.dropnull = dropnull})
                       .by({key})
                       .agg({
                           {mean_name, std::string(value), pluss::table::DataFrame::GroupOp::Mean},
                           {"count", std::string(value), pluss::table::DataFrame::GroupOp::Count},
                       });
    grouped = grouped.filter(grouped.features("count").ge(static_cast<double>(min_count)))
                  .arrange({mean_name}, false);
    const auto render_n = top_n > 0 ? std::min<std::int64_t>(top_n, grouped.ncases()) : grouped.ncases();
    grouped.head(render_n, true);
  };

  std::cout << "\n// =========== Question One==========\n";
  std::cout << "// What are the top 5 states with the most accidents?\n";
  show_counts("State", 5);

  std::cout << "\n// =========== Question Two==========\n";
  std::cout << "// What are the top 10 cities with the most accidents?\n";
  show_counts("City", 10);

  std::cout << "\n// =========== Question Three==========\n";
  std::cout << "// What counties have the highest accident rates?\n";
  show_counts("County", 10);

  std::cout << "\n// =========== Question Four==========\n";
  std::cout << "// Which states have the highest accident severity on average?\n";
  show_mean("State", "Severity", 10, 1000);

  std::cout << "\n// =========== Question Five==========\n";
  std::cout << "// What is the distribution of accidents across time zones?\n";
  show_counts("Timezone", 20);

  std::cout << "\n// =========== Question Six==========\n";
  std::cout << "// Which zip codes have the highest accident density?\n";
  show_counts("Zipcode", 20);

  std::cout << "\n// =========== Question Seven==========\n";
  std::cout << "// Are accidents more frequent in urban or rural areas?\n";
  std::cout << "Urban proxy (City notnull): " << work.query("City notnull").ncases() << "\n";
  std::cout << "Rural proxy (City isnull): " << work.query("isnull(City)").ncases() << "\n";

  std::cout << "\n// =========== Question Eight==========\n";
  std::cout << "// Which airports are associated with the most accidents nearby?\n";
  show_counts("Airport_Code", 10);

  std::cout << "\n// =========== Question Nine==========\n";
  std::cout << "// What is the geographic clustering pattern (heatmap) of accidents?\n";
  std::cout << "Not concise yet (needs first-class binning helper for geo heatmaps).\n";

  std::cout << "\n// =========== Question Ten==========\n";
  std::cout << "// Are accidents concentrated along certain highways (Street analysis)?\n";
  show_counts("Street", 20);

  std::cout << "\n// =========== Question Eleven==========\n";
  std::cout << "// What year had the most accidents?\n";
  show_counts("Start_Year", 20);

  std::cout << "\n// =========== Question Twelve==========\n";
  std::cout << "// What month has the highest accident frequency?\n";
  show_counts("Start_Month", 12);

  std::cout << "\n// =========== Question Thirteen==========\n";
  std::cout << "// Which day of the week has the most accidents?\n";
  show_counts("Start_DOW", 7);

  std::cout << "\n// =========== Question Fourteen==========\n";
  std::cout << "// What time of day sees the most accidents?\n";
  show_counts("Start_Hour", 24);

  std::cout << "\n// =========== Question Fifteen==========\n";
  std::cout << "// Are accidents more common during rush hours?\n";
  show_counts("Is_RushHour", 2);

  std::cout << "\n// =========== Question Eighteen==========\n";
  std::cout << "// What is the average accident duration (minutes)?\n";
  work.aggregate({{"avg_duration_min", "Duration_min", pluss::table::DataFrame::SummaryOp::Mean}});

  std::cout << "\n// =========== Question Twenty==========\n";
  std::cout << "// Are nighttime accidents more severe than daytime accidents?\n";
  show_mean("Sunrise_Sunset", "Severity", 8, 1);

  std::cout << "\n// =========== Question Twenty One==========\n";
  std::cout << "// What is the distribution of accident severity levels?\n";
  show_counts("Severity", 10);

  std::cout << "\n// =========== Question Twenty Two==========\n";
  std::cout << "// Which states have the highest proportion of severity >= 3 accidents?\n";
  show_mean("State", "Sev_ge_3", 10, 1000);

  std::cout << "\n// =========== Question Thirty One==========\n";
  std::cout << "// What weather condition is most associated with accidents?\n";
  show_counts("Weather_Condition", 10);

  std::cout << "\n// =========== Question Thirty Nine==========\n";
  std::cout << "// Are accidents more frequent at Night vs Day (Sunrise_Sunset)?\n";
  show_counts("Sunrise_Sunset", 10);

  std::cout << "\n// =========== Question Forty Two==========\n";
  std::cout << "// Are accidents more common near traffic signals?\n";
  show_counts("Traffic_Signal", 10);

  std::cout << "\n// =========== Question Fifty==========\n";
  std::cout << "// Can we predict accident severity using weather + time + infrastructure variables?\n";
  std::cout << "Not yet: needs ML/train-test/predict verbs integration.\n";

  return 0;
}

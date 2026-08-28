#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <fmt/format.h>

#include <pluss/pluss.h>

namespace {

using pluss::table::DataFrame;
using pluss::table::Thread;

DataFrame make_time_series_df() {
  constexpr std::int64_t ncases = 50;

  std::vector<std::string> ts_labels;
  ts_labels.reserve(static_cast<std::size_t>(ncases));
  for (std::int64_t i = 0; i < ncases; ++i) {
    const auto day = 1 + static_cast<int>(i / 24);
    const auto hour = static_cast<int>(i % 24);
    ts_labels.push_back(fmt::format("2026-01-{:02d} {:02d}:00:00", day, hour));
  }

  std::vector<double> load_mw;
  std::vector<double> speed_mph;
  std::vector<std::int64_t> volume;
  load_mw.reserve(static_cast<std::size_t>(ncases));
  speed_mph.reserve(static_cast<std::size_t>(ncases));
  volume.reserve(static_cast<std::size_t>(ncases));

  for (std::int64_t i = 0; i < ncases; ++i) {
    load_mw.push_back(180.0 + static_cast<double>((i * 7) % 60));
    speed_mph.push_back(35.0 + static_cast<double>((i * 3) % 25));
    volume.push_back(900 + i * 11);
  }

  return DataFrame({
      {"event_time", Thread(pluss::make_timeline(ts_labels, std::vector<std::int64_t>{ncases}))},
      {"load_mw", Thread(pluss::make_series(load_mw, std::vector<std::int64_t>{ncases}))},
      {"speed_mph", Thread(pluss::make_series(speed_mph, std::vector<std::int64_t>{ncases}))},
      {"volume", Thread(pluss::make_series(volume, std::vector<std::int64_t>{ncases}))},
  });
}

DataFrame make_note_df() {
  const std::vector<std::string> city = {
      "Accra", "Kumasi", "Tamale", "Tema", "Cape Coast", "Ho", "Takoradi", "Sunyani",
  };
  const std::vector<std::string> comment = {
      "Fog at dawn", "Rain started", "Road clear", "Heavy traffic",
      "Signal issue", "Dry roads", "Low visibility", "Calm evening",
  };
  const std::vector<double> score = {0.95, 0.72, 0.88, 0.67, 0.41, 0.89, 0.53, 0.91};

  constexpr std::int64_t ncases = 8;
  return DataFrame({
      {"city", Thread(pluss::make_note(city, std::vector<std::int64_t>{ncases}))},
      {"comment", Thread(pluss::make_note(comment, std::vector<std::int64_t>{ncases}))},
      {"score", Thread(pluss::make_series(score, std::vector<std::int64_t>{ncases}))},
  });
}

} // namespace

int main() {
  std::cout << "=== manual time-series DataFrame (50 cs) ===\n";
  auto ts = make_time_series_df();
  ts.head(8);
  ts.info();

  std::cout << "df.mean() over numeric features:\n";
  std::cout << ts.mean() << "\n";

  auto ts_enriched = ts.assign({
      {"year", [](const DataFrame& df) { return df.features("event_time").as_timeline().year(); }},
      {"hour", [](const DataFrame& df) { return df.features("event_time").as_timeline().hour(); }},
  });
  std::cout << "features(\"event_time\").as_timeline().year()/hour() used to derive year/hour:\n";
  ts_enriched.features({"event_time", "year", "hour", "load_mw"}).head(8);

  auto ts_idx = ts.features({"event_time"}).cast().timeindex();
  auto bins = ts_idx.features("event_time").as_timeindex().resample_count("H", 6);
  std::cout << "features(\"event_time\").as_timeindex().resample_count(\"H\", 6):\n";
  std::cout << pluss::series::print(bins) << "\n";

  std::cout << "features({\"load_mw\", \"speed_mph\"}).ts({.axis = features(\"event_time\")}).resample(\"6H\").mean():\n";
  ts_idx.features({"load_mw", "speed_mph"})
      .ts(pluss::table::TsOptions{.axis = ts_idx.features("event_time")})
      .resample("6H")
      .mean()
      .show();

  std::cout << "\n=== manual Note/DataFrame demo ===\n";
  auto txt = make_note_df();
  txt.head(8);
  txt.info();

  auto txt_aug = txt;
  txt_aug.set_("city_lower", txt.features("city").as_note().lower());
  txt_aug.set_("comment_tagged", txt.features("comment").as_note().replace(" ", "_"));

  std::cout << "features(\"...\").as_note() chain derived features:\n";
  txt_aug.features({"city", "city_lower", "comment", "comment_tagged", "score"}).head(8);

  return 0;
}

#include "pluss/codeh/timeline_factory.h"
#include <pluss/mic.h>

#include <array>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

using pluss::table::Atlas;
using pluss::table::Axis;
using pluss::table::DataFrame;
using pluss::table::ParseInvalid;
using pluss::table::Slate;
using pluss::table::SlateCastOptions;
using pluss::table::SlateCastTarget;
using pluss::table::TimeUnit;
using pluss::table::TimelineLocalizeOptions;
using pluss::table::WindowOptions;
using pluss::table::WindowStatistic;

int main() {
  const std::vector<std::string_view> raw_dates{
      "2026-01-01T00:00:00Z",
      "2026-01-02T06:30:00Z",
      "2026-02-03T12:45:00Z",
      "2026-03-04T18:15:00Z",
  };
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(raw_dates.size())};
  const auto event_time = pluss::codeh::ops_internal::timeline_from_utf8(
      std::span<const std::string_view>(raw_dates.data(), raw_dates.size()),
      std::span<const std::int64_t>(shape.data(), shape.size()));

  DataFrame df({
      {"case_id", Slate(pluss::make_series(std::vector<std::int64_t>{101, 102, 103, 104}))},
      {"event_time", Slate(event_time)},
      {"event_date_text", Slate(pluss::make_note({"2026-01-01", "bad-date", "2026-02-03", "2026-03-04"}))},
      {"revenue", Slate(pluss::make_series(std::vector<double>{10.25, 22.50, 31.75, 45.00}))},
      {"cost", Slate(pluss::make_series(std::vector<double>{4.00, 9.50, 14.25, 20.00}))},
      {"region", Slate(pluss::make_note({"north", "south", "north", "west"}))},
  });

  std::cout << "Original frame\n" << df.head(4) << "\n";

  auto renamed_one = df.rename("case_id", "account");
  auto renamed_many = renamed_one.rename({
      {"revenue", "sales"},
      {"cost", "expense"},
  });
  auto parsed_dates = renamed_many
                          .features({"event_date_text"})
                          .cast(SlateCastOptions{.format = "%Y-%m-%d", .invalid = ParseInvalid::Null})
                          .timeline();
  std::cout << "Note surface parsing text dates into Timeline with invalid-as-null\n"
            << parsed_dates.head(4) << "\n";

  auto renamed_all = renamed_many.rename({"account", "event_time", "event_date_text", "sales", "expense", "region"});
  std::cout << "Rename overloads\n" << renamed_all.head(4) << "\n";

  auto margin = renamed_all.features("sales") - renamed_all.features("expense");
  renamed_all.set("margin") = margin;
  std::cout << "Slate arithmetic result\n" << renamed_all.features("margin") << "\n";

  auto scaled = renamed_all.features({"sales", "expense", "margin"}).mul(2.0);
  auto compared = renamed_all.features({"sales", "expense", "margin"}).gt(20.0);
  auto frame_pair = renamed_all.features({"sales", "expense", "margin"})
                        .add(scaled.features({"sales", "expense", "margin"}));
  std::cout << "Batched scalar math\n" << scaled.head(4) << "\n";
  std::cout << "Batched scalar compare\n" << compared.head(4) << "\n";
  std::cout << "Batched frame math\n" << frame_pair.head(4) << "\n";

  auto reduced_by_feature = renamed_all.features({"sales", "expense", "margin"}).sum(Axis::Case);
  auto reduced_by_case = renamed_all.features({"sales", "expense", "margin"}).sum(Axis::Feature);
  std::cout << "DF reduction to Slate over features\n" << reduced_by_feature << "\n";
  std::cout << "DF reduction to Slate over cases\n" << reduced_by_case << "\n";
  std::cout << "Slate reduction to SlateValue\n" << renamed_all.features("sales").sum() << "\n";

  auto event_slate = renamed_all.features("event_time");
  std::cout << "Slate timeline year\n" << event_slate.tl().year() << "\n";
  std::cout << "Slate timeline ISO text\n" << event_slate.tl().iso() << "\n";
  std::cout << "Slate timeline epoch seconds\n" << event_slate.tl().epoch(TimeUnit::Seconds) << "\n";
  std::cout << "Slate timeline min scalar\n" << event_slate.min() << "\n";
  std::cout << "Slate timeline max scalar\n" << event_slate.max() << "\n";

  auto calendar = renamed_all.features({"event_time"}).tl().year()
      .rename({std::pair<std::string_view, std::string_view>{"event_time", "event_year"}});
  auto rounded = renamed_all.features({"event_time"}).tl().floor("D");
  
  auto localized = renamed_all.features({"event_time"}).tl().localize(TimelineLocalizeOptions{.tz = "UTC", .fold = 0, .nonexistent = "raise"});
  std::cout << "DataFrame tl year\n" << calendar.head(4) << "\n";
  std::cout << "DataFrame tl floor\n" << rounded.head(4) << "\n";
  std::cout << "DataFrame tl localize with option struct\n" << localized.head(4) << "\n";
  
  auto hierarchical_cases = renamed_all.label().set({"region", "account"}, false, " / ");
  std::cout << "Composite case labels from multiple features\n" << hierarchical_cases.head(4) << "\n";

  auto time_indexed = renamed_all.label().set("event_time", false);
  auto values = time_indexed.features({"sales", "expense", "margin"});
  std::cout << "Selected DataFrame ts sort by timeline cases\n"
            << values.ts().sort().head(4) << "\n";
  std::cout << "Selected DataFrame ts moving mean\n"
            << values.ts().rolling(2, WindowStatistic::Mean, WindowOptions{.min_count = 1}).head(4) << "\n";
  std::cout << "Selected DataFrame ts seven-day rolling sum over timeline cases\n"
            << values.ts().rolling("7D", WindowStatistic::Sum).head(4) << "\n";
  std::cout << "Selected DataFrame ts growing variance\n"
            << values.ts().expanding(WindowStatistic::Var, {.ddof = 1, .min_count = 2}).head(4) << "\n";
  std::cout << "Selected DataFrame ts decay mean\n"
            << values.ts().ewm(0.4, WindowStatistic::Mean).head(4) << "\n";
  std::cout << "Cases-based DataFrame ts daily resample sum\n"
            << values.ts().resample("D").sum().head(8) << "\n";
  std::cout << "Cases-based DataFrame ts daily asfreq\n"
            << values.ts().asfreq("D").head(8) << "\n";

  Atlas atlas({time_indexed, time_indexed});
  std::cout << "Atlas tl month using selected datetime feature\n"
            << atlas.features({"event_time"}).tl().month().frame(0).head(4) << "\n";
  std::cout << "Atlas ts moving mean using selected numeric frame\n"
            << Atlas({values, values}).ts().rolling(2, WindowStatistic::Mean).frame(0).head(4) << "\n";
  std::cout << "Atlas ts seven-day rolling mean using selected numeric frame\n"
            << Atlas({values, values}).ts().rolling("7D", WindowStatistic::Mean).frame(0).head(4) << "\n";
  std::cout << "Atlas ts daily resample mean\n"
            << Atlas({values, values}).ts().resample("D").mean().frame(0).head(8) << "\n";

  return 0;
}

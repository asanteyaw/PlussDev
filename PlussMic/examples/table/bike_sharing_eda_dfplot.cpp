#include <array>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <pluss/pluss.h>

namespace {

namespace fs = std::filesystem;
using DataFrame = pluss::table::DataFrame;
using GroupOp = DataFrame::GroupOp;
namespace pl = pluss::pl;
namespace flash = pluss::pl::flash;

constexpr std::array<std::string_view, 4> kSeasonLabels{"spring", "summer", "fall", "winter"};
constexpr std::array<std::string_view, 4> kWeatherLabels{"Clear/Few clouds", "Mist/Cloudy", "Light Snow/Rain",
                                                          "Heavy Rain/Snow/Fog"};
constexpr std::array<std::string_view, 7> kWeekdayLabels{"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday",
                                                          "Friday", "Saturday"};
constexpr std::array<std::string_view, 2> kYearLabels{"2011", "2012"};
constexpr std::array<std::string_view, 12> kMonthLabels{"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                                         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

struct Cli {
  fs::path data_path;
  fs::path out_dir;
  bool show{false};
};

[[nodiscard]] fs::path default_data_path() {
  return fs::path(PLUSSMIC_SOURCE_DIR) / "data" / "hour.csv";
}

[[nodiscard]] fs::path default_output_dir() {
  return fs::path(PLUSSMIC_SOURCE_DIR) / "examples" / "outputs" / "bike_sharing_eda_dfplot";
}

[[nodiscard]] Cli parse_cli(int argc, char** argv) {
  Cli cli{default_data_path(), default_output_dir(), false};

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i] != nullptr ? argv[i] : "";
    if (arg == "--show") {
      cli.show = true;
      continue;
    }
    if (arg == "--data" && (i + 1) < argc) {
      cli.data_path = fs::path(argv[++i]);
      continue;
    }
    if (arg == "--out" && (i + 1) < argc) {
      cli.out_dir = fs::path(argv[++i]);
      continue;
    }
    std::cout << "Ignoring unknown argument: " << arg << "\n";
  }

  return cli;
}

template <std::size_t NCodes, std::size_t NLabels>
[[nodiscard]] DataFrame make_lookup_frame(std::string_view code_feature,
                                          std::string_view label_feature,
                                          const std::array<double, NCodes>& codes,
                                          const std::array<std::string_view, NLabels>& labels) {
  static_assert(NCodes == NLabels, "lookup frame: code/label cardinality mismatch");
  std::vector<std::string> mapped_labels;
  mapped_labels.reserve(NLabels);
  for (const auto label : labels) {
    mapped_labels.emplace_back(label);
  }

  return DataFrame({
      {std::string(code_feature), pluss::table::Slate(pluss::make_series(codes))},
      {std::string(label_feature), pluss::table::Slate(pluss::make_note(mapped_labels))},
  });
}

[[nodiscard]] DataFrame load_and_prepare(const fs::path& csv_path) {
  PLUSS_CHECK(fs::exists(csv_path), "bike_sharing_eda_dfplot: input CSV does not exist: " + csv_path.string());

  auto df = DataFrame::load_csv(csv_path.string())
                .rename("instant", "rec_id")
                .rename("dteday", "date")
                .rename("holiday", "is_holiday")
                .rename("workingday", "is_workingday")
                .rename("weathersit", "weather_condition")
                .rename("hum", "humidity")
                .rename("mnth", "month")
                .rename("cnt", "total_count")
                .rename("hr", "hour")
                .rename("yr", "year");
  df.set("date") = df.features("date").cast().timeline();
  df = df.astype({
                    {"rec_id", pluss::codem::ScalarType::Float64},
                    {"season", pluss::codem::ScalarType::Float64},
                    {"year", pluss::codem::ScalarType::Float64},
                    {"month", pluss::codem::ScalarType::Float64},
                    {"hour", pluss::codem::ScalarType::Float64},
                    {"is_holiday", pluss::codem::ScalarType::Float64},
                    {"weekday", pluss::codem::ScalarType::Float64},
                    {"is_workingday", pluss::codem::ScalarType::Float64},
                    {"weather_condition", pluss::codem::ScalarType::Float64},
                    {"temp", pluss::codem::ScalarType::Float64},
                    {"atemp", pluss::codem::ScalarType::Float64},
                    {"humidity", pluss::codem::ScalarType::Float64},
                    {"windspeed", pluss::codem::ScalarType::Float64},
                    {"casual", pluss::codem::ScalarType::Float64},
                    {"registered", pluss::codem::ScalarType::Float64},
                    {"total_count", pluss::codem::ScalarType::Float64},
                });

  static constexpr std::array<double, 4> kSeasonCodes{1.0, 2.0, 3.0, 4.0};
  static constexpr std::array<double, 4> kWeatherCodes{1.0, 2.0, 3.0, 4.0};
  static constexpr std::array<double, 7> kWeekdayCodes{0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
  static constexpr std::array<double, 2> kYearCodes{0.0, 1.0};
  static constexpr std::array<double, 12> kMonthCodes{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0};

  const auto season_lookup = make_lookup_frame("season", "season_name", kSeasonCodes, kSeasonLabels);
  const auto weather_lookup =
      make_lookup_frame("weather_condition", "weather_name", kWeatherCodes, kWeatherLabels);
  const auto weekday_lookup = make_lookup_frame("weekday", "weekday_name", kWeekdayCodes, kWeekdayLabels);
  const auto year_lookup = make_lookup_frame("year", "year_label", kYearCodes, kYearLabels);
  const auto month_lookup = make_lookup_frame("month", "month_name", kMonthCodes, kMonthLabels);

  df = df.unify("season", season_lookup, "season", "keep_left", "_lookup", "many_to_one")
           .unify("weather_condition", weather_lookup, "weather_condition", "keep_left", "_lookup", "many_to_one")
           .unify("weekday", weekday_lookup, "weekday", "keep_left", "_lookup", "many_to_one")
           .unify("year", year_lookup, "year", "keep_left", "_lookup", "many_to_one")
           .unify("month", month_lookup, "month", "keep_left", "_lookup", "many_to_one")
           .assign({
               DataFrame::DeriveSpec{"is_weekend", [](const DataFrame& frame) {
                                      const auto weekday = frame.features("weekday").as_series();
                                      return pluss::table::Slate(weekday.eq(0.0).logical_or(weekday.eq(6.0)));
                                    }},
           });

  return df;
}

[[nodiscard]] pluss::table::DataFramePlotOptions build_plot_options(std::optional<std::string> default_x = std::nullopt,
                                                                     std::optional<std::string> default_hue =
                                                                         std::nullopt) {
  pluss::table::DataFramePlotOptions options;
  options.default_x_feature = std::move(default_x);
  options.default_hue_feature = std::move(default_hue);
  options.default_bins = 24;
  options.width = 1400.0;
  options.height = 860.0;
  options.use_tight_layout = true;
  options.tight_layout_padding = 4.0;
  return options;
}

void save_and_optionally_show(const pluss::pl::Studio& figure,
                              const fs::path& out_dir,
                              std::string_view stem,
                              bool show_window) {
  const auto pdf_path = out_dir / (std::string(stem) + ".pdf");
  figure.save_pdf(pdf_path.string());
  std::cout << "Saved: " << pdf_path << "\n";

  if (show_window) {
    (void)figure.show(stem, 1400, 860, true);
  }
}

void clear_plot_outputs(const fs::path& out_dir) {
  if (!fs::exists(out_dir)) {
    return;
  }
  for (const auto& entry : fs::directory_iterator(out_dir)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    const auto ext = entry.path().extension();
    if (ext == ".png" || ext == ".pdf") {
      std::error_code ec;
      fs::remove(entry.path(), ec);
    }
  }
}

std::vector<double> slate_as_numeric(const pluss::table::Slate& slate) {
  switch (slate.kind()) {
    case pluss::table::SlateKind::Series:
      return slate.as_series().to(pluss::codem::ScalarType::Float64).host<double>();
    case pluss::table::SlateKind::Timeline: {
      const auto ticks = slate.as_timeline().ticks();
      std::vector<double> values;
      values.reserve(ticks.size());
      for (const auto tick : ticks) {
        values.push_back(static_cast<double>(tick));
      }
      return values;
    }
    case pluss::table::SlateKind::Note: {
      std::vector<double> values;
      const auto note = slate.as_note();
      values.reserve(static_cast<std::size_t>(note.numel()));
      note.for_each_flat([&](std::string_view text) {
        std::string token(text);
        char* end = nullptr;
        const auto parsed = std::strtod(token.c_str(), &end);
        const bool ok = (end != nullptr) && (end != token.c_str()) && (*end == '\0');
        values.push_back(ok ? parsed : std::numeric_limits<double>::quiet_NaN());
      });
      return values;
    }
    default:
      return {};
  }
}

std::vector<std::string> slate_as_labels(const pluss::table::Slate& slate) {
  const auto note_like =
      slate.kind() == pluss::table::SlateKind::Note ? slate : slate.cast().note();
  std::vector<std::string> labels;
  const auto note = note_like.as_note();
  labels.reserve(static_cast<std::size_t>(note.numel()));
  note.for_each_flat([&](std::string_view text) { labels.emplace_back(text); });
  return labels;
}

std::string compact_label(double value) {
  const auto rounded = std::round(value);
  if (std::abs(value - rounded) < 1e-9) {
    return std::to_string(static_cast<std::int64_t>(rounded));
  }
  auto text = std::to_string(value);
  while (!text.empty() && text.back() == '0') {
    text.pop_back();
  }
  if (!text.empty() && text.back() == '.') {
    text.pop_back();
  }
  return text;
}

void configure_pane(pl::Pane& pane, bool legend = true) {
  pane.grid(pl::GridStyle::Major);
  pane.legend(legend ? pl::KeyPlace::Auto : pl::KeyPlace::None);
}

[[nodiscard]] std::vector<std::string_view> label_views(const std::vector<std::string>& labels) {
  std::vector<std::string_view> out;
  out.reserve(labels.size());
  for (const auto& label : labels) {
    out.emplace_back(label);
  }
  return out;
}

pl::Studio make_registered_vs_casual_figure(const DataFrame& registered, const DataFrame& casual) {
  auto fig = pl::studio("03_registered_vs_casual_by_weekday");
  fig.theme(pl::themes::paper());
  fig.mosaic(1, 2, 18.0, 18.0);
  fig.tight_layout(true, 4.0);

  const auto x = slate_as_numeric(registered.labels().values());
  std::vector<std::string> x_labels;
  x_labels.reserve(x.size());
  for (const auto value : x) {
    x_labels.push_back(compact_label(value));
  }

  const auto draw_panel = [&](pl::Pane& pane, const DataFrame& frame) {
    configure_pane(pane, true);
    const auto x_label_views = label_views(x_labels);
    pane.x_ticks(x, pluss::msharp::RangeView::Of<std::string_view>(x_label_views));
    pane.x_label("Hour").y_label("Mean rentals");
    for (const auto weekday : kWeekdayLabels) {
      if (!frame.get(weekday).has_value()) {
        continue;
      }
      auto& trace = flash::lineplot(pane, x, slate_as_numeric(frame.features(weekday)));
      trace.label(std::string(weekday));
    }
  };

  auto& left = fig.pane2("Registered: mean hourly rentals");
  left.place(0, 0);
  draw_panel(left, registered);

  auto& right = fig.pane2("Casual: mean hourly rentals");
  right.place(0, 1);
  draw_panel(right, casual);

  fig.headline("Registered vs Casual by weekday");
  return fig;
}

void draw_grouped_bar_panel(pl::Pane& pane, const DataFrame& pivoted, std::string_view x_label) {
  configure_pane(pane, true);
  const auto x = slate_as_numeric(pivoted.labels().values());
  std::vector<std::string> x_labels;
  x_labels.reserve(x.size());
  for (const auto value : x) {
    x_labels.push_back(compact_label(value));
  }
  const auto x_label_views = label_views(x_labels);
  pane.x_ticks(x, pluss::msharp::RangeView::Of<std::string_view>(x_label_views));
  pane.x_label(std::string(x_label)).y_label("Mean rentals");

  constexpr double group_span = 0.8;
  constexpr std::size_t group_count = 4;
  for (std::size_t g = 0; g < group_count; ++g) {
    const auto season = kSeasonLabels[g];
    if (!pivoted.get(season).has_value()) {
      continue;
    }
    std::vector<double> centers(x.size(), 0.0);
    const double width = group_span / static_cast<double>(group_count);
    for (std::size_t i = 0; i < x.size(); ++i) {
      centers[i] = x[i] - 0.5 * group_span + width * (static_cast<double>(g) + 0.5);
    }
    auto& trace = flash::barplot(pane, centers, slate_as_numeric(pivoted.features(season)));
    trace.label(std::string(season));
  }
}

pl::Studio make_holiday_workingday_figure(const DataFrame& holiday_by_season,
                                          const DataFrame& workingday_by_season) {
  auto fig = pl::studio("09_holiday_workingday_by_season");
  fig.theme(pl::themes::paper());
  fig.mosaic(1, 2, 18.0, 18.0);
  fig.tight_layout(true, 4.0);

  auto& left = fig.pane2("Holiday vs rentals (by season)");
  left.place(0, 0);
  draw_grouped_bar_panel(left, holiday_by_season, "Holiday (0/1)");

  auto& right = fig.pane2("Working day vs rentals (by season)");
  right.place(0, 1);
  draw_grouped_bar_panel(right, workingday_by_season, "Working day (0/1)");

  fig.headline("Holiday and working day effects by season");
  return fig;
}

void draw_box_features(pl::Pane& pane,
                       const DataFrame& frame,
                       std::initializer_list<std::string_view> features,
                       std::string_view y_label) {
  configure_pane(pane, false);
  std::vector<double> groups;
  std::vector<double> values;
  groups.reserve(static_cast<std::size_t>(frame.ncases()) * features.size());
  values.reserve(static_cast<std::size_t>(frame.ncases()) * features.size());
  std::vector<std::string> labels;
  labels.reserve(features.size());

  std::size_t idx = 0;
  for (const auto feature : features) {
    labels.emplace_back(feature);
    const auto v = slate_as_numeric(frame.features(feature));
    for (const auto value : v) {
      if (!std::isfinite(value)) {
        continue;
      }
      groups.push_back(static_cast<double>(idx));
      values.push_back(value);
    }
    ++idx;
  }

  auto& trace = flash::boxplot(
      pane,
      pluss::make_series(groups, {static_cast<std::int64_t>(groups.size())}),
      pluss::make_series(values, {static_cast<std::int64_t>(values.size())}));
  trace.label("distribution");

  std::vector<double> ticks(labels.size(), 0.0);
  for (std::size_t i = 0; i < labels.size(); ++i) {
    ticks[i] = static_cast<double>(i);
  }
  const auto tick_label_views = label_views(labels);
  pane.x_ticks(ticks, pluss::msharp::RangeView::Of<std::string_view>(tick_label_views));
  pane.x_label("feature").y_label(std::string(y_label));
}

pl::Studio make_boxplot_distributions_figure(const DataFrame& frame) {
  auto fig = pl::studio("10_boxplots_distributions");
  fig.theme(pl::themes::paper());
  fig.mosaic(1, 2, 18.0, 18.0);
  fig.tight_layout(true, 4.0);

  auto& left = fig.pane2("Distribution: total vs casual vs registered");
  left.place(0, 0);
  draw_box_features(left, frame, {"total_count", "casual", "registered"}, "Count");

  auto& right = fig.pane2("Distribution: weather numeric features");
  right.place(0, 1);
  draw_box_features(right, frame, {"temp", "windspeed", "humidity"}, "Value");

  fig.headline("Distribution diagnostics");
  return fig;
}

void write_parity_report(const fs::path& out_dir) {
  const auto report_path = out_dir / "parity_report.md";
  std::ofstream out(report_path);
  out << "# Bike Sharing EDA Parity (Python -> df.plot)\n\n";
  out << "## Matched/implemented\n";
  out << "- 01_mean_hourly_by_season: pivoted line plot\n";
  out << "- 02_mean_hourly_by_weekday: pivoted line plot\n";
  out << "- 03_registered_vs_casual_by_weekday: dual-panel weekday lines\n";
  out << "- 04_daily_mean_time_series: grouped daily line plot\n";
  out << "- 06_box_total_by_weather: grouped box plot\n";
  out << "- 07_monthly_mean: grouped monthly line plot\n";
  out << "- 08_mean_by_month_ci: grouped monthly bar plot with CI\n";
  out << "- 09_holiday_workingday_by_season: dual-panel grouped bars\n";
  out << "- 10_boxplots_distributions: dual-panel box plots\n";
  out << "- 11_hourly_violin: grouped violin plot\n";
  out << "- 12_correlation_heatmap: corr heatmap\n";
  out << "- 13_pairplot_sampled: full scatter matrix (pairplot proxy)\n\n";
  out << "## Known gaps vs Python reference script\n";
  out << "- Grouped-bar CI for multiple hue columns is still limited when bars are pre-pivoted.\n";
  out << "- STL and SARIMAX sections were intentionally skipped as requested.\n";
  out << "- Facet-grid style relplot layout is approximated by pivoted multi-line charts.\n";

  std::cout << "Saved: " << report_path << "\n";
}

} // namespace

int main(int argc, char** argv) {
#if !__has_include("pluss/pl.h")
  std::cout << "pluss/pl.h is unavailable in this build.\n";
  return 0;
#else
  const auto cli = parse_cli(argc, argv);
  fs::create_directories(cli.out_dir);
  clear_plot_outputs(cli.out_dir);

  auto df = load_and_prepare(cli.data_path);

  std::cout << "\n=== Dataset shape ===\n" << df.ncases() << " rows x " << df.nfeatures() << " columns\n";
  std::cout << "\n=== Head (first 5 rows) ===\n";
  df.head(5, true);

  std::cout << "\n=== dtypes/info ===\n";
  df.info();

  std::cout << "\n=== describe (numeric) ===\n";
  df.describe(true);

  std::cout << "\n=== Missing values summary (per column) ===\n";
  for (const auto& feature : df.features()) {
    const auto null_count = df.features(feature).isnull().count().as_series().host<std::int64_t>().front();
    if (null_count > 0) {
      std::cout << feature << ": " << null_count << "\n";
    }
  }
  if (df.isnull().any(pluss::table::Axis::Case).sum().to_int() == 0) {
    std::cout << "No missing values detected.\n";
  }

  std::cout << "\n=== Value counts (season_name) ===\n";
  const auto season_counts = df.group({.dropnull = true})
                               .by({"season_name"})
                               .agg({
                                   {"count", "", GroupOp::Count},
                               })
                               .arrange({"count"}, false);
  season_counts.head(season_counts.ncases(), true);

  const auto hour_line_opts = build_plot_options();

  const auto season_hour = df.pivot_table("hour", "season_name", "total_count", GroupOp::Mean, true, false);
  auto fig_01 = season_hour.plot(hour_line_opts).line({"spring", "summer", "fall", "winter"});
  fig_01.headline("Mean hourly bike rentals by season");
  save_and_optionally_show(fig_01, cli.out_dir, "01_mean_hourly_by_season", cli.show);

  const auto weekday_hour = df.pivot_table("hour", "weekday_name", "total_count", GroupOp::Mean, true, false);
  auto fig_02 = weekday_hour.plot(hour_line_opts).line({"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"});
  fig_02.headline("Mean hourly bike rentals by weekday");
  save_and_optionally_show(fig_02, cli.out_dir, "02_mean_hourly_by_weekday", cli.show);

  const auto weekday_registered = df.pivot_table("hour", "weekday_name", "registered", GroupOp::Mean, true, false);
  const auto weekday_casual = df.pivot_table("hour", "weekday_name", "casual", GroupOp::Mean, true, false);
  auto fig_03 = make_registered_vs_casual_figure(weekday_registered, weekday_casual);
  save_and_optionally_show(fig_03, cli.out_dir, "03_registered_vs_casual_by_weekday", cli.show);

  auto daily = df.group({.dropnull = true})
                   .by({"date"})
                   .agg({{"mean_total_count", "total_count", GroupOp::Mean}})
                   .arrange({"date"}, true, true)
                   .labels().set("date", false);
  auto fig_04 = daily.plot(build_plot_options("date")).line("mean_total_count", "date");
  fig_04.headline("Daily mean bike rentals (from hourly data)");
  save_and_optionally_show(fig_04, cli.out_dir, "04_daily_mean_time_series", cli.show);

  auto fig_06 = pl::studio("06_box_total_by_weather");
  fig_06.theme(pl::themes::paper());
  fig_06.tight_layout(true, 4.0);
  auto& pane_06 = fig_06.pane2("Total rentals by weather condition");
  configure_pane(pane_06, false);
  auto& trace_06 = flash::boxplot(pane_06, df.features("weather_name").as_note(), df.features("total_count").as_series());
  trace_06.label("total_count");
  pane_06.x_label("Weather condition").y_label("Total rentals (hourly)");
  fig_06.headline("Total rentals by weather condition");
  save_and_optionally_show(fig_06, cli.out_dir, "06_box_total_by_weather", cli.show);

  auto monthly = df.group({.dropnull = true})
                     .by({"month_name"})
                     .agg({{"mean_total_count", "total_count", GroupOp::Mean}});
  auto fig_07 = monthly.plot(build_plot_options("month_name")).line("mean_total_count", "month_name");
  fig_07.headline("Monthly mean bike rentals (hourly averaged)");
  save_and_optionally_show(fig_07, cli.out_dir, "07_monthly_mean", cli.show);

  pluss::table::BarPlotOptions bar_opts;
  bar_opts.ci_enabled = true;
  bar_opts.ci_level = 0.95;
  auto fig_08 = df.plot(build_plot_options("month_name", std::nullopt)).bar("total_count", "month_name", bar_opts);
  fig_08.headline("Mean hourly rentals by month (95% CI)");
  save_and_optionally_show(fig_08, cli.out_dir, "08_mean_by_month_ci", cli.show);

  const auto holiday_by_season = df.pivot_table("is_holiday", "season_name", "total_count", GroupOp::Mean, true, false);
  const auto workday_by_season = df.pivot_table("is_workingday", "season_name", "total_count", GroupOp::Mean, true, false);
  auto fig_09 = make_holiday_workingday_figure(holiday_by_season, workday_by_season);
  save_and_optionally_show(fig_09, cli.out_dir, "09_holiday_workingday_by_season", cli.show);

  auto fig_10 = make_boxplot_distributions_figure(df);
  save_and_optionally_show(fig_10, cli.out_dir, "10_boxplots_distributions", cli.show);

  auto fig_11v = df.plot(build_plot_options("hour")).violin("total_count", "hour");
  fig_11v.headline("Hourly distribution of rentals (violin)");
  save_and_optionally_show(fig_11v, cli.out_dir, "11_hourly_violin", cli.show);

  auto fig_12 = df.plot(build_plot_options())
                    .corr_heatmap({"temp", "atemp", "humidity", "windspeed", "casual", "registered", "total_count"});
  fig_12.headline("Correlation heatmap (numeric features)");
  save_and_optionally_show(fig_12, cli.out_dir, "12_correlation_heatmap", cli.show);

  auto sample = df.features({"temp", "atemp", "humidity", "windspeed", "casual", "registered", "total_count"})
                    .sample(std::min<std::int64_t>(800, df.ncases()), 42);
  pluss::table::ScatterMatrixPlotOptions scatter_matrix_opts;
  scatter_matrix_opts.pair.triangle = pluss::table::ScatterMatrixTriangle::Full;
  auto fig_13 = sample.plot(build_plot_options()).scatter_matrix({}, scatter_matrix_opts);
  fig_13.headline("Pairplot (sampled)");
  save_and_optionally_show(fig_13, cli.out_dir, "13_pairplot_sampled", cli.show);

  write_parity_report(cli.out_dir);

  std::cout << "\nDone. Outputs saved to: " << fs::absolute(cli.out_dir) << "\n";
  return 0;
#endif
}

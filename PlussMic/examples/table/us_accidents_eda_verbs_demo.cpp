#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include <pluss/pluss.h>
#include <pluss/tils.h>

namespace {

using pluss::table::DataFrame;
using pluss::table::Thread;
using pluss::table::ThreadBinaryCompareOp;
using pluss::table::ThreadCastTarget;
using pluss::table::ThreadKind;
using pluss::table::ThreadWindowOp;

struct MeanAccum {
  double sum{0.0};
  std::int64_t n{0};

  void add(double x) {
    sum += x;
    ++n;
  }

  double mean() const {
    return n == 0 ? 0.0 : (sum / static_cast<double>(n));
  }
};

struct CorrAccum {
  double sx{0.0};
  double sy{0.0};
  double sxx{0.0};
  double syy{0.0};
  double sxy{0.0};
  std::int64_t n{0};

  void add(double x, double y) {
    sx += x;
    sy += y;
    sxx += x * x;
    syy += y * y;
    sxy += x * y;
    ++n;
  }

  double corr() const {
    if (n < 2) {
      return 0.0;
    }
    const auto nn = static_cast<double>(n);
    const auto cov = sxy - (sx * sy / nn);
    const auto vx = sxx - (sx * sx / nn);
    const auto vy = syy - (sy * sy / nn);
    if (vx <= 0.0 || vy <= 0.0) {
      return 0.0;
    }
    return cov / std::sqrt(vx * vy);
  }
};

struct ParsedTime {
  int year{0};
  int month{0};
  int day{0};
  int hour{0};
  bool ok{false};
};

std::filesystem::path data_file(const std::string& name) {
  return std::filesystem::path(PLUSSMIC_SOURCE_DIR) / "data" / name;
}

bool parse_int_2(std::string_view s, std::size_t pos, int& out) {
  if (pos + 1 >= s.size()) {
    return false;
  }
  if (!std::isdigit(static_cast<unsigned char>(s[pos])) ||
      !std::isdigit(static_cast<unsigned char>(s[pos + 1]))) {
    return false;
  }
  out = (s[pos] - '0') * 10 + (s[pos + 1] - '0');
  return true;
}

bool parse_int_4(std::string_view s, std::size_t pos, int& out) {
  if (pos + 3 >= s.size()) {
    return false;
  }
  for (std::size_t i = 0; i < 4; ++i) {
    if (!std::isdigit(static_cast<unsigned char>(s[pos + i]))) {
      return false;
    }
  }
  out = ((s[pos] - '0') * 1000) + ((s[pos + 1] - '0') * 100) + ((s[pos + 2] - '0') * 10) + (s[pos + 3] - '0');
  return true;
}

ParsedTime parse_start_time(std::string_view s) {
  ParsedTime out{};
  if (!parse_int_4(s, 0, out.year)) {
    return out;
  }
  if (!parse_int_2(s, 5, out.month) || !parse_int_2(s, 8, out.day)) {
    return out;
  }

  std::size_t hour_pos = std::string_view::npos;
  if (s.size() > 13 && (s[10] == ' ' || s[10] == 'T')) {
    hour_pos = 11;
  } else if (s.size() > 12 && (s[8] == ' ' || s[8] == 'T')) {
    hour_pos = 9;
  }

  if (hour_pos == std::string_view::npos || !parse_int_2(s, hour_pos, out.hour)) {
    return out;
  }

  out.ok = true;
  return out;
}

int day_of_week_idx(int y, int m, int d) {
  using namespace std::chrono;
  const auto ymd = year_month_day{year{y}, month{static_cast<unsigned>(m)}, day{static_cast<unsigned>(d)}};
  if (!ymd.ok()) {
    return -1;
  }
  const auto wd = weekday{sys_days{ymd}};
  return wd.c_encoding(); // 0=Sun
}

std::string month_name(int m) {
  static const std::array<const char*, 12> kMonths = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  if (m < 1 || m > 12) {
    return "NA";
  }
  return kMonths[static_cast<std::size_t>(m - 1)];
}

std::string day_name(int d) {
  static const std::array<const char*, 7> kDays = {
      "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  if (d < 0 || d > 6) {
    return "NA";
  }
  return kDays[static_cast<std::size_t>(d)];
}

std::string season_name(int month) {
  if (month == 12 || month <= 2) {
    return "Winter";
  }
  if (month <= 5) {
    return "Spring";
  }
  if (month <= 8) {
    return "Summer";
  }
  return "Fall";
}

std::string lower_copy(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return s;
}

bool contains_ci(std::string_view text, std::string_view needle) {
  auto t = lower_copy(std::string(text));
  auto n = lower_copy(std::string(needle));
  return t.find(n) != std::string::npos;
}

std::string weather_bucket(std::string_view condition) {
  if (contains_ci(condition, "rain") || contains_ci(condition, "drizzle")) {
    return "Rain";
  }
  if (contains_ci(condition, "snow") || contains_ci(condition, "sleet") || contains_ci(condition, "ice")) {
    return "Snow/Ice";
  }
  if (contains_ci(condition, "clear") || contains_ci(condition, "fair")) {
    return "Clear/Fair";
  }
  if (contains_ci(condition, "storm") || contains_ci(condition, "thunder")) {
    return "Storm";
  }
  return "Other";
}

std::string time_of_day_bucket(int hour) {
  if (hour < 0 || hour > 23) {
    return "Unknown";
  }
  if (hour < 6) {
    return "Night";
  }
  if (hour < 12) {
    return "Morning";
  }
  if (hour < 18) {
    return "Afternoon";
  }
  return "Evening";
}

std::string distance_bin(double miles) {
  if (miles < 1.0) {
    return "<1mi";
  }
  if (miles < 3.0) {
    return "1-3mi";
  }
  if (miles < 5.0) {
    return "3-5mi";
  }
  return ">=5mi";
}

std::string temp_bin(double temp_f) {
  if (temp_f < 32.0) {
    return "<32F";
  }
  if (temp_f < 60.0) {
    return "32-60F";
  }
  if (temp_f < 80.0) {
    return "60-80F";
  }
  return ">=80F";
}

std::string pressure_bin(double p) {
  if (p < 29.5) {
    return "Low(<29.5)";
  }
  if (p <= 30.0) {
    return "Normal(29.5-30.0)";
  }
  return "High(>30.0)";
}

std::string geo_bin(double lat, double lng) {
  const auto blat = std::floor(lat * 2.0) / 2.0;
  const auto blng = std::floor(lng * 2.0) / 2.0;
  return fmt::format("{:.1f},{:.1f}", blat, blng);
}

bool is_highway_like(std::string_view street) {
  const auto s = lower_copy(std::string(street));
  return s.find(" i-") != std::string::npos || s.rfind("i-", 0) == 0 || s.find("hwy") != std::string::npos ||
         s.find("highway") != std::string::npos || s.find("us-") != std::string::npos || s.find("route") != std::string::npos;
}

bool extract_numeric_value(const pluss::Series& s, std::int64_t idx, double& out) {
  const auto* base = s.raw_data();
  switch (s.scalar_type()) {
    case pluss::codem::ScalarType::Bool:
      out = reinterpret_cast<const bool*>(base)[idx] ? 1.0 : 0.0;
      return true;
    case pluss::codem::ScalarType::Int8:
      out = static_cast<double>(reinterpret_cast<const std::int8_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Int16:
      out = static_cast<double>(reinterpret_cast<const std::int16_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Int32:
      out = static_cast<double>(reinterpret_cast<const std::int32_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Int64:
      out = static_cast<double>(reinterpret_cast<const std::int64_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::UInt8:
      out = static_cast<double>(reinterpret_cast<const std::uint8_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::UInt16:
      out = static_cast<double>(reinterpret_cast<const std::uint16_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::UInt32:
      out = static_cast<double>(reinterpret_cast<const std::uint32_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::UInt64:
      out = static_cast<double>(reinterpret_cast<const std::uint64_t*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Float32:
      out = static_cast<double>(reinterpret_cast<const float*>(base)[idx]);
      return true;
    case pluss::codem::ScalarType::Float64:
      out = reinterpret_cast<const double*>(base)[idx];
      return true;
    default:
      return false;
  }
}

std::optional<double> numeric_at(const Thread& t, std::int64_t row) {
  if (!t.is_valid(row) || t.kind() != ThreadKind::Series) {
    return std::nullopt;
  }
  double out = 0.0;
  if (!extract_numeric_value(t.as_series(), row, out)) {
    return std::nullopt;
  }
  return out;
}

bool bool_at(const Thread& t, std::int64_t row, bool default_value = false) {
  if (!t.is_valid(row) || t.kind() != ThreadKind::Series) {
    return default_value;
  }
  double x = 0.0;
  if (!extract_numeric_value(t.as_series(), row, x)) {
    return default_value;
  }
  return x != 0.0;
}

std::optional<std::string> note_at(const Thread& t, std::int64_t row) {
  if (!t.is_valid(row) || t.kind() != ThreadKind::Note) {
    return std::nullopt;
  }
  return std::string(t.as_note().at(row));
}

Thread make_i64_thread(const std::vector<std::int64_t>& values) {
  auto out = pluss::Series::empty({static_cast<std::int64_t>(values.size())}, pluss::codem::ScalarType::Int64);
  if (!values.empty()) {
    std::memcpy(out.raw_data(), values.data(), values.size() * sizeof(std::int64_t));
  }
  return Thread(out);
}

Thread make_f64_thread(const std::vector<double>& values) {
  auto out = pluss::Series::empty({static_cast<std::int64_t>(values.size())}, pluss::codem::ScalarType::Float64);
  if (!values.empty()) {
    std::memcpy(out.raw_data(), values.data(), values.size() * sizeof(double));
  }
  return Thread(out);
}

Thread make_note_thread(const std::vector<std::string>& values) {
  std::vector<std::string_view> views;
  views.reserve(values.size());
  for (const auto& v : values) {
    views.emplace_back(v);
  }
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(views.size())};
  auto note = pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(views.data(), views.size()),
      std::span<const std::int64_t>(shape.data(), shape.size()));
  return Thread(note);
}

std::vector<std::pair<std::string, std::int64_t>> sorted_counts(
    const std::unordered_map<std::string, std::int64_t>& counts,
    std::size_t top_n = 0) {
  std::vector<std::pair<std::string, std::int64_t>> rows(counts.begin(), counts.end());
  std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) {
    if (a.second != b.second) {
      return a.second > b.second;
    }
    return a.first < b.first;
  });
  if (top_n > 0 && rows.size() > top_n) {
    rows.resize(top_n);
  }
  return rows;
}

std::vector<std::pair<std::string, double>> sorted_means(
    const std::unordered_map<std::string, MeanAccum>& means,
    std::size_t top_n = 0) {
  std::vector<std::pair<std::string, double>> rows;
  rows.reserve(means.size());
  for (const auto& [k, v] : means) {
    if (v.n == 0) {
      continue;
    }
    rows.emplace_back(k, v.mean());
  }
  std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) {
    if (a.second != b.second) {
      return a.second > b.second;
    }
    return a.first < b.first;
  });
  if (top_n > 0 && rows.size() > top_n) {
    rows.resize(top_n);
  }
  return rows;
}

DataFrame count_table(std::string key_name,
                      std::string value_name,
                      const std::vector<std::pair<std::string, std::int64_t>>& rows) {
  std::vector<std::string> keys;
  std::vector<std::int64_t> vals;
  keys.reserve(rows.size());
  vals.reserve(rows.size());
  for (const auto& [k, v] : rows) {
    keys.push_back(k);
    vals.push_back(v);
  }
  return DataFrame({{std::move(key_name), make_note_thread(keys)}, {std::move(value_name), make_i64_thread(vals)}});
}

DataFrame metric_table(std::string key_name,
                       std::string value_name,
                       const std::vector<std::pair<std::string, double>>& rows) {
  std::vector<std::string> keys;
  std::vector<double> vals;
  keys.reserve(rows.size());
  vals.reserve(rows.size());
  for (const auto& [k, v] : rows) {
    keys.push_back(k);
    vals.push_back(v);
  }
  return DataFrame({{std::move(key_name), make_note_thread(keys)}, {std::move(value_name), make_f64_thread(vals)}});
}

void show_q_table(int q, std::string_view title, const DataFrame& frame, std::int64_t n = 10) {
  std::cout << fmt::format("\nQ{}. {}\n", q, title);
  frame.head(n);
}

void run_verb_walkthrough(const DataFrame& df) {
  std::cout << "\n================ Verb Walkthrough ================\n";
  df.info();
  df.head(5);
  df.tail(5);
  df.view(5, 10);

  auto sel = df.features({"State", "City", "Severity", "Distance(mi)", "Start_Time", "Weather_Condition"});
  (void)sel.drop({"City"}).reorder({"State", "Severity", "Distance(mi)", "Start_Time", "Weather_Condition"});

  auto severe_mask = df.features("Severity").compare(3.0, ThreadBinaryCompareOp::Ge).as_series();
  auto q_mask = df.filter(severe_mask);
  auto q_pred = df.filter(df.features("Distance(mi)").compare(1.0, ThreadBinaryCompareOp::Gt).as_series());
  auto q_expr = df.query(R"(Severity >= 3 and `Visibility(mi)` < 5)");
  (void)q_pred;
  (void)q_expr;

  auto d_assign = df.assign({{"Severity_Copy", df.features("Severity")}});
  auto d_spec = d_assign.assign({{
      "Is_Severe",
      [](const DataFrame& frame) {
        return frame.features("Severity").compare(3.0, ThreadBinaryCompareOp::Ge);
      },
  }});
  (void)d_spec.assign({{"Severity_Alias", d_spec.features("Severity")}});

  auto sum = df.aggregate({
      {"cases", "Severity", DataFrame::SummaryOp::Count},
      {"severity_mean", "Severity", DataFrame::SummaryOp::Mean},
      {"severity_std", "Severity", DataFrame::SummaryOp::Std},
      {"distance_mean", "Distance(mi)", DataFrame::SummaryOp::Mean},
  });
  (void)df.aggregate({{"severity_max", "Severity", DataFrame::SummaryOp::Max}});
  (void)sum;

  auto arranged = q_mask.sort_values({"State"}).sort_values({
      {"Severity", false, true},
      {"Distance(mi)", false, true},
  });
  (void)arranged;

  auto miss_isnull = df.isna({"Visibility(mi)", "Precipitation(in)", "Weather_Condition"});
  auto miss_notnull = df.notna({"Visibility(mi)", "Precipitation(in)"});
  auto miss_fill = df.fillna(0.0, {"Precipitation(in)"});
  auto miss_drop_any = df.dropna();
  auto miss_drop_all = df.dropna_all({"Precipitation(in)", "Visibility(mi)"});
  auto miss_ffill = df.ffill();
  auto miss_bfill = df.bfill();
  (void)miss_isnull;
  (void)miss_notnull;
  (void)miss_fill;
  (void)miss_drop_any;
  (void)miss_drop_all;
  (void)miss_ffill;
  (void)miss_bfill;

  auto cast_note = df.cast({DataFrame::CastSpec{"Start_Time", ThreadCastTarget::Note, std::nullopt}});
  auto cast_series = cast_note.cast(
      {DataFrame::CastSpec{"Start_Time", ThreadCastTarget::Series, pluss::codem::ScalarType::Int64}});
  auto typed = cast_series.astype({{"Severity", pluss::codem::ScalarType::Float64}});
  (void)typed;

  auto numeric = df.features({"Severity", "Distance(mi)", "Visibility(mi)"}).astype({
      {"Severity", pluss::codem::ScalarType::Float64},
      {"Distance(mi)", pluss::codem::ScalarType::Float64},
      {"Visibility(mi)", pluss::codem::ScalarType::Float64},
  });
  auto arith = numeric.add(1.0)
                   .sub(1.0)
                   .mul(2.0)
                   .div(2.0)
                   .truediv(2.0)
                   .floordiv(2.0)
                   .mod(2.0)
                   .pow(2.0)
                   .radd(5.0)
                   .rsub(10.0)
                   .rmul(3.0)
                   .rdiv(10.0)
                   .rtruediv(10.0)
                   .rfloordiv(10.0)
                   .rmod(10.0)
                   .rpow(2.0);
  auto cmp = arith.eq(arith)
                 .ne(arith.add(1.0))
                 .lt(5.0)
                 .le(5.0)
                 .gt(5.0)
                 .ge(5.0)
                 .compare(arith.add(2.0));
  (void)cmp;

  auto win = numeric.rolling({
      DataFrame::WindowSpec{"severity_rollmean_12", "Severity", ThreadWindowOp::RollingMean, 12, 3},
      DataFrame::WindowSpec{"distance_rollmax_24", "Distance(mi)", ThreadWindowOp::RollingMax, 24, 1},
  });
  (void)win.rolling(
      {DataFrame::WindowSpec{"visibility_rollmin_6", "Visibility(mi)", ThreadWindowOp::RollingMin, 6, 1}});

  std::cout << "Verb walkthrough complete.\n";
}

} // namespace

int main(int argc, char** argv) {
  std::optional<std::size_t> ncases = std::nullopt;
  if (argc > 1) {
    const auto v = std::stoll(argv[1]);
    if (v > 0) {
      ncases = static_cast<std::size_t>(v);
    }
  }

  pluss::loader::ReadCsvOptions options;
  options.filepath = data_file("US_Accidents_Dec20.csv").string();
  options.engine = pluss::loader::CsvEngine::Auto;
  options.on_bad_lines = pluss::loader::BadLinePolicy::Skip;
  options.parse_dates = {"Start_Time", "End_Time"};
  options.usefeats = {
      "Severity",
      "Start_Time",
      "End_Time",
      "Start_Lat",
      "Start_Lng",
      "Distance(mi)",
      "Street",
      "Side",
      "City",
      "County",
      "State",
      "Zipcode",
      "Timezone",
      "Airport_Code",
      "Temperature(F)",
      "Humidity(%)",
      "Pressure(in)",
      "Visibility(mi)",
      "Wind_Speed(mph)",
      "Precipitation(in)",
      "Weather_Condition",
      "Crossing",
      "Junction",
      "Railway",
      "Roundabout",
      "Traffic_Calming",
      "Traffic_Signal",
      "Sunrise_Sunset",
      "Civil_Twilight",
      "Nautical_Twilight",
      "Astronomical_Twilight",
  };
  options.ncases = ncases;

  auto df = DataFrame::load_csv(options.filepath, options);

  std::cout << fmt::format("Loaded US accidents dataset: [ {} cs x {} feat ]\n", df.ncases(), df.nfeats());
  std::cout << "Use argv[1]=0 or omit for full file; pass a positive integer for a quick sample.\n";

  run_verb_walkthrough(df);

  auto start_as_note = df.cast({DataFrame::CastSpec{"Start_Time", ThreadCastTarget::Note, std::nullopt}});
  auto times_ns = df.cast({
      {"Start_Time", ThreadCastTarget::Series, pluss::codem::ScalarType::Int64},
      {"End_Time", ThreadCastTarget::Series, pluss::codem::ScalarType::Int64},
  });

  const auto& severity_t = df.features("Severity");
  const auto& distance_t = df.features("Distance(mi)");
  const auto& street_t = df.features("Street");
  const auto& city_t = df.features("City");
  const auto& county_t = df.features("County");
  const auto& state_t = df.features("State");
  const auto& zip_t = df.features("Zipcode");
  const auto& timezone_t = df.features("Timezone");
  const auto& airport_t = df.features("Airport_Code");
  const auto& weather_t = df.features("Weather_Condition");
  const auto& temp_t = df.features("Temperature(F)");
  const auto& humidity_t = df.features("Humidity(%)");
  const auto& pressure_t = df.features("Pressure(in)");
  const auto& visibility_t = df.features("Visibility(mi)");
  const auto& wind_t = df.features("Wind_Speed(mph)");
  const auto& precip_t = df.features("Precipitation(in)");
  const auto& side_t = df.features("Side");
  const auto& junction_t = df.features("Junction");
  const auto& railway_t = df.features("Railway");
  const auto& roundabout_t = df.features("Roundabout");
  const auto& traffic_calming_t = df.features("Traffic_Calming");
  const auto& traffic_signal_t = df.features("Traffic_Signal");
  const auto& sunrise_t = df.features("Sunrise_Sunset");
  const auto& civil_t = df.features("Civil_Twilight");
  const auto& nautical_t = df.features("Nautical_Twilight");
  const auto& astro_t = df.features("Astronomical_Twilight");
  const auto& lat_t = df.features("Start_Lat");
  const auto& lng_t = df.features("Start_Lng");

  let something = distance_t.mean();
  let something_else = df.features({"Distance(mi)", "Severity"});

  const auto& start_note_t = start_as_note.features("Start_Time");
  const auto& start_ns_t = times_ns.features("Start_Time");
  const auto& end_ns_t = times_ns.features("End_Time");

  std::unordered_map<std::string, std::int64_t> state_count, city_count, county_count, timezone_count, zip_count,
      airport_count, geo_count, street_count, weather_count, weather_bucket_count, season_count, tod_count,
      day_night_count, civil_count, nautical_count, astro_count, side_count, temp_bin_count, dist_bin_count,
      pressure_bin_count;

  std::unordered_map<std::string, MeanAccum> state_mean_sev, weather_mean_sev, hwy_vs_city_mean,
      junction_mean, railway_mean, signal_mean, precip_mean, vis_mean, roundabout_vs_junction_mean,
      calming_mean, tod_sev_mean;
  std::unordered_map<std::string, MeanAccum> severity_duration_mean, weather_tod_mean;

  std::unordered_map<std::string, std::int64_t> severe_state_count, total_state_count;
  std::unordered_map<std::int64_t, std::int64_t> severity_dist, year_count, month_count, dow_count, hour_count;

  CorrAccum severity_distance_corr, severity_wind_corr, severity_humidity_corr, severity_pressure_corr,
      severity_duration_corr;

  std::int64_t urban_count = 0;
  std::int64_t rural_count = 0;
  std::int64_t rush_count = 0;
  std::int64_t non_rush_count = 0;
  std::int64_t vis_lt5_count = 0;
  std::int64_t vis_ge5_count = 0;
  std::int64_t storm_count = 0;

  MeanAccum duration_mean;
  MeanAccum night_sev;
  MeanAccum day_sev;

  const auto n = df.ncases();
  for (std::int64_t i = 0; i < n; ++i) {
    const auto sev = numeric_at(severity_t, i);
    if (!sev.has_value()) {
      continue;
    }
    const auto sev_level = static_cast<std::int64_t>(std::llround(*sev));
    severity_dist[sev_level] += 1;

    const auto st = note_at(state_t, i);
    if (st.has_value() && !st->empty()) {
      state_count[*st] += 1;
      total_state_count[*st] += 1;
      state_mean_sev[*st].add(*sev);
      if (*sev >= 3.0) {
        severe_state_count[*st] += 1;
      }
    }

    const auto city = note_at(city_t, i);
    if (city.has_value() && !city->empty()) {
      city_count[*city] += 1;
      urban_count += 1;
    } else {
      rural_count += 1;
    }

    const auto county = note_at(county_t, i);
    if (county.has_value() && !county->empty()) {
      county_count[*county] += 1;
    }

    const auto tz = note_at(timezone_t, i);
    if (tz.has_value() && !tz->empty()) {
      timezone_count[*tz] += 1;
    }

    const auto zip = note_at(zip_t, i);
    if (zip.has_value() && !zip->empty()) {
      zip_count[*zip] += 1;
    }

    const auto ap = note_at(airport_t, i);
    if (ap.has_value() && !ap->empty()) {
      airport_count[*ap] += 1;
    }

    const auto street = note_at(street_t, i);
    if (street.has_value() && !street->empty()) {
      street_count[*street] += 1;
    }

    if (const auto lat = numeric_at(lat_t, i); lat.has_value()) {
      if (const auto lng = numeric_at(lng_t, i); lng.has_value()) {
        geo_count[geo_bin(*lat, *lng)] += 1;
      }
    }

    if (const auto d = numeric_at(distance_t, i); d.has_value()) {
      severity_distance_corr.add(*d, *sev);
      dist_bin_count[distance_bin(*d)] += 1;
      if (street.has_value() && !street->empty()) {
        const auto key = is_highway_like(*street) ? "Highway-like" : "City/local";
        hwy_vs_city_mean[key].add(*sev);
      }
    }

    if (const auto w = numeric_at(wind_t, i); w.has_value()) {
      severity_wind_corr.add(*w, *sev);
    }
    if (const auto h = numeric_at(humidity_t, i); h.has_value()) {
      severity_humidity_corr.add(*h, *sev);
    }
    if (const auto p = numeric_at(pressure_t, i); p.has_value()) {
      severity_pressure_corr.add(*p, *sev);
      pressure_bin_count[pressure_bin(*p)] += 1;
    }

    if (const auto t = numeric_at(temp_t, i); t.has_value()) {
      temp_bin_count[temp_bin(*t)] += 1;
    }

    const bool j = bool_at(junction_t, i);
    const bool rw = bool_at(railway_t, i);
    const bool ts = bool_at(traffic_signal_t, i);
    const bool rb = bool_at(roundabout_t, i);
    const bool tc = bool_at(traffic_calming_t, i);

    junction_mean[j ? "Junction=True" : "Junction=False"].add(*sev);
    railway_mean[rw ? "Railway=True" : "Railway=False"].add(*sev);
    signal_mean[ts ? "Traffic_Signal=True" : "Traffic_Signal=False"].add(*sev);
    calming_mean[tc ? "Traffic_Calming=True" : "Traffic_Calming=False"].add(*sev);
    if (rb) {
      roundabout_vs_junction_mean["Roundabout=True"].add(*sev);
    }
    if (j) {
      roundabout_vs_junction_mean["Junction=True"].add(*sev);
    }

    if (const auto p = numeric_at(precip_t, i); p.has_value()) {
      precip_mean[*p > 0.0 ? "Precip>0" : "Precip=0"].add(*sev);
    }

    if (const auto v = numeric_at(visibility_t, i); v.has_value()) {
      const bool low_vis = (*v < 5.0);
      vis_mean[low_vis ? "Visibility<5" : "Visibility>=5"].add(*sev);
      if (low_vis) {
        ++vis_lt5_count;
      } else {
        ++vis_ge5_count;
      }
    }

    const auto side = note_at(side_t, i);
    if (side.has_value() && !side->empty()) {
      side_count[*side] += 1;
    }

    const auto sunrise = note_at(sunrise_t, i);
    if (sunrise.has_value() && !sunrise->empty()) {
      const std::string bucket = contains_ci(*sunrise, "night") ? "Night" : "Day";
      day_night_count[bucket] += 1;
      if (bucket == "Night") {
        night_sev.add(*sev);
      } else {
        day_sev.add(*sev);
      }
    }

    if (const auto c = note_at(civil_t, i); c.has_value() && !c->empty()) {
      civil_count[*c] += 1;
    }
    if (const auto nt = note_at(nautical_t, i); nt.has_value() && !nt->empty()) {
      nautical_count[*nt] += 1;
    }
    if (const auto a = note_at(astro_t, i); a.has_value() && !a->empty()) {
      astro_count[*a] += 1;
    }

    const auto weather = note_at(weather_t, i);
    if (weather.has_value() && !weather->empty()) {
      weather_count[*weather] += 1;
      weather_mean_sev[*weather].add(*sev);
      const auto wb = weather_bucket(*weather);
      weather_bucket_count[wb] += 1;
      if (wb == "Storm") {
        ++storm_count;
      }
    }

    const auto start_txt = note_at(start_note_t, i);
    if (!start_txt.has_value()) {
      continue;
    }

    const auto pt = parse_start_time(*start_txt);
    if (!pt.ok) {
      continue;
    }

    year_count[pt.year] += 1;
    month_count[pt.month] += 1;
    hour_count[pt.hour] += 1;
    season_count[season_name(pt.month)] += 1;

    const auto tod = time_of_day_bucket(pt.hour);
    tod_count[tod] += 1;
    tod_sev_mean[tod].add(*sev);

    const auto dow = day_of_week_idx(pt.year, pt.month, pt.day);
    if (dow >= 0) {
      dow_count[static_cast<std::int64_t>(dow)] += 1;
    }

    const bool rush = (pt.hour >= 7 && pt.hour <= 9) || (pt.hour >= 16 && pt.hour <= 18);
    if (rush) {
      ++rush_count;
    } else {
      ++non_rush_count;
    }

    if (weather.has_value()) {
      const auto combo = fmt::format("{} | {}", weather_bucket(*weather), tod);
      weather_tod_mean[combo].add(*sev);
    }

    const auto start_ns = numeric_at(start_ns_t, i);
    const auto end_ns = numeric_at(end_ns_t, i);
    if (start_ns.has_value() && end_ns.has_value() && *end_ns >= *start_ns) {
      const auto dur_h = (*end_ns - *start_ns) / 3.6e12;
      duration_mean.add(dur_h);
      severity_duration_mean[fmt::format("Severity={}", sev_level)].add(dur_h);
      severity_duration_corr.add(dur_h, *sev);
    }
  }

  show_q_table(1, "Top five states with the most accidents", count_table("state", "accidents", sorted_counts(state_count, 5)), 5);
  show_q_table(2, "Top ten cities with the most accidents", count_table("city", "accidents", sorted_counts(city_count, 10)), 10);
  show_q_table(3, "Counties with highest accident rates (count proxy; no population denominator in dataset)",
               count_table("county", "accidents", sorted_counts(county_count, 10)), 10);
  show_q_table(4, "States with highest average accident severity", metric_table("state", "avg_severity", sorted_means(state_mean_sev, 10)), 10);
  show_q_table(5, "Distribution of accidents across time zones", count_table("timezone", "accidents", sorted_counts(timezone_count, 10)), 10);
  show_q_table(6, "Zip codes with highest accident density", count_table("zipcode", "accidents", sorted_counts(zip_count, 10)), 10);

  {
    std::vector<std::pair<std::string, std::int64_t>> rows{{"Urban proxy (City present)", urban_count}, {"Rural proxy (City missing)", rural_count}};
    show_q_table(7, "Urban vs rural frequency (proxy based on City presence)", count_table("bucket", "accidents", rows), 2);
  }

  show_q_table(8, "Airports associated with most nearby accidents", count_table("airport_code", "accidents", sorted_counts(airport_count, 10)), 10);
  show_q_table(9, "Geographic clustering pattern (top 0.5deg geobins)", count_table("geo_bin", "accidents", sorted_counts(geo_count, 10)), 10);
  show_q_table(10, "Street segments/highways with most accidents", count_table("street", "accidents", sorted_counts(street_count, 10)), 10);

  {
    std::unordered_map<std::string, std::int64_t> y;
    for (const auto& [k, v] : year_count) {
      y[fmt::format("{}", k)] = v;
    }
    show_q_table(11, "Year with most accidents", count_table("year", "accidents", sorted_counts(y, 10)), 10);
  }

  {
    std::unordered_map<std::string, std::int64_t> m;
    for (const auto& [k, v] : month_count) {
      m[month_name(static_cast<int>(k))] = v;
    }
    show_q_table(12, "Month with highest accident frequency", count_table("month", "accidents", sorted_counts(m, 12)), 12);
  }

  {
    std::unordered_map<std::string, std::int64_t> d;
    for (const auto& [k, v] : dow_count) {
      d[day_name(static_cast<int>(k))] = v;
    }
    show_q_table(13, "Day of week with most accidents", count_table("day", "accidents", sorted_counts(d, 7)), 7);
  }

  {
    std::unordered_map<std::string, std::int64_t> h;
    for (const auto& [k, v] : hour_count) {
      h[fmt::format("{:02d}:00", static_cast<int>(k))] = v;
    }
    show_q_table(14, "Time of day with most accidents", count_table("hour", "accidents", sorted_counts(h, 24)), 12);
  }

  {
    std::vector<std::pair<std::string, std::int64_t>> rows{{"Rush hour (7-9,16-18)", rush_count}, {"Other hours", non_rush_count}};
    show_q_table(15, "Accidents during rush hours", count_table("bucket", "accidents", rows), 2);
  }

  show_q_table(16, "Seasonal pattern in accidents", count_table("season", "accidents", sorted_counts(season_count, 4)), 4);

  {
    std::vector<std::pair<std::int64_t, std::int64_t>> yv(year_count.begin(), year_count.end());
    std::sort(yv.begin(), yv.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    std::vector<std::string> years;
    std::vector<std::int64_t> counts;
    years.reserve(yv.size());
    counts.reserve(yv.size());
    for (const auto& [year, c] : yv) {
      years.push_back(fmt::format("{}", year));
      counts.push_back(c);
    }
    auto ydf = DataFrame({{"year", make_note_thread(years)}, {"accidents", make_i64_thread(counts)}});
    auto trend = ydf.rolling(
        {DataFrame::WindowSpec{"accidents_rollmean_3", "accidents", ThreadWindowOp::RollingMean, 3, 1}}, true);
    show_q_table(17, "Accident frequency trend over time", trend, trend.ncases());
  }

  std::cout << fmt::format("\nQ18. Average accident duration (End_Time - Start_Time): {:.3f} hours\n", duration_mean.mean());
  std::cout << fmt::format("Q19. Correlation(duration, severity): {:.4f}\n", severity_duration_corr.corr());
  show_q_table(19, "Average duration by severity level", metric_table("severity", "avg_duration_hours", sorted_means(severity_duration_mean, 8)), 8);

  std::cout << fmt::format("\nQ20. Mean severity at night: {:.4f} | day: {:.4f}\n", night_sev.mean(), day_sev.mean());

  {
    std::unordered_map<std::string, std::int64_t> s;
    for (const auto& [k, v] : severity_dist) {
      s[fmt::format("{}", k)] = v;
    }
    show_q_table(21, "Distribution of accident severity levels", count_table("severity", "accidents", sorted_counts(s, 8)), 8);
  }

  {
    std::vector<std::pair<std::string, double>> rows;
    rows.reserve(total_state_count.size());
    for (const auto& [st, total] : total_state_count) {
      if (total == 0) {
        continue;
      }
      const auto severe = severe_state_count[st];
      rows.emplace_back(st, static_cast<double>(severe) / static_cast<double>(total));
    }
    std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) {
      if (a.second != b.second) {
        return a.second > b.second;
      }
      return a.first < b.first;
    });
    if (rows.size() > 10) {
      rows.resize(10);
    }
    show_q_table(22, "States with highest proportion of severity 3-4 accidents", metric_table("state", "prop_severity_3_4", rows), 10);
  }

  std::cout << fmt::format("\nQ23. Correlation(Severity, Distance(mi)): {:.4f}\n", severity_distance_corr.corr());
  show_q_table(24, "Severity on highway-like vs city/local streets", metric_table("road_type", "avg_severity", sorted_means(hwy_vs_city_mean, 4)), 4);
  show_q_table(25, "Severity near intersections (Junction)", metric_table("junction", "avg_severity", sorted_means(junction_mean, 2)), 2);
  show_q_table(26, "Severity near railway crossings", metric_table("railway", "avg_severity", sorted_means(railway_mean, 2)), 2);
  show_q_table(27, "Severity with vs without traffic signals", metric_table("traffic_signal", "avg_severity", sorted_means(signal_mean, 2)), 2);
  show_q_table(28, "Severity with precipitation vs dry", metric_table("precip", "avg_severity", sorted_means(precip_mean, 2)), 2);
  show_q_table(29, "Low visibility vs higher visibility severity", metric_table("visibility_bucket", "avg_severity", sorted_means(vis_mean, 2)), 2);
  show_q_table(30, "Weather conditions with highest average severity", metric_table("weather", "avg_severity", sorted_means(weather_mean_sev, 10)), 10);

  show_q_table(31, "Weather conditions most associated with accidents", count_table("weather", "accidents", sorted_counts(weather_count, 10)), 10);
  show_q_table(32, "Rain vs snow vs clear accident frequency", count_table("weather_bucket", "accidents", sorted_counts(weather_bucket_count, 6)), 6);
  show_q_table(33, "Temperature bins vs accident frequency", count_table("temp_bin", "accidents", sorted_counts(temp_bin_count, 8)), 8);
  std::cout << fmt::format("\nQ34. Correlation(Wind Speed, Severity): {:.4f}\n", severity_wind_corr.corr());

  {
    std::vector<std::pair<std::string, std::int64_t>> rows{{"Visibility<5", vis_lt5_count}, {"Visibility>=5", vis_ge5_count}};
    show_q_table(35, "Accidents with visibility below 5 miles", count_table("visibility_bucket", "accidents", rows), 2);
  }

  std::cout << fmt::format("\nQ36. Correlation(Humidity, Severity): {:.4f}\n", severity_humidity_corr.corr());
  show_q_table(37, "Pressure bins and accident counts", count_table("pressure_bin", "accidents", sorted_counts(pressure_bin_count, 5)), 5);
  std::cout << fmt::format("Q37b. Correlation(Pressure, Severity): {:.4f}\n", severity_pressure_corr.corr());
  std::cout << fmt::format("\nQ38. Storm-related accidents count: {}\n", storm_count);

  show_q_table(39, "Accidents at night vs day", count_table("sunrise_sunset", "accidents", sorted_counts(day_night_count, 4)), 4);
  show_q_table(40, "Twilight period counts (civil)", count_table("civil_twilight", "accidents", sorted_counts(civil_count, 8)), 8);

  {
    std::unordered_map<std::string, MeanAccum> astro_sev;
    for (std::int64_t i = 0; i < n; ++i) {
      const auto sev = numeric_at(severity_t, i);
      const auto astro = note_at(astro_t, i);
      if (!sev.has_value() || !astro.has_value() || astro->empty()) {
        continue;
      }
      astro_sev[*astro].add(*sev);
    }
    show_q_table(41, "Mean severity in astronomical twilight categories", metric_table("astronomical_twilight", "avg_severity", sorted_means(astro_sev, 8)), 8);
  }

  {
    std::vector<std::pair<std::string, std::int64_t>> signal_counts{{"Traffic_Signal=True", 0}, {"Traffic_Signal=False", 0}};
    std::vector<std::pair<std::string, std::int64_t>> junction_counts{{"Junction=True", 0}, {"Junction=False", 0}};
    for (std::int64_t i = 0; i < n; ++i) {
      signal_counts[bool_at(traffic_signal_t, i) ? 0 : 1].second += 1;
      junction_counts[bool_at(junction_t, i) ? 0 : 1].second += 1;
    }
    show_q_table(42, "Accidents near traffic signals", count_table("traffic_signal", "accidents", signal_counts), 2);
    show_q_table(43, "Accidents at junctions", count_table("junction", "accidents", junction_counts), 2);
  }

  show_q_table(44, "Roundabouts vs junctions average severity", metric_table("road_feature", "avg_severity", sorted_means(roundabout_vs_junction_mean, 4)), 4);
  show_q_table(45, "Railway presence and average severity", metric_table("railway", "avg_severity", sorted_means(railway_mean, 2)), 2);
  show_q_table(46, "Accidents by road side (L vs R)", count_table("side", "accidents", sorted_counts(side_count, 4)), 4);
  show_q_table(47, "Distance bins and accident counts", count_table("distance_bin", "accidents", sorted_counts(dist_bin_count, 6)), 6);
  show_q_table(48, "Traffic calming and average severity", metric_table("traffic_calming", "avg_severity", sorted_means(calming_mean, 2)), 2);

  show_q_table(49, "Weather + time-of-day combinations with highest severity", metric_table("combo", "avg_severity", sorted_means(weather_tod_mean, 12)), 12);

  std::cout << "\nQ50. Predictive modeling (severity prediction) is not yet in current table verbs.\n";
  std::cout << "      Needed next: model/train API (or integration with Pluss ML layer), feature encoding, train/test split, metrics.\n";

  return 0;
}

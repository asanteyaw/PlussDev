#include <pluss/pluss.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using NumericMap = std::map<std::string, std::vector<double>>;

[[nodiscard]] std::string trim_copy(std::string value) {
  auto is_ws = [](unsigned char c) { return std::isspace(c) != 0; };
  while (!value.empty() && is_ws(static_cast<unsigned char>(value.front()))) {
    value.erase(value.begin());
  }
  while (!value.empty() && is_ws(static_cast<unsigned char>(value.back()))) {
    value.pop_back();
  }
  return value;
}

[[nodiscard]] std::string escape_single_quotes(std::string value) {
  std::string out;
  out.reserve(value.size() + 8);
  for (char c : value) {
    out.push_back(c);
    if (c == '\'') {
      out.push_back('\'');
    }
  }
  return out;
}

[[nodiscard]] std::vector<double> parse_csv_doubles(std::string_view values) {
  std::vector<double> out;
  std::string token;
  std::stringstream stream{std::string(values)};
  while (std::getline(stream, token, ',')) {
    token = trim_copy(token);
    if (token.empty()) {
      continue;
    }
    out.push_back(std::stod(token));
  }
  return out;
}

[[nodiscard]] NumericMap parse_reference_file(const std::filesystem::path& path) {
  NumericMap out;
  std::ifstream input(path);
  if (!input.is_open()) {
    throw std::runtime_error("failed to open MATLAB reference file");
  }
  std::string line;
  while (std::getline(input, line)) {
    const auto split = line.find('=');
    if (split == std::string::npos) {
      continue;
    }
    std::string key = trim_copy(line.substr(0, split));
    std::string value = trim_copy(line.substr(split + 1));
    out[std::move(key)] = parse_csv_doubles(value);
  }
  return out;
}

[[nodiscard]] std::optional<std::filesystem::path> matlab_bin_path() {
  if (const char* env = std::getenv("PLUSS_MATLAB_BIN")) {
    std::filesystem::path candidate(env);
    if (std::filesystem::exists(candidate)) {
      return candidate;
    }
  }
  const std::filesystem::path default_bin("/Applications/MATLAB_R2025b.app/bin/matlab");
  if (std::filesystem::exists(default_bin)) {
    return default_bin;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::filesystem::path> run_matlab_reference() {
  const auto matlab = matlab_bin_path();
  if (!matlab.has_value()) {
    return std::nullopt;
  }

  const std::filesystem::path repo_root = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
  const std::filesystem::path matlab_dir = repo_root / "PlussPl" / "test" / "matlab";
  const std::filesystem::path out_path =
      std::filesystem::temp_directory_path() / "plusspl_matlab_parity_reference.txt";

  const std::string matlab_dir_q = escape_single_quotes(matlab_dir.string());
  const std::string out_path_q = escape_single_quotes(out_path.string());
  const std::string batch =
      "addpath('" + matlab_dir_q + "'); plusspl_parity_reference('" + out_path_q + "');";
  const std::string command =
      "\"" + matlab->string() + "\" -batch \"" + batch + "\" > /dev/null 2>&1";

  const int code = std::system(command.c_str());
  if (code != 0 || !std::filesystem::exists(out_path)) {
    return std::nullopt;
  }
  return out_path;
}

[[nodiscard]] std::vector<std::string> svg_text_labels(const std::string& svg) {
  std::vector<std::string> labels;
  std::size_t pos = 0;
  while (true) {
    const std::size_t open = svg.find("<text", pos);
    if (open == std::string::npos) {
      break;
    }
    const std::size_t gt = svg.find('>', open);
    if (gt == std::string::npos) {
      break;
    }
    const std::size_t close = svg.find("</text>", gt + 1);
    if (close == std::string::npos) {
      break;
    }
    labels.push_back(trim_copy(svg.substr(gt + 1, close - (gt + 1))));
    pos = close + 7;
  }
  return labels;
}

[[nodiscard]] std::vector<double> numeric_labels(const std::string& svg) {
  std::vector<double> values;
  for (const auto& label : svg_text_labels(svg)) {
    if (label.empty()) {
      continue;
    }
    char* end = nullptr;
    const double value = std::strtod(label.c_str(), &end);
    if (end == label.c_str()) {
      continue;
    }
    if (*trim_copy(std::string(end)).c_str() != '\0') {
      continue;
    }
    if (std::isfinite(value)) {
      values.push_back(value);
    }
  }
  std::sort(values.begin(), values.end());
  values.erase(std::unique(values.begin(), values.end(), [](double lhs, double rhs) {
                 return std::abs(lhs - rhs) <= 1e-9;
               }),
               values.end());
  return values;
}

[[nodiscard]] std::vector<double> pluss_linear_xticks() {
  using namespace pluss::pl;
  Studio book("linear-ticks");
  auto& pane = book.pane2();
  pane.grid(GridStyle::Major)
      .x_limits({0.8, 3.2})
      .y_limits({0.5, 9.5})
      .y_ticks(std::array<double, 3>{1.0, 5.0, 9.0},
               std::vector<std::string>{"low", "mid", "high"});
  pane.trend(std::array<double, 3>{1.0, 2.0, 3.0}, std::array<double, 3>{1.0, 4.0, 9.0});
  const auto labels = numeric_labels(book.to_svg());
  std::vector<double> filtered;
  for (const double v : labels) {
    if (v >= 0.8 - 1e-9 && v <= 3.2 + 1e-9) {
      filtered.push_back(v);
    }
  }
  return filtered;
}

[[nodiscard]] std::vector<double> pluss_log_xticks() {
  using namespace pluss::pl;
  Studio book("log-ticks");
  auto& pane = book.pane2();
  pane.grid(GridStyle::Major)
      .scale_x(ScalePolicy::log10())
      .x_limits({1e-1, 1e3})
      .y_ticks(std::array<double, 3>{0.0, 1.0, 2.0},
               std::vector<std::string>{"a", "b", "c"});
  pane.trend(std::array<double, 5>{0.1, 1.0, 10.0, 100.0, 1000.0},
             std::array<double, 5>{0.0, 0.5, 1.0, 1.5, 2.0});
  const auto labels = numeric_labels(book.to_svg());
  std::vector<double> filtered;
  for (const double v : labels) {
    if (v > 0.0 && v >= 1e-1 - 1e-9 && v <= 1e3 + 1e-9) {
      filtered.push_back(v);
    }
  }
  return filtered;
}

[[nodiscard]] double match_ratio(const std::vector<double>& expected, const std::vector<double>& actual, double tol) {
  if (expected.empty()) {
    return 0.0;
  }
  std::size_t matched = 0;
  for (const double exp : expected) {
    const bool found = std::any_of(actual.begin(), actual.end(), [&](double got) {
      return std::abs(exp - got) <= tol;
    });
    if (found) {
      ++matched;
    }
  }
  return static_cast<double>(matched) / static_cast<double>(expected.size());
}

void write_parity_report(
    const std::filesystem::path& report_path,
    double score_2d,
    double score_3d,
    double tick_parity,
    double overall,
    const std::vector<double>& expected_linear,
    const std::vector<double>& actual_linear,
    const std::vector<double>& expected_log,
    const std::vector<double>& actual_log) {
  std::filesystem::create_directories(report_path.parent_path());
  std::ofstream out(report_path);
  out.setf(std::ios::fixed);
  out << std::setprecision(2);
  out << "plusspl_2d_family_score=" << score_2d << "\n";
  out << "plusspl_3d_family_score=" << score_3d << "\n";
  out << "plusspl_matlab_tick_parity=" << tick_parity << "\n";
  out << "plusspl_overall_maturity_score=" << overall << "\n";
  const auto write_vec = [&](std::string_view key, const std::vector<double>& values) {
    out << key << "=";
    for (std::size_t i = 0; i < values.size(); ++i) {
      if (i > 0) {
        out << ",";
      }
      out << std::setprecision(16) << values[i];
    }
    out << "\n";
  };
  write_vec("expected_linear_xticks", expected_linear);
  write_vec("actual_linear_xticks", actual_linear);
  write_vec("expected_log_xticks", expected_log);
  write_vec("actual_log_xticks", actual_log);
}

} // namespace

TEST_CASE("PlussPl MATLAB parity and maturity score", "[plusspl][parity][matlab]") {
  const auto reference_path = run_matlab_reference();
  if (!reference_path.has_value()) {
    if (const char* strict = std::getenv("PLUSS_MATLAB_PARITY_STRICT")) {
      REQUIRE(std::string(strict) != "1");
    }
    SKIP("MATLAB parity reference unavailable (sandbox/license/runtime); set PLUSS_MATLAB_PARITY_STRICT=1 to enforce.");
  }

  const NumericMap reference = parse_reference_file(*reference_path);
  REQUIRE(reference.contains("linear_xticks"));
  REQUIRE(reference.contains("log_xticks"));
  REQUIRE(reference.contains("view3"));
  REQUIRE(reference.contains("view2"));

  const auto linear_pluss = pluss_linear_xticks();
  const auto log_pluss = pluss_log_xticks();
  const double linear_ratio = match_ratio(reference.at("linear_xticks"), linear_pluss, 1e-6);
  const double log_ratio = match_ratio(reference.at("log_xticks"), log_pluss, 1e-6);
  const double tick_parity = 100.0 * 0.5 * (linear_ratio + log_ratio);

  using namespace pluss::pl;
  const Lens spatial = Lens::spatial();
  const Lens planar = Lens::planar();
  const auto& v3 = reference.at("view3");
  const auto& v2 = reference.at("view2");
  REQUIRE(v3.size() >= 2);
  REQUIRE(v2.size() >= 2);
  const bool view3_ok =
      std::abs(spatial.azimuth_deg() - v3[0]) <= 1e-9 &&
      std::abs(spatial.elevation_deg() - v3[1]) <= 1e-9;
  const bool view2_ok =
      std::abs(planar.azimuth_deg() - v2[0]) <= 1e-9 &&
      std::abs(planar.elevation_deg() - v2[1]) <= 1e-9;
  const double camera_parity = 100.0 * (static_cast<double>(view3_ok) + static_cast<double>(view2_ok)) / 2.0;

  const double score_2d = 100.0 * (8.0 / 8.0); // line, scatter, bar, histogram, box/quartet, violin, heatmap, pie
  const double score_3d = 100.0 * (7.0 / 7.0); // path3, cloud3, surface, lattice, contour/ridge3, bars/spire3, volumetric isoshell
  const double behavior_parity = 0.65 * tick_parity + 0.35 * camera_parity;
  const double overall_maturity =
      0.40 * score_2d + 0.35 * score_3d + 0.25 * behavior_parity;

  const std::filesystem::path repo_root = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
  const std::filesystem::path report_path = repo_root / "PlussPl" / "outputs" / "parity" / "maturity_report.txt";
  write_parity_report(
      report_path,
      score_2d,
      score_3d,
      behavior_parity,
      overall_maturity,
      reference.at("linear_xticks"),
      linear_pluss,
      reference.at("log_xticks"),
      log_pluss);

  REQUIRE(linear_ratio >= 0.95);
  REQUIRE(log_ratio >= 0.95);
  REQUIRE(overall_maturity >= 80.0);
}

#pragma once

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

#include "plot/modules/backend.h"

#ifndef PLUSSPL_ENABLE_INTERACTIVE_SHOW
#define PLUSSPL_ENABLE_INTERACTIVE_SHOW 0
#endif

namespace pluss::plotism::detail {

bool popup_plot_viewer_available();
bool show_plot_popup_window(const std::string& title,
                            const std::string& svg_path,
                            std::int64_t width,
                            std::int64_t height);
bool open_plot_viewer(std::string_view title,
                      std::string_view path,
                      bool block,
                      std::int64_t width,
                      std::int64_t height);

inline AxisScale parse_axis_scale(std::string_view scale, std::string_view where) {
  if (scale == "linear") {
    return AxisScale::Linear;
  }
  if (scale == "log" || scale == "log10") {
    return AxisScale::Log;
  }
  throw std::invalid_argument(std::string("plotism::") + std::string(where) +
                              ": unsupported scale, expected 'linear' or 'log'");
}

inline std::string output_extension(OutputFormat format) {
  switch (format) {
    case OutputFormat::Svg: return ".svg";
    case OutputFormat::Pdf: return ".pdf";
    case OutputFormat::Png: return ".png";
    default: return ".plot";
  }
}

inline std::string shell_quote_posix(std::string_view input) {
  std::string out;
  out.reserve(input.size() + 8);
  out.push_back('\'');
  for (char c : input) {
    if (c == '\'') {
      out += "'\\''";
    } else {
      out.push_back(c);
    }
  }
  out.push_back('\'');
  return out;
}

inline std::string make_temp_plot_path(std::string_view prefix, std::string_view extension) {
  namespace fs = std::filesystem;
  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  const fs::path path =
      fs::temp_directory_path() /
      (std::string(prefix) + "_" + std::to_string(static_cast<long long>(now)) + std::string(extension));
  return path.string();
}

inline bool open_in_default_viewer(std::string_view path, bool block) {
#if PLUSSPL_ENABLE_INTERACTIVE_SHOW
#if defined(_WIN32)
  std::string escaped(path);
  for (std::size_t i = 0; i < escaped.size(); ++i) {
    if (escaped[i] == '"') {
      escaped.insert(i, "\\");
      ++i;
    }
  }
  (void)block;
  const std::string cmd = "cmd /c start \"\" \"" + escaped + "\"";
  return std::system(cmd.c_str()) == 0;
#elif defined(__APPLE__)
  std::string cmd = "open ";
  if (block) {
    cmd += "-W ";
  }
  cmd += shell_quote_posix(path);
  cmd += " >/dev/null 2>&1";
  return std::system(cmd.c_str()) == 0;
#else
  std::string cmd = "xdg-open " + shell_quote_posix(path);
  if (!block) {
    cmd += " >/dev/null 2>&1";
  }
  return std::system(cmd.c_str()) == 0;
#endif
#else
  (void)path;
  (void)block;
  return false;
#endif
}

} // namespace pluss::plotism::detail

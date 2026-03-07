#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>

namespace pluss::table {

class Thread;
class DataFrame;
class Atlas;

namespace display {

enum class DisplayMode : std::uint8_t {
  Auto = 0,
  Compact = 1,
  Wide = 2,
};

enum class DisplayBackend : std::uint8_t {
  Auto = 0,
  Terminal = 1,
  NativeWebView = 2,
  InHouseHtml = 3,
  // Backward-compatible alias for existing call sites.
  GtkWebKit = NativeWebView,
};

struct TerminalInfo {
  std::int64_t columns{120};
  std::int64_t rows{40};
  bool is_tty{false};
};

struct DisplayOptions {
  DisplayMode mode{DisplayMode::Auto};
  DisplayBackend backend{DisplayBackend::Auto};
  std::int64_t terminal_width{0};  // <= 0 => detect from terminal
  std::int64_t terminal_height{0}; // <= 0 => detect from terminal
  bool show_shape_footer{true};
  bool show_case_index{true};
  bool color{false};
  std::int64_t viewer_width{1280};
  std::int64_t viewer_height{760};
  bool html_sortable{true};
  bool html_search{true};
  bool html_copy_button{true};
  bool html_desktop_table_viewport{false};
  std::string window_title{};
};

TerminalInfo detect_terminal();
DisplayBackend default_backend();
void set_default_backend(DisplayBackend backend);
bool gui_backend_available();

std::string render(const Thread& thread, const DisplayOptions& options = {});
std::string render(const DataFrame& frame, const DisplayOptions& options = {});
std::string render(const Atlas& atlas, const DisplayOptions& options = {});
std::string render_info(const DataFrame& frame, const DisplayOptions& options = {});
std::string render_html(const Thread& thread, const DisplayOptions& options = {});
std::string render_html(const DataFrame& frame, const DisplayOptions& options = {});
std::string render_html(const Atlas& atlas, const DisplayOptions& options = {});
std::string render_info_html(const DataFrame& frame, const DisplayOptions& options = {});

void print(std::ostream& os, const Thread& thread, const DisplayOptions& options = {});
void print(std::ostream& os, const DataFrame& frame, const DisplayOptions& options = {});
void print(std::ostream& os, const Atlas& atlas, const DisplayOptions& options = {});
void print_info(std::ostream& os, const DataFrame& frame, const DisplayOptions& options = {});
void show(const Thread& thread, const DisplayOptions& options = {});
void show(const DataFrame& frame, const DisplayOptions& options = {});
void show(const Atlas& atlas, const DisplayOptions& options = {});
void show_info(const DataFrame& frame, const DisplayOptions& options = {});

} // namespace display

} // namespace pluss::table

#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>

namespace pluss::table {

class Thread;
class DataFrame;
class Atlas;

namespace display {

struct TerminalInfo {
  std::int64_t columns{120};
  std::int64_t rows{40};
  bool is_tty{false};
};

struct DisplayOptions {
  std::int64_t max_rows{20};
  std::int64_t max_columns{20};
  std::int64_t edge_rows{4};
  std::int64_t edge_columns{3};
  std::int64_t max_cell_width{28};
  std::int64_t min_column_width{6};
  std::int64_t terminal_width{0};  // <= 0 => detect from terminal
  std::int64_t terminal_height{0}; // <= 0 => detect from terminal
  bool show_shape_footer{true};
  bool show_case_index{true};
  bool color{false};
};

TerminalInfo detect_terminal();

std::string render(const Thread& thread, const DisplayOptions& options = {});
std::string render(const DataFrame& frame, const DisplayOptions& options = {});
std::string render(const Atlas& atlas, const DisplayOptions& options = {});
std::string render_info(const DataFrame& frame, const DisplayOptions& options = {});

void print(std::ostream& os, const Thread& thread, const DisplayOptions& options = {});
void print(std::ostream& os, const DataFrame& frame, const DisplayOptions& options = {});
void print(std::ostream& os, const Atlas& atlas, const DisplayOptions& options = {});
void print_info(std::ostream& os, const DataFrame& frame, const DisplayOptions& options = {});

} // namespace display

} // namespace pluss::table


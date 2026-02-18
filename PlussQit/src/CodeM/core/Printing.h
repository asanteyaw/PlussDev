#pragma once
#include <cstdint>
#include <iosfwd>
#include <string>
#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem {

enum class PrintNumberMode : std::uint8_t {
  General,
  Fixed,
  Scientific,
};

enum class PrintAlignMode : std::uint8_t {
  Right,
  Left,
  Decimal,
};

enum class PrintBracketStyle : std::uint8_t {
  Unicode,
  Ascii,
  None,
};

enum class PrintSliceSampling : std::uint8_t {
  HeadTail,
  Uniform,
};

enum class PrintBoolStyle : std::uint8_t {
  TrueFalse,
  OneZero,
};

enum class PrintComplexStyle : std::uint8_t {
  ParenJ,     // (a+bj)
  ParenI,     // (a+bi)
  CartesianJ, // a + bj
  CartesianI, // a + bi
};

enum class PrintMetadataMode : std::uint8_t {
  Full,
  Compact,
  None,
};

enum class PrintContentMode : std::uint8_t {
  Both,
  HeaderOnly,
  ValuesOnly,
};

struct PrintOptions {
  std::int64_t edge_items = 3;     // items to show at each edge per dimension
  std::int64_t max_elements = 200; // beyond this, summarize

  // Standard stream (`operator<<` / to_string) rank-2 summarization policy.
  std::int64_t legacy_rank2_max_rows = 7;
  std::int64_t legacy_rank2_max_cols = 5;
  std::int64_t legacy_rank2_head_items = 2;
  std::int64_t legacy_rank2_tail_items = 1;

  // Header/value visibility.
  PrintMetadataMode metadata_mode = PrintMetadataMode::Full;
  PrintContentMode content_mode = PrintContentMode::Both;
  bool show_dtype = true;
  bool show_shape = true;
  bool show_strides = true;
  bool show_numel = true;
  bool show_platform = true;
  bool show_contiguous = true;

  // Number formatting.
  PrintNumberMode number_mode = PrintNumberMode::General;
  std::int64_t precision = 8;
  bool trim_trailing_zeros = false;

  // Type-specific text style.
  PrintBoolStyle bool_style = PrintBoolStyle::TrueFalse;
  PrintComplexStyle complex_style = PrintComplexStyle::ParenJ;

  // Settings used by `print_series(...)` (aligned matrix view).
  PrintAlignMode pretty_align = PrintAlignMode::Right;
  PrintBracketStyle pretty_bracket_style = PrintBracketStyle::Unicode;
  bool pretty_values_with_commas = false;
  bool pretty_show_slice_indices = true;
  std::string pretty_slice_label_prefix = "values";
  std::string pretty_slice_label_suffix = " =";
  PrintSliceSampling pretty_slice_sampling = PrintSliceSampling::HeadTail;
  std::int64_t pretty_matrix_max_rows = 7; // if rows > this: show head/tail rows with ellipsis
  std::int64_t pretty_matrix_max_cols = 5; // if cols > this: show head/tail cols with ellipsis
  std::int64_t pretty_row_head_items = 2;  // leading rows kept when summarized
  std::int64_t pretty_row_tail_items = 1;  // trailing rows kept when summarized
  std::int64_t pretty_col_head_items = 2;  // leading cols kept when summarized
  std::int64_t pretty_col_tail_items = 1;  // trailing cols kept when summarized
  std::int64_t pretty_slice_head_items = 2;
  std::int64_t pretty_slice_tail_items = 1;
  std::int64_t pretty_max_slices = 7;      // for rank > 2, summarize number of 2D slices
  std::int64_t pretty_max_line_width = 0;  // 0 disables line-width based column reduction
  std::int64_t pretty_cell_max_width = 0;  // 0 disables clipping
  bool pretty_auto_wrap = false;           // wraps very wide matrices into column windows

  // Optional ANSI color.
  bool enable_color = false;
  bool color_negative = true;
  bool color_special = true;
};

std::string to_string(const Series& s, const PrintOptions& opt = {});
std::string print_series(const Series& s, const PrintOptions& opt = {});
std::ostream& print_series(std::ostream& os, const Series& s, const PrintOptions& opt = {});
std::ostream& operator<<(std::ostream& os, const Series& s);

} // namespace pluss::codem

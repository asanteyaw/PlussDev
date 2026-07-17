#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "loader/CsvLoader.h"

namespace pluss::table {
class DataFrame;
}

namespace pluss::loader {

struct ReadExcelOptions {
  bool header{true};
  std::int64_t header_row{0};
  std::size_t header_rows{1};
  bool header_ffill{true};
  std::string header_joiner{"_"};
  std::optional<std::string> sheet_name{};
  std::optional<std::int64_t> sheet_index{};
  std::vector<std::string> sheet_names{};
  std::vector<std::int64_t> sheet_indices{};
  std::optional<std::size_t> n_sheets{};
  bool all_sheets{false};
  std::optional<std::int64_t> start_row{};
  std::optional<std::int64_t> end_row{};
  std::optional<std::int64_t> start_col{};
  std::optional<std::int64_t> end_col{};
  bool trim_outer_empty{true};
  bool auto_table_block{false};
  std::size_t auto_min_non_empty_cells{2};
  std::size_t auto_max_blank_gap{1};
};

struct WriteExcelOptions {
  std::string sheet_name{"Sheet1"};
  bool add_cases{false};
  bool cases{false}; // legacy alias
  bool header{true};
  std::optional<std::string> case_label{};
  std::string null_rep{};
};

using Workbook = std::vector<std::pair<std::string, pluss::table::DataFrame>>;
using ExcelBatchWorkbook = std::vector<std::pair<std::string, CsvBatch>>;
using ExcelRawBatchWorkbook = std::vector<std::pair<std::string, CsvRawBatch>>;

CsvRawBatch read_excel_raw_batch(std::string_view filepath, const ReadExcelOptions& options = {});
ExcelRawBatchWorkbook read_excel_workbook_raw_batch(std::string_view filepath,
                                                    const ReadExcelOptions& options = {});

CsvBatch read_excel_batch(std::string_view filepath, const ReadExcelOptions& options = {});
ExcelBatchWorkbook read_excel_workbook_batch(std::string_view filepath,
                                             const ReadExcelOptions& options = {});

pluss::table::DataFrame load_excel(std::string_view filepath, const ReadExcelOptions& options = {});
Workbook load_excel_workbook(
    std::string_view filepath,
    const ReadExcelOptions& options = {});
void save_excel(const pluss::table::DataFrame& frame,
                std::string_view filepath,
                const WriteExcelOptions& options = {});

} // namespace pluss::loader

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pluss::table {
class DataFrame;
}

namespace pluss::loader {

struct ReadExcelOptions {
  bool header{true};
  std::optional<std::string> sheet_name{};
  std::optional<std::int64_t> sheet_index{};
  std::vector<std::string> sheet_names{};
  std::vector<std::int64_t> sheet_indices{};
  std::optional<std::size_t> n_sheets{};
  bool all_sheets{false};
};

struct WriteExcelOptions {
  std::string sheet_name{"Sheet1"};
  bool add_cases{false};
  bool cases{false}; // legacy alias
  bool header{true};
  std::optional<std::string> case_label{};
  std::string na_rep{"NA"};
};

using Workbook = std::vector<std::pair<std::string, pluss::table::DataFrame>>;

pluss::table::DataFrame load_excel(std::string_view filepath, const ReadExcelOptions& options = {});
Workbook load_excel_workbook(
    std::string_view filepath,
    const ReadExcelOptions& options = {});
void save_excel(const pluss::table::DataFrame& frame,
                std::string_view filepath,
                const WriteExcelOptions& options = {});

} // namespace pluss::loader

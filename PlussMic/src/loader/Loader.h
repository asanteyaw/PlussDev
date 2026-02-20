#pragma once

#include <cstddef>
#include <initializer_list>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "loader/Config.h"
#include "loader/CsvLoader.h"
#include "loader/ExcelLoader.h"
#include "table/DataFrame.h"

namespace pluss::loader {

class SheetNames {
public:
  SheetNames() = default;
  explicit SheetNames(std::vector<std::string> names);

  const std::vector<std::string>& values() const noexcept;
  std::size_t size() const noexcept;
  bool empty() const noexcept;

private:
  std::vector<std::string> names_{};
};

std::ostream& operator<<(std::ostream& os, const SheetNames& names);

class LoadResult {
public:
  LoadResult() = default;
  explicit LoadResult(pluss::table::DataFrame frame);
  explicit LoadResult(Workbook workbook);

  bool is_frame() const noexcept;
  bool is_workbook() const noexcept;

  const pluss::table::DataFrame& dataframe() const;
  pluss::table::DataFrame& dataframe();

  const Workbook& workbook() const;
  Workbook& workbook();

  const pluss::table::DataFrame& sheets(std::string_view name) const;
  pluss::table::DataFrame& sheets(std::string_view name);

  const pluss::table::DataFrame& sheet_by_index(std::size_t index) const;
  pluss::table::DataFrame& sheet_by_index(std::size_t index);

  SheetNames sheet_names() const;

private:
  bool is_workbook_{false};
  pluss::table::DataFrame frame_{};
  Workbook workbook_{};
};

std::ostream& operator<<(std::ostream& os, const LoadResult& loaded);

// Format-specific first-class loaders (recommended public API).
pluss::table::DataFrame load_csv(std::string_view filepath, const Config& config);
pluss::table::DataFrame load_csv(std::string_view filepath,
                                 std::initializer_list<ConfigEntry> config_entries);
pluss::table::DataFrame load_csv(std::string_view filepath,
                                 std::string_view header,
                                 std::string_view case_feature = "none",
                                 std::string_view engine = "auto",
                                 std::optional<std::size_t> ncases = std::nullopt);
pluss::table::DataFrame load_csv(std::string_view filepath,
                                 std::string_view header,
                                 std::size_t ncases);
pluss::table::DataFrame load_excel(std::string_view filepath, const Config& config);
pluss::table::DataFrame load_excel(std::string_view filepath,
                                   std::initializer_list<ConfigEntry> config_entries);
pluss::table::DataFrame load_excel(std::string_view filepath,
                                   std::string_view sheet_name,
                                   bool header = true);
pluss::table::DataFrame load_excel(std::string_view filepath,
                                   std::int64_t sheet_index,
                                   bool header = true);
LoadResult load_excel(std::string_view filepath,
                      std::nullptr_t,
                      bool header = true);
LoadResult load_excel(std::string_view filepath,
                      const std::vector<std::string>& sheet_names,
                      bool header = true);
LoadResult load_excel(std::string_view filepath,
                      std::initializer_list<std::string_view> sheet_names,
                      bool header = true);
LoadResult load_excel(std::string_view filepath,
                      const std::vector<std::int64_t>& sheet_indices,
                      bool header = true);
LoadResult load_excel(std::string_view filepath,
                      std::initializer_list<std::int64_t> sheet_indices,
                      bool header = true);
Workbook load_excel_workbook(std::string_view filepath, const Config& config);
Workbook load_excel_workbook(std::string_view filepath,
                             std::initializer_list<ConfigEntry> config_entries);
pluss::table::DataFrame load_parquet(std::string_view filepath, const Config& config = {});
pluss::table::DataFrame load_parquet(std::string_view filepath,
                                     std::initializer_list<ConfigEntry> config_entries);

// Central delegating loader (kept for dynamic dispatch-style workflows).
LoadResult load(std::string_view source_type, std::string_view filepath, const Config& config = {});
LoadResult load(std::string_view source_type,
                std::string_view filepath,
                std::initializer_list<ConfigEntry> config_entries);

void save_as(const pluss::table::DataFrame& frame,
             std::string_view target_type,
             std::string_view filepath,
             const Config& config = {});
void save_as(const pluss::table::DataFrame& frame,
             std::string_view target_type,
             std::string_view filepath,
             std::initializer_list<ConfigEntry> config_entries);

} // namespace pluss::loader

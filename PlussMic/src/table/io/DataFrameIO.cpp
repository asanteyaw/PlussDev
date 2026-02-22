#include "table/DataFrame.h"

#include <iostream>
#include <ostream>
#include <string>
#include <utility>

#include "loader/Config.h"
#include "loader/Loader.h"
#include "table/display/Display.h"

namespace pluss::table {

std::string DataFrame::to_string(const display::DisplayOptions& options) const {
  return display::render(*this, options);
}

std::string DataFrame::info_string(const display::DisplayOptions& options) const {
  return display::render_info(*this, options);
}

void DataFrame::info(const display::DisplayOptions& options) const {
  display::print_info(std::cout, *this, options);
}

void DataFrame::info(std::ostream& os, const display::DisplayOptions& options) const {
  display::print_info(os, *this, options);
}

void DataFrame::save_csv(std::string_view filepath) const {
  pluss::loader::save_csv(*this, filepath, {});
}

void DataFrame::save_csv(std::string_view filepath,
                         bool add_cases,
                         bool header,
                         char sep,
                         std::string na_rep,
                         std::optional<std::string> case_label,
                         std::optional<std::string> float_format,
                         char quotechar,
                         bool doublequote,
                         std::string line_terminator,
                         char decimal,
                         std::string encoding) const {
  pluss::loader::WriteCsvOptions options;
  options.sep = sep;
  options.header = header;
  options.add_cases = add_cases;
  options.cases = add_cases;
  options.na_rep = std::move(na_rep);
  options.case_label = std::move(case_label);
  options.float_format = std::move(float_format);
  options.quotechar = quotechar;
  options.doublequote = doublequote;
  options.line_terminator = std::move(line_terminator);
  options.decimal = decimal;
  options.encoding = std::move(encoding);
  pluss::loader::save_csv(*this, filepath, options);
}

void DataFrame::save_csv(std::string_view filepath, const pluss::loader::WriteCsvOptions& options) const {
  pluss::loader::save_csv(*this, filepath, options);
}

void DataFrame::save_csv(std::string_view filepath, const pluss::loader::Config& config) const {
  save_as("csv", filepath, config);
}

void DataFrame::save_csv(std::string_view filepath,
                         std::initializer_list<pluss::loader::ConfigEntry> config_entries) const {
  save_as("csv", filepath, config_entries);
}

void DataFrame::save_excel(std::string_view filepath) const {
  pluss::loader::save_excel(*this, filepath, {});
}

void DataFrame::save_excel(std::string_view filepath,
                           std::string sheet_name,
                           bool add_cases,
                           bool header,
                           std::string na_rep,
                           std::optional<std::string> case_label) const {
  pluss::loader::WriteExcelOptions options;
  options.sheet_name = std::move(sheet_name);
  options.add_cases = add_cases;
  options.cases = add_cases;
  options.header = header;
  options.na_rep = std::move(na_rep);
  options.case_label = std::move(case_label);
  pluss::loader::save_excel(*this, filepath, options);
}

void DataFrame::save_excel(std::string_view filepath, const pluss::loader::WriteExcelOptions& options) const {
  pluss::loader::save_excel(*this, filepath, options);
}

void DataFrame::save_excel(std::string_view filepath, const pluss::loader::Config& config) const {
  save_as("excel", filepath, config);
}

void DataFrame::save_excel(std::string_view filepath,
                           std::initializer_list<pluss::loader::ConfigEntry> config_entries) const {
  save_as("excel", filepath, config_entries);
}

void DataFrame::save_as(std::string_view target_type,
                        std::string_view filepath,
                        const pluss::loader::Config& config) const {
  pluss::loader::save_as(*this, target_type, filepath, config);
}

void DataFrame::save_as(std::string_view target_type,
                        std::string_view filepath,
                        std::initializer_list<pluss::loader::ConfigEntry> config_entries) const {
  save_as(target_type, filepath, pluss::loader::config_from(config_entries));
}

std::ostream& operator<<(std::ostream& os, const DataFrame& frame) {
  if (frame.consume_stream_suppression_()) {
    return os;
  }
  os << frame.to_string();
  return os;
}

} // namespace pluss::table

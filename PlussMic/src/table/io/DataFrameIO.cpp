#include "table/DataFrame.h"

#include <exception>
#include <iostream>
#include <ostream>
#include <string>
#include <utility>

#include "loader/Config.h"
#include "loader/Loader.h"
#include "table/display/Display.h"

namespace pluss::table {

struct DataFrameView::State {
  bool auto_render_terminal{false};
  mutable bool consumed{false};
  std::string operation{};
  std::string source_title{};
};

std::string DataFrame::to_string(const display::DisplayOptions& options) const {
  return display::render(*this, options);
}

std::string DataFrame::info_string(const display::DisplayOptions& options) const {
  return display::render_info(*this, options);
}

void DataFrame::show(const display::DisplayOptions& options) const {
  auto effective = options;
  if (effective.window_title.empty()) {
    effective.window_title = title().empty() ? "DataFrame" : title();
  }
  display::show(*this, effective);
}

void DataFrame::show(std::ostream& os, const display::DisplayOptions& options) const {
  display::print(os, *this, options);
}

void DataFrame::info(const display::DisplayOptions& options) const {
  display::print_info(std::cout, *this, options);
}

void DataFrame::info(std::ostream& os, const display::DisplayOptions& options) const {
  display::print_info(os, *this, options);
}

DataFrameView::DataFrameView() : DataFrame(), state_(std::make_shared<State>()) {}

DataFrameView::DataFrameView(DataFrame frame,
                             bool auto_render_terminal,
                             std::string operation,
                             std::string source_title)
    : DataFrame(std::move(frame)), state_(std::make_shared<State>()) {
  state_->auto_render_terminal = auto_render_terminal;
  state_->operation = std::move(operation);
  state_->source_title = std::move(source_title);
  if (state_->source_title.empty()) {
    state_->source_title = DataFrame::title();
  }
}

DataFrameView::DataFrameView(const DataFrameView& other) : DataFrame(other), state_(other.state_) {}

DataFrameView::DataFrameView(DataFrameView&& other) noexcept : DataFrame(std::move(other)), state_(std::move(other.state_)) {}

DataFrameView& DataFrameView::operator=(const DataFrameView& other) {
  if (this == &other) {
    return *this;
  }
  DataFrame::operator=(other);
  state_ = other.state_;
  return *this;
}

DataFrameView& DataFrameView::operator=(DataFrameView&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  DataFrame::operator=(std::move(other));
  state_ = std::move(other.state_);
  return *this;
}

DataFrameView::~DataFrameView() {
  maybe_autorender_();
}

void DataFrameView::consume_pending_display_() const {
  if (!state_) {
    return;
  }
  state_->consumed = true;
}

void DataFrameView::maybe_autorender_() const {
  if (!state_ || !state_->auto_render_terminal || state_->consumed || std::uncaught_exceptions() > 0) {
    return;
  }
  state_->consumed = true;
  display::print(std::cout, static_cast<const DataFrame&>(*this));
}

void DataFrameView::show(const display::DisplayOptions& options) const {
  consume_pending_display_();
  auto effective = options;
  if (effective.window_title.empty()) {
    effective.window_title = resolved_window_title_();
  }
  DataFrame::show(effective);
}

void DataFrameView::show(std::ostream& os, const display::DisplayOptions& options) const {
  consume_pending_display_();
  auto effective = options;
  if (effective.window_title.empty()) {
    effective.window_title = resolved_window_title_();
  }
  DataFrame::show(os, effective);
}

std::string DataFrameView::resolved_window_title_() const {
  const auto base = (!state_ || state_->source_title.empty()) ? std::string("DataFrame") : state_->source_title;
  if (!state_ || state_->operation.empty()) {
    return base;
  }
  return base + " - " + state_->operation;
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

void DataFrame::save_txt(std::string_view filepath) const {
  save_as("txt", filepath, {});
}

void DataFrame::save_txt(std::string_view filepath, const pluss::loader::Config& config) const {
  save_as("txt", filepath, config);
}

void DataFrame::save_txt(std::string_view filepath,
                         std::initializer_list<pluss::loader::ConfigEntry> config_entries) const {
  save_as("txt", filepath, config_entries);
}

void DataFrame::save_json(std::string_view filepath) const {
  save_as("json", filepath, {});
}

void DataFrame::save_json(std::string_view filepath, const pluss::loader::Config& config) const {
  save_as("json", filepath, config);
}

void DataFrame::save_json(std::string_view filepath,
                          std::initializer_list<pluss::loader::ConfigEntry> config_entries) const {
  save_as("json", filepath, config_entries);
}

void DataFrame::save_parquet(std::string_view filepath) const {
  save_as("parquet", filepath, {});
}

void DataFrame::save_parquet(std::string_view filepath, const pluss::loader::Config& config) const {
  save_as("parquet", filepath, config);
}

void DataFrame::save_parquet(std::string_view filepath,
                             std::initializer_list<pluss::loader::ConfigEntry> config_entries) const {
  save_as("parquet", filepath, config_entries);
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
  output_render(os, frame);
  return os;
}

void output_render(std::ostream& os, const DataFrame& frame) {
  if (frame.consume_stream_suppression_()) {
    return;
  }
  os << frame.to_string();
}

std::ostream& operator<<(std::ostream& os, const DataFrameView& frame) {
  output_render(os, frame);
  return os;
}

void output_render(std::ostream& os, const DataFrameView& frame) {
  frame.consume_pending_display_();
  output_render(os, static_cast<const DataFrame&>(frame));
}

} // namespace pluss::table

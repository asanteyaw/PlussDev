#include "kindle/display/GridModel.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string_view>
#include <utility>

#include "kindle/Stream.h"
#include "table/Shelf.h"

namespace pluss::kindle::display {

namespace {

using pluss::table::display::DisplayAlign;
using pluss::table::display::DisplayFilter;

// The kinds of entry the filter can look for, as bits.
constexpr std::uint8_t kNullBit = 1;
constexpr std::uint8_t kNaNBit = 2;
constexpr std::uint8_t kInfBit = 4;

std::uint8_t wanted_bits(const DisplayFilter filter) {
  switch (filter) {
    case DisplayFilter::Invalid: return kNullBit | kNaNBit | kInfBit;
    case DisplayFilter::Null: return kNullBit;
    case DisplayFilter::NaN: return kNaNBit;
    case DisplayFilter::Infinite: return kInfBit;
    case DisplayFilter::None: break;
  }
  return 0;
}

std::string folded(std::string_view text) {
  std::string out(text);
  for (char& ch : out) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return out;
}

}  // namespace

struct CatalogGridModel::Column {
  std::string name{};
  std::int64_t entries{0};
  bool numeric{false};
  pluss::table::Shelf shelf{};
  std::optional<pluss::Note> text{};  // every entry as text (a Note has no default state)
  std::vector<bool> missing{};        // empty when the stream holds no NULL
  std::vector<std::uint8_t> kinds{};  // per entry, the filter bits it has; filled when a filter is first used
  bool classified{false};
};

CatalogGridModel::CatalogGridModel(const Catalog& catalog) {
  for (const auto& name : catalog.keys()) {
    const auto stream = catalog.keys(name);
    auto column = std::make_shared<Column>();
    column->name = name;
    column->entries = stream.size();
    column->shelf = stream.to_shelf();
    if (column->shelf.kind() == pluss::table::ShelfKind::Series) {
      const auto dtype = column->shelf.as_series().dtype();
      column->numeric = dtype.is_floating() || dtype.is_integral();
    }
    column->text.emplace(column->shelf.cast().note().as_note());
    if (pluss::table::detail::has_mask(column->shelf)) {
      const auto valid = pluss::table::detail::validity_mask(column->shelf)->host<bool>();
      column->missing.assign(valid.size(), false);
      for (std::size_t i = 0; i < valid.size(); ++i) {
        column->missing[i] = !valid[i];
      }
    }
    longest_ = std::max(longest_, column->entries);
    columns_.push_back(std::move(column));
  }
  shortest_ = columns_.empty() ? 0 : longest_;
  for (const auto& column : columns_) {
    shortest_ = std::min(shortest_, column->entries);
  }
}

void CatalogGridModel::set_alignment(const DisplayAlign align) const {
  tail_.store(align == DisplayAlign::Tail, std::memory_order_release);
}

void CatalogGridModel::set_filter(const DisplayFilter filter) const {
  {
    const std::scoped_lock lock(mutex_);
    filter_ = filter;
  }
  if (filter != DisplayFilter::None) {
    for (const auto& column : columns_) {
      classify_(*column);
    }
  }
  filtering_.store(filter != DisplayFilter::None, std::memory_order_release);
}

void CatalogGridModel::set_number_format(const pluss::table::display::NumberFormat format) const {
  number_format_.store(static_cast<int>(format), std::memory_order_release);
}

void CatalogGridModel::set_window(const pluss::view::RowWindow& window) const {
  const std::scoped_lock lock(mutex_);
  window_ = window;
}

pluss::view::RowWindow CatalogGridModel::row_window() const {
  const std::scoped_lock lock(mutex_);
  return window_;
}

std::int64_t CatalogGridModel::entry_at(const Column& column, const std::int64_t row) const {
  const std::int64_t offset = tail_.load(std::memory_order_acquire) ? longest_ - column.entries : 0;
  const std::int64_t entry = row - offset;
  return entry >= 0 && entry < column.entries ? entry : -1;
}

// What each entry is, once, by the library's own tests (isnull, isnan, isinf), never by its text.
void CatalogGridModel::classify_(Column& column) const {
  const std::scoped_lock lock(mutex_);
  if (column.classified) {
    return;
  }
  column.kinds.assign(static_cast<std::size_t>(column.entries), 0);
  const auto mark = [&](const pluss::table::Shelf& flags, const std::uint8_t bit) {
    const auto host = flags.as_series().to(pluss::codem::ScalarType::Bool).host<bool>();
    for (std::size_t i = 0; i < host.size() && i < column.kinds.size(); ++i) {
      if (host[i]) {
        column.kinds[i] |= bit;
      }
    }
  };
  mark(column.shelf.isnull(), kNullBit);
  if (column.numeric) {
    mark(column.shelf.isnan(), kNaNBit);
    mark(column.shelf.isinf(), kInfBit);
  }
  column.classified = true;
}

std::string CatalogGridModel::column_name(const std::int64_t column) const {
  if (column < 0 || column >= keys()) {
    return {};
  }
  const auto& c = *columns_[static_cast<std::size_t>(column)];
  return c.name + " · " + std::to_string(c.entries);  // the key, and how many entries it really has
}

// How much of the longest key this one covers: the raggedness, at a glance.
double CatalogGridModel::column_meter(const std::int64_t column) const {
  if (column < 0 || column >= keys() || longest_ <= 0) {
    return -1.0;
  }
  return static_cast<double>(columns_[static_cast<std::size_t>(column)]->entries) / static_cast<double>(longest_);
}

bool CatalogGridModel::column_numeric(const std::int64_t column) const noexcept {
  return column >= 0 && column < keys() && columns_[static_cast<std::size_t>(column)]->numeric;
}

pluss::view::VirtualTableCell CatalogGridModel::cell(const std::int64_t row, const std::int64_t column) const {
  if (column < 0 || column >= keys()) {
    return {.absent = true};
  }
  const auto& c = *columns_[static_cast<std::size_t>(column)];
  const std::int64_t entry = entry_at(c, row);
  if (entry < 0) {
    return {.absent = true};
  }
  const bool missing = !c.missing.empty() && c.missing[static_cast<std::size_t>(entry)];
  if (missing) {
    return {.text = "NULL", .missing = true};
  }
  std::string text(c.text->at(entry));
  const bool nonfinite = c.numeric && (text == "nan" || text == "-nan" || text == "inf" || text == "-inf");
  if (nonfinite) {
    const bool nan = text == "nan" || text == "-nan";
    text = nan ? "NAN" : (text == "inf" ? "INF" : "-INF");  // the loud forms, as in a DataFrame
    return {.text = std::move(text), .nonfinite = true, .infinite = !nan};
  }
  if (c.numeric) {
    const auto format = static_cast<pluss::table::display::NumberFormat>(number_format_.load(std::memory_order_acquire));
    if (format != pluss::table::display::NumberFormat::Default) {
      text = pluss::table::display::format_number(text, format);
    }
  }
  return {.text = std::move(text)};
}

bool CatalogGridModel::row_matches(const std::int64_t row, const std::string_view folded_query) const {
  if (folded_query.empty()) {
    return true;
  }
  if (folded(row_label(row)).find(folded_query) != std::string::npos) {
    return true;
  }
  for (std::int64_t column = 0; column < keys(); ++column) {
    const auto value = cell(row, column);
    if (!value.absent && folded(value.text).find(folded_query) != std::string::npos) {
      return true;
    }
  }
  return false;
}

bool CatalogGridModel::row_allowed(const std::int64_t row) const {
  std::uint8_t wanted = 0;
  {
    const std::scoped_lock lock(mutex_);
    wanted = wanted_bits(filter_);
  }
  if (wanted == 0) {
    return true;
  }
  for (const auto& column : columns_) {
    const std::int64_t entry = entry_at(*column, row);
    if (entry >= 0 && column->classified && (column->kinds[static_cast<std::size_t>(entry)] & wanted) != 0) {
      return true;
    }
  }
  return false;
}

}  // namespace pluss::kindle::display

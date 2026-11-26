#include "table/display/RaggedGrid.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <utility>

#include "table/Shelf.h"

namespace pluss::table::display {

namespace {

std::uint8_t wanted_bits(const DisplayFilter filter) {
  switch (filter) {
    case DisplayFilter::Invalid: return kEntryNull | kEntryNaN | kEntryInf;
    case DisplayFilter::Null: return kEntryNull;
    case DisplayFilter::NaN: return kEntryNaN;
    case DisplayFilter::Infinite: return kEntryInf;
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

// What a shelf's entries are, once, by the library's own tests (isnull, isnan, isinf), never by their text.
RaggedColumn ragged_column_from_shelf(std::string name, const Shelf& shelf) {
  RaggedColumn column;
  column.name = std::move(name);
  column.entries = shelf.size();
  if (shelf.kind() == ShelfKind::Series) {
    const auto dtype = shelf.as_series().dtype();
    column.numeric = dtype.is_floating() || dtype.is_integral();
  }
  const auto text = std::make_shared<pluss::Note>(shelf.cast().note().as_note());
  column.text = [text](const std::int64_t i) { return std::string(text->at(i)); };
  if (detail::has_mask(shelf)) {
    const auto valid = detail::validity_mask(shelf)->host<bool>();
    auto missing = std::make_shared<std::vector<bool>>(valid.size(), false);
    for (std::size_t i = 0; i < valid.size(); ++i) {
      (*missing)[i] = !valid[i];
    }
    column.missing = [missing](const std::int64_t i) { return (*missing)[static_cast<std::size_t>(i)]; };
  }
  const auto held = std::make_shared<Shelf>(shelf);
  const bool numeric = column.numeric;
  column.classify = [held, numeric](std::vector<std::uint8_t>& kinds) {
    const auto mark = [&](const Shelf& flags, const std::uint8_t bit) {
      const auto host = flags.as_series().to(pluss::codem::ScalarType::Bool).host<bool>();
      for (std::size_t i = 0; i < host.size() && i < kinds.size(); ++i) {
        if (host[i]) {
          kinds[i] |= bit;
        }
      }
    };
    mark(held->isnull(), kEntryNull);
    if (numeric) {
      mark(held->isnan(), kEntryNaN);
      mark(held->isinf(), kEntryInf);
    }
  };
  return column;
}

struct RaggedGridModel::State {
  RaggedColumn source;
  std::vector<std::uint8_t> kinds{};  // per entry, the filter bits it has; filled when a filter is first used
  bool classified{false};
};

RaggedGridModel::RaggedGridModel(std::string row_header, std::vector<RaggedColumn> columns)
    : row_header_(std::move(row_header)) {
  for (auto& column : columns) {
    longest_ = std::max(longest_, column.entries);
    auto state = std::make_shared<State>();
    state->source = std::move(column);
    columns_.push_back(std::move(state));
  }
  shortest_ = columns_.empty() ? 0 : longest_;
  for (const auto& column : columns_) {
    shortest_ = std::min(shortest_, column->source.entries);
  }
}

void RaggedGridModel::set_alignment(const DisplayAlign align) const {
  tail_.store(align == DisplayAlign::Tail, std::memory_order_release);
}

void RaggedGridModel::set_filter(const DisplayFilter filter) const {
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

void RaggedGridModel::set_number_format(const NumberFormat format) const {
  number_format_.store(static_cast<int>(format), std::memory_order_release);
}

void RaggedGridModel::set_window(const pluss::view::RowWindow& window) const {
  const std::scoped_lock lock(mutex_);
  window_ = window;
}

void RaggedGridModel::apply(const DisplayOptions& options) const {
  set_alignment(options.align);
  set_filter(options.filter);
  set_window(row_window_for(options));
  set_number_format(options.number_format);
}

pluss::view::RowWindow RaggedGridModel::row_window() const {
  const std::scoped_lock lock(mutex_);
  return window_;
}

std::int64_t RaggedGridModel::entry_at(const State& column, const std::int64_t row) const {
  const std::int64_t offset = tail_.load(std::memory_order_acquire) ? longest_ - column.source.entries : 0;
  const std::int64_t entry = row - offset;
  return entry >= 0 && entry < column.source.entries ? entry : -1;
}

void RaggedGridModel::classify_(State& column) const {
  const std::scoped_lock lock(mutex_);
  if (column.classified) {
    return;
  }
  column.kinds.assign(static_cast<std::size_t>(column.source.entries), 0);
  if (column.source.classify) {
    column.source.classify(column.kinds);
  } else if (column.source.missing) {
    for (std::int64_t i = 0; i < column.source.entries; ++i) {
      if (column.source.missing(i)) {
        column.kinds[static_cast<std::size_t>(i)] |= kEntryNull;
      }
    }
  }
  column.classified = true;
}

std::string RaggedGridModel::column_name(const std::int64_t column) const {
  if (column < 0 || column >= columns()) {
    return {};
  }
  const auto& c = columns_[static_cast<std::size_t>(column)]->source;
  return c.name + " · " + std::to_string(c.entries);  // the name, and how many entries it really has
}

// How much of the longest column this one covers: the raggedness, at a glance.
double RaggedGridModel::column_meter(const std::int64_t column) const {
  if (column < 0 || column >= columns() || longest_ <= 0) {
    return -1.0;
  }
  return static_cast<double>(columns_[static_cast<std::size_t>(column)]->source.entries) / static_cast<double>(longest_);
}

bool RaggedGridModel::column_numeric(const std::int64_t column) const noexcept {
  return column >= 0 && column < columns() && columns_[static_cast<std::size_t>(column)]->source.numeric;
}

pluss::view::VirtualTableCell RaggedGridModel::cell(const std::int64_t row, const std::int64_t column) const {
  if (column < 0 || column >= columns()) {
    return {.absent = true};
  }
  const auto& c = columns_[static_cast<std::size_t>(column)]->source;
  const std::int64_t entry = entry_at(*columns_[static_cast<std::size_t>(column)], row);
  if (entry < 0) {
    return {.absent = true};
  }
  if (c.missing && c.missing(entry)) {
    return {.text = "NULL", .missing = true};
  }
  std::string text = c.text(entry);
  const bool nonfinite = c.numeric && (text == "nan" || text == "-nan" || text == "inf" || text == "-inf");
  if (nonfinite) {
    const bool nan = text == "nan" || text == "-nan";
    text = nan ? "NAN" : (text == "inf" ? "INF" : "-INF");  // the loud forms, as in a DataFrame
    return {.text = std::move(text), .nonfinite = true, .infinite = !nan};
  }
  if (c.numeric) {
    const auto format = static_cast<NumberFormat>(number_format_.load(std::memory_order_acquire));
    if (format != NumberFormat::Default) {
      text = pluss::table::display::format_number(text, format);
    }
  }
  return {.text = std::move(text)};
}

bool RaggedGridModel::row_matches(const std::int64_t row, const std::string_view folded_query) const {
  if (folded_query.empty()) {
    return true;
  }
  if (folded(row_label(row)).find(folded_query) != std::string::npos) {
    return true;
  }
  for (std::int64_t column = 0; column < columns(); ++column) {
    const auto value = cell(row, column);
    if (!value.absent && folded(value.text).find(folded_query) != std::string::npos) {
      return true;
    }
  }
  return false;
}

bool RaggedGridModel::row_allowed(const std::int64_t row) const {
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

ModelPage ragged_model_page(const std::string& title, const std::string& parts, const std::string& part_word,
                            const std::string& entries_word, std::shared_ptr<RaggedGridModel> model,
                            std::vector<std::pair<std::string, std::shared_ptr<const Shelf>>> streams) {
  ModelPage page;
  page.title = title;
  const bool ragged = model->longest() != model->shortest();
  const auto count = std::to_string(model->columns());
  page.subtitle = ragged ? count + " " + parts + ", " + std::to_string(model->shortest()) + " to " +
                               std::to_string(model->longest()) + " " + entries_word
                         : count + " " + parts + ", " + std::to_string(model->longest()) + " " + entries_word;
  std::string capitalised = parts;
  if (!capitalised.empty()) {
    capitalised[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(capitalised[0])));
  }
  page.cards = {{capitalised, count}, {"Longest", std::to_string(model->longest())}, {"Shortest", std::to_string(model->shortest())}};
  page.streams = std::move(streams);
  page.part_word = part_word;
  page.entries_word = entries_word;
  page.offer_align = ragged;
  page.on_options = [model](const DisplayOptions& options) { model->apply(options); };
  page.model = std::move(model);
  return page;
}

}  // namespace pluss::table::display

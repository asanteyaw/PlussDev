#pragma once

// A grid whose columns are as long as their own data: a catalog's keys side by side, a registry's fields. Nothing is
// padded; where a column has no entry at a row it draws nothing (which is not a NULL). One model holds everything a
// window or a notebook needs of such a grid (alignment, filter, visit window, number format, search, the meter of how
// much of the longest column each one covers), so every ragged type shares it and only supplies its columns.

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/tils/view/html_window.h"
#include "table/display/Display.h"

namespace pluss::table {
class Shelf;
}

namespace pluss::table::display {

// The kinds of entry the filter can look for, as bits.
inline constexpr std::uint8_t kEntryNull = 1;
inline constexpr std::uint8_t kEntryNaN = 2;
inline constexpr std::uint8_t kEntryInf = 4;

// One column: its name, how many entries it has, and how to read them.
struct RaggedColumn {
  std::string name;
  std::int64_t entries{0};
  bool numeric{false};
  std::function<std::string(std::int64_t)> text;  // the text of entry i (never asked for a missing one)
  std::function<bool(std::int64_t)> missing;      // whether entry i has no value; unset: none is missing
  // Fills, per entry, the bits of the kinds of entry it is (null, NaN, infinite); unset: the missing ones are null.
  std::function<void(std::vector<std::uint8_t>&)> classify;
};

// A column over a Shelf of values (what a catalog's stream is, and a registry's field when its cells agree).
RaggedColumn ragged_column_from_shelf(std::string name, const Shelf& shelf);

class RaggedGridModel : public OptionsModel {
 public:
  // `row_header` titles the label column ("entry" for a catalog, "state" for a registry).
  RaggedGridModel(std::string row_header, std::vector<RaggedColumn> columns);

  std::int64_t longest() const noexcept { return longest_; }
  std::int64_t shortest() const noexcept { return shortest_; }
  std::int64_t columns() const noexcept { return static_cast<std::int64_t>(columns_.size()); }

  // Where each column sits in the shared rows: from the top (head) or ending at the bottom (tail).
  void set_alignment(DisplayAlign align) const;
  // Keeps the rows where some column holds the kind of entry the filter looks for.
  void set_filter(DisplayFilter filter) const;
  void set_window(const pluss::view::RowWindow& window) const;
  // How numbers are written (View > Format); the stored values do not change.
  void set_number_format(NumberFormat format) const;
  // All of the above, from a set of options.
  void apply(const DisplayOptions& options) const override;

  std::int64_t row_count() const noexcept override { return longest_; }
  std::int64_t column_count() const noexcept override { return columns(); }
  std::string row_header() const override { return row_header_; }
  std::string row_label(std::int64_t row) const override { return std::to_string(row); }
  std::string column_name(std::int64_t column) const override;
  bool column_numeric(std::int64_t column) const noexcept override;
  pluss::view::VirtualTableCell cell(std::int64_t row, std::int64_t column) const override;
  bool row_matches(std::int64_t row, std::string_view folded_query) const override;
  bool filtering() const noexcept override { return filtering_.load(std::memory_order_acquire); }
  bool row_allowed(std::int64_t row) const override;
  pluss::view::RowWindow row_window() const override;
  bool sortable() const noexcept override { return false; }
  double column_meter(std::int64_t column) const override;

 private:
  struct State;
  std::int64_t entry_at(const State& column, std::int64_t row) const;
  void classify_(State& column) const;

  std::string row_header_;
  std::vector<std::shared_ptr<State>> columns_{};
  std::int64_t longest_{0};
  std::int64_t shortest_{0};
  mutable std::atomic<bool> tail_{false};
  mutable std::atomic<bool> filtering_{false};
  mutable std::atomic<int> number_format_{0};
  mutable std::mutex mutex_{};
  mutable DisplayFilter filter_{DisplayFilter::None};
  mutable pluss::view::RowWindow window_{};
};

// The page a ragged window opens, the same for every ragged type: the grid, three cards (how many columns, the longest
// and the shortest), and the view tabs over `streams`. `parts` and `part_word` name the columns (keys/key, fields/
// field), `entries_word` their items (entries, states).
ModelPage ragged_model_page(const std::string& title, const std::string& parts, const std::string& part_word,
                            const std::string& entries_word, std::shared_ptr<RaggedGridModel> model,
                            std::vector<std::pair<std::string, std::shared_ptr<const Shelf>>> streams);

}  // namespace pluss::table::display

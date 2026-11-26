#pragma once

// A catalog laid out for the viewer as a ragged grid: its keys side by side, each column as long as its own stream.
// Nothing is padded to make the columns equal. Past the end of a stream a cell is absent (nothing is drawn); a
// real NULL inside a stream is still a NULL. It is a model for the one table renderer, so scrolling, resizing,
// themes, the filter and Visit work as they do for any table.

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "pluss/tils/view/html_window.h"
#include "kindle/catalog.h"
#include "table/display/Display.h"

namespace pluss::kindle::display {

class CatalogGridModel final : public pluss::view::VirtualTableModel {
 public:
  explicit CatalogGridModel(const Catalog& catalog);

  std::int64_t longest() const noexcept { return longest_; }
  std::int64_t shortest() const noexcept { return shortest_; }
  std::int64_t keys() const noexcept { return static_cast<std::int64_t>(columns_.size()); }

  // Where each stream sits in the shared rows: from the top (head) or ending at the bottom (tail).
  void set_alignment(pluss::table::display::DisplayAlign align) const;
  // Keeps the rows where some stream holds the kind of entry the filter looks for.
  void set_filter(pluss::table::display::DisplayFilter filter) const;
  void set_window(const pluss::view::RowWindow& window) const;
  // How numbers are written (View > Format); the stored values do not change.
  void set_number_format(pluss::table::display::NumberFormat format) const;

  std::int64_t row_count() const noexcept override { return longest_; }
  std::int64_t column_count() const noexcept override { return keys(); }
  std::string row_header() const override { return "entry"; }
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
  struct Column;
  // The entry of `column` shown at `row`, or -1 when the stream has none there.
  std::int64_t entry_at(const Column& column, std::int64_t row) const;
  void classify_(Column& column) const;

  std::vector<std::shared_ptr<Column>> columns_{};
  std::int64_t longest_{0};
  std::int64_t shortest_{0};
  mutable std::atomic<bool> tail_{false};
  mutable std::atomic<bool> filtering_{false};
  mutable std::atomic<int> number_format_{0};
  mutable std::mutex mutex_{};
  mutable pluss::table::display::DisplayFilter filter_{pluss::table::display::DisplayFilter::None};
  mutable pluss::view::RowWindow window_{};
};

}  // namespace pluss::kindle::display

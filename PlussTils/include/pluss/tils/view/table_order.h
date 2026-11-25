#pragma once

// How a long table (one painted from a VirtualTableModel) is put in order. Kept in one place so every long
// table sorts the same way, whatever window or page holds it.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "pluss/tils/view/html_window.h"

namespace pluss::view {

// Orders `rows` (indices into the model) by one column, stably. Columns are numbered as in a short table: 0 is the
// label column and n is the model's column n - 1. Numbers sort as numbers (NaN and infinity where their value
// puts them), text as text. NaN goes after every number and a missing cell after that, whichever way the column is
// sorted, so the order is always a strict one. When `rows` is empty and no filter is in force it means every row.
inline void sort_model_rows(const VirtualTableModel& model,
                            std::vector<std::int64_t>& rows,
                            const int sort_column,
                            const bool ascending) {
  if (rows.empty() && model.row_count() > 0 && !model.filtering()) {
    rows.resize(static_cast<std::size_t>(model.row_count()));
    std::iota(rows.begin(), rows.end(), std::int64_t{0});
  }
  const std::int64_t column = static_cast<std::int64_t>(sort_column) - 1;
  const bool numeric = column >= 0 && model.column_numeric(column);
  struct Key {
    double number{0.0};
    std::string text{};
    bool missing{false};
  };
  std::vector<Key> keys(rows.size());
  for (std::size_t i = 0; i < rows.size(); ++i) {
    if (column < 0) {
      keys[i].text = model.row_label(rows[i]);
      continue;
    }
    const auto cell = model.cell(rows[i], column);
    keys[i].missing = cell.missing;
    if (numeric) {
      keys[i].number = std::strtod(cell.text.c_str(), nullptr);
    } else {
      keys[i].text = cell.text;
    }
  }
  std::vector<std::size_t> order(rows.size());
  std::iota(order.begin(), order.end(), std::size_t{0});
  // Ordinary values come first, then NaN, then missing cells.
  const auto rank = [&](const std::size_t i) { return keys[i].missing ? 2 : (numeric && std::isnan(keys[i].number) ? 1 : 0); };
  const auto before = [&](const std::size_t a, const std::size_t b) {
    if (rank(a) != rank(b)) {
      return rank(a) < rank(b);
    }
    if (rank(a) != 0) {
      return false;
    }
    if (numeric) {
      return ascending ? keys[a].number < keys[b].number : keys[a].number > keys[b].number;
    }
    return ascending ? keys[a].text < keys[b].text : keys[a].text > keys[b].text;
  };
  std::stable_sort(order.begin(), order.end(), before);
  std::vector<std::int64_t> sorted(rows.size());
  for (std::size_t i = 0; i < order.size(); ++i) {
    sorted[i] = rows[order[i]];
  }
  rows = std::move(sorted);
}

}  // namespace pluss::view

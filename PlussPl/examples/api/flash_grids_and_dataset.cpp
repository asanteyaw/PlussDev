// JointGrid, pairgrid_map_plot, jointgrid_map_plot and Dataset: examples added to their reference pages ("### name n" markers).
#include <pluss/pl/plusspl.h>
#include <pluss/pluss.h>

#include <iostream>
#include <string>

using namespace pluss::pl;

int main() {
  const auto x = pluss::make_series({1.0, 2.0, 3.0});
  const auto y = pluss::make_series({1.0, 4.0, 9.0});

  std::cout << "### JointGrid 2\n";
  {
    auto fig = studio("grid_empty");
    flash::JointGrid(fig);
    std::cout << "panes after JointGrid: " << fig.pane_count() << "\n";
  }
  std::cout << "### JointGrid 3\n";
  {
    auto fig = studio("grid_fill");
    flash::JointGrid(fig, 20.0, 20.0);
    for (int i = 0; i < 4; ++i) fig.pane2("cell " + std::to_string(i)).trend(x, y);
    std::cout << "panes after filling the grid: " << fig.pane_count() << "\n";
  }
  std::cout << "### JointGrid 4\n";
  {
    auto a = studio("same");
    auto b = studio("same");
    flash::JointGrid(a, 20.0, 20.0);
    b.mosaic(2, 2, 20.0, 20.0);
    for (int i = 0; i < 4; ++i) { a.pane2("c").trend(x, y); b.pane2("c").trend(x, y); }
    std::cout << "JointGrid(fig, 20, 20) draws the same figure as mosaic(2, 2, 20, 20): " << (a.to_svg() == b.to_svg()) << "\n";
  }

  std::cout << "### pairgrid_map_plot 2\n";
  {
    auto fig = studio("pairs_count");
    int diagonal = 0, off = 0;
    flash::pairgrid_map_plot(fig, 3, [&](Pane &pane, std::int64_t, std::int64_t, bool is_diagonal) {
      (is_diagonal ? diagonal : off)++;
      pane.trend(x, y);
    });
    std::cout << "diagonal cells " << diagonal << ", off-diagonal cells " << off << ", panes " << fig.pane_count() << "\n";
  }
  std::cout << "### pairgrid_map_plot 3\n";
  {
    auto fig = studio("pairs_order");
    std::string order;
    flash::pairgrid_map_plot(fig, 2, [&](Pane &pane, std::int64_t row, std::int64_t col, bool) {
      order += "(" + std::to_string(row) + "," + std::to_string(col) + ") ";
      pane.trend(x, y);
    });
    std::cout << "cells visited in order: " << order << "\n";
  }
  std::cout << "### pairgrid_map_plot 4\n";
  {
    auto fig = studio("pairs_one");
    int calls = 0;
    flash::pairgrid_map_plot(fig, 1, [&](Pane &pane, std::int64_t, std::int64_t, bool is_diagonal) {
      ++calls;
      std::cout << "one-by-one grid: the only cell is on the diagonal: " << is_diagonal << "\n";
      pane.trend(x, y);
    });
    std::cout << "calls " << calls << ", panes " << fig.pane_count() << "\n";
  }

  std::cout << "### jointgrid_map_plot 2\n";
  {
    auto fig = studio("joint_calls");
    std::string log;
    flash::jointgrid_map_plot(
        fig, [&](Pane &pane) { log += "joint "; pane.cloud(x, y); },
        [&](Pane &pane, bool top) { log += top ? "top " : "right "; pane.trend(x, y); });
    std::cout << "calls in order: " << log << "\n";
  }
  std::cout << "### jointgrid_map_plot 3\n";
  {
    auto fig = studio("joint_panes");
    flash::jointgrid_map_plot(fig, [&](Pane &pane) { pane.cloud(x, y); }, [&](Pane &pane, bool) { pane.trend(x, y); });
    std::cout << "panes: " << fig.pane_count() << "\n";
  }

  std::cout << "### Dataset 2\n";
  {
    const auto tips = *flash::load_dataset("tips");
    std::cout << "numeric columns " << tips.numeric_names->numel() << ", text columns " << tips.text_names->numel() << ", rows " << tips.nrows() << "\n";
  }
  std::cout << "### Dataset 3\n";
  {
    const auto tips = *flash::load_dataset("tips");
    std::cout << "numeric(\"tip\") found: " << tips.numeric("tip").has_value() << ", numeric(\"day\") found: " << tips.numeric("day").has_value()
              << ", text(\"day\") found: " << tips.text("day").has_value() << ", numeric(\"nothing\") found: " << tips.numeric("nothing").has_value() << "\n";
  }
  std::cout << "### Dataset 4\n";
  {
    auto frame = pluss::table::DataFrame({{"a", pluss::make_series({1.0, 2.0})}, {"b", pluss::make_series({3.0, 4.0})}});
    const auto wrapped = flash::Dataset::from_table(frame);
    std::cout << "wrapped table: " << wrapped.nrows() << " rows, " << wrapped.ncols() << " columns, numeric names " << wrapped.numeric_names->numel() << "\n";
  }
}

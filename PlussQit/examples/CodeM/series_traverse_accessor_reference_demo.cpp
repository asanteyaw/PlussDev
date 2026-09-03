#include <pluss/pluss.h>

#include <exception>
#include <iostream>
#include <string>

namespace {

void heading(const char* label) { std::cout << "=== " << label << " ===\n"; }

void print_error(const std::exception& error) {
  const std::string message = error.what();
  std::cout << message.substr(0, message.find('\n')) << "\n";
}

void print_match(const pluss::codem::TraverseMatch& match) {
  std::cout << "count=" << match.count()
            << " rank=" << match.rank()
            << " indexed_rank=" << match.indexed_rank()
            << " has_values=" << std::boolalpha << match.has_values() << "\n";
  std::cout << "coords=" << match.coords() << "\n";
  std::cout << "values=" << match.values() << "\n";
  std::cout << "linear=" << match.linear_indices() << "\n";
}

} // namespace

int main() {
  auto x = pluss::make_series({
      {10.0, 11.0, 12.0, 13.0},
      {20.0, 21.0, 22.0, 23.0},
      {30.0, 31.0, 32.0, 33.0},
  });

  heading("scalar_comparisons");
  std::cout << "eq=" << x.traverse.find_eq(22.0).coords() << "\n";
  std::cout << "neq=" << x.traverse.find_neq(22.0).coords() << "\n";
  std::cout << "ge=" << x.traverse.find_ge(31.0).coords() << "\n";
  std::cout << "le=" << x.traverse.find_le(11.0).coords() << "\n\n";

  heading("series_comparisons_and_broadcasting");
  auto row_threshold = pluss::make_series({11.0, 21.0, 31.0}).unsqueeze(1);
  std::cout << "ge_row_threshold=" << x.traverse.find_ge(row_threshold).coords() << "\n";
  auto exact = x.clone();
  std::cout << "eq_exact_count=" << x.traverse().find_eq(exact).count() << "\n\n";

  heading("between_bounds");
  print_match(x.traverse.find_btw(21.0, 31.0));
  std::cout << "exclusive=" << x.traverse.find_btw(21.0, 31.0, false).values() << "\n";
  auto lo = pluss::make_series({10.0, 20.0, 30.0}).unsqueeze(1);
  auto hi = pluss::make_series({11.0, 22.0, 33.0}).unsqueeze(1);
  std::cout << "series_bounds=" << x.traverse.find_btw(lo, hi).coords() << "\n\n";

  heading("mask_queries");
  auto even = x.remainder(2.0).eq(0.0);
  auto high = x.ge(22.0);
  std::cout << "all=" << x.traverse.find_all(even).coords() << "\n";
  std::cout << "if_alias=" << x.traverse.find_if(even).coords() << "\n";
  std::cout << "and=" << x.traverse.find_and(even, high).coords() << "\n";
  std::cout << "or=" << x.traverse.find_or(x.eq(11.0), x.eq(32.0)).coords() << "\n";
  std::cout << "not=" << x.traverse.find_not(even).values() << "\n";
  auto numeric_mask = pluss::make_series({
      {0, 0, 1, 0},
      {0, -2, 0, 0},
      {0, 0, 0, 3},
  });
  std::cout << "numeric_mask=" << x.traverse.find_all(numeric_mask).coords() << "\n\n";

  heading("bound_selector_view_coordinates");
  auto tail = x.traverse({pluss::Slicing::SliceAll(), pluss::Slicing::SliceFrom(1)})
                  .find_btw(21.0, 32.0);
  print_match(tail);
  auto tail_view = x.traverse_slice({
      pluss::Slicing::SliceAll(),
      pluss::Slicing::SliceFrom(1),
  });
  std::cout << "selected_from_view=" << tail.select_from(tail_view) << "\n\n";

  heading("match_consumption");
  auto match = x.traverse.find_ge(22.0);
  const auto split = match.split_indices();
  std::cout << "row_indices=" << split[0] << "\n";
  std::cout << "column_indices=" << split[1] << "\n";
  auto labels = pluss::make_series({
      {100, 101, 102, 103},
      {200, 201, 202, 203},
      {300, 301, 302, 303},
  });
  std::cout << "selected_payload=" << match.select_from(labels) << "\n";
  auto vectors = pluss::make_series({
      {{1.0, 10.0}, {2.0, 20.0}, {3.0, 30.0}, {4.0, 40.0}},
      {{5.0, 50.0}, {6.0, 60.0}, {7.0, 70.0}, {8.0, 80.0}},
      {{9.0, 90.0}, {10.0, 100.0}, {11.0, 110.0}, {12.0, 120.0}},
  });
  std::cout << "selected_vectors=" << match.select_from(vectors) << "\n\n";

  heading("find_first_modes");
  auto first_mask = pluss::make_series({
      {false, true, true, false},
      {false, false, true, true},
      {false, true, false, true},
  });
  std::cout << "global=" << x.traverse.find_first(first_mask).coords() << "\n";
  std::cout << "per_row=" << x.traverse.find_first(first_mask, 1).coords() << "\n";
  std::cout << "per_column=" << x.traverse.find_first(first_mask, 0).coords() << "\n";
  std::cout << "per_batch=" << x.traverse.find_first(first_mask, -1, true).coords() << "\n\n";

  heading("empty_match");
  auto empty = x.traverse.find_eq(999.0);
  print_match(empty);
  std::cout << "selected_empty=" << empty.select_from(x) << "\n\n";

  heading("value_and_payload_gradients");
  auto tracked = x.clone();
  tracked.tracks_slope_(true);
  auto tracked_match = tracked.traverse.find_ge(31.0);
  tracked_match.values().sum().reverse();
  std::cout << "owner_gradient=" << tracked.sensitivity() << "\n";

  auto payload = x.clone();
  payload.tracks_slope_(true);
  match.select_from(payload).sum().reverse();
  std::cout << "payload_gradient=" << payload.sensitivity() << "\n\n";

  heading("errors");
  try {
    x.traverse.find_all(pluss::make_series({true, false}));
  } catch (const std::exception& error) {
    print_error(error);
  }
  try {
    x.traverse.find_first(first_mask, 1, true);
  } catch (const std::exception& error) {
    print_error(error);
  }
  try {
    match.select_from(pluss::make_series({1.0, 2.0, 3.0}));
  } catch (const std::exception& error) {
    print_error(error);
  }

  return 0;
}

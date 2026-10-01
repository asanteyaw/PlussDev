#include <pluss/pluss.h>

#include <cmath>
#include <iostream>
#include <string>

namespace {

void heading(const char* name) { std::cout << "=== " << name << " ===\n"; }

void error_line(const std::exception& error) {
  const std::string message = error.what();
  std::cout << message.substr(0, message.find('\n')) << "\n";
}

void print_strides(const pluss::Series& value) {
  std::cout << "[";
  for (std::size_t i = 0; i < value.strides().size(); ++i) {
    if (i != 0) std::cout << ",";
    std::cout << value.strides()[i];
  }
  std::cout << "]\n";
}

void print_parts(const std::vector<pluss::Series>& parts) {
  std::cout << "count=" << parts.size() << "\n";
  for (std::size_t i = 0; i < parts.size(); ++i) {
    std::cout << "part[" << i << "]=" << parts[i] << "\n";
  }
}

}  // namespace

int main() {
  auto table = pluss::make_series({
      {4.0, 1.0, 4.0, 2.0},
      {3.0, 5.0, 0.0, 5.0},
  });

  heading("sort_pair_default");
  auto ordered = table.sort_pair();
  std::cout << "values=" << ordered.first << "\nindices=" << ordered.second << "\n\n";

  heading("sort_pair_axis_stable_descending");
  auto columns = table.sort_pair(0, true, true);
  std::cout << "values=" << columns.first << "\nindices=" << columns.second << "\n\n";

  heading("sort_pair_reverse");
  auto sort_leaf = pluss::make_series({3.0, 1.0, 2.0});
  sort_leaf.tracks_slope_(true);
  auto sort_result = sort_leaf.sort_pair();
  (sort_result.first * pluss::make_series({10.0, 20.0, 30.0})).sum().reverse();
  std::cout << "indices=" << sort_result.second << "\ndx=" << sort_leaf.sensitivity() << "\n\n";

  heading("topk_pair_largest");
  auto largest = table.topk_pair(2, 1);
  std::cout << "values=" << largest.first << "\nindices=" << largest.second << "\n\n";

  heading("topk_pair_smallest");
  auto smallest = table.topk_pair(2, -1, false, true);
  std::cout << "values=" << smallest.first << "\nindices=" << smallest.second << "\n\n";

  heading("topk_pair_unsorted");
  auto unordered = pluss::make_series({9LL, 1LL, 7LL, 3LL, 5LL}).topk_pair(3, 0, true, false);
  std::cout << "values=" << unordered.first << "\nindices=" << unordered.second << "\n\n";

  heading("topk_pair_zero");
  auto none = table.topk_pair(0, 1);
  std::cout << "values=" << none.first << "\nindices=" << none.second << "\n\n";

  heading("topk_pair_bad_k");
  try {
    (void)table.topk_pair(5, 1);
  } catch (const std::exception& error) {
    error_line(error);
  }
  std::cout << "\n";

  heading("betaln_scalar");
  std::cout << pluss::make_series({1.0, 2.0, 3.0}).betaln(3.0) << "\n\n";

  heading("betaln_series_broadcast");
  auto a = pluss::make_series(std::vector<double>{1.0, 2.0}, {2, 1});
  auto b = pluss::make_series({2.0, 3.0, 4.0});
  std::cout << a.betaln(b) << "\n\n";

  heading("betaln_two_sided_reverse");
  auto left = pluss::make_series({2.0, 4.0});
  auto right = pluss::make_series({3.0, 5.0});
  left.tracks_slope_(true);
  right.tracks_slope_(true);
  left.betaln(right).sum().reverse();
  std::cout << "da=" << left.sensitivity() << "\ndb=" << right.sensitivity() << "\n\n";

  heading("platform_query");
  std::cout << "source=" << table.platform() << "\nview=" << table.transpose(0, 1).platform()
            << "\ncopy=" << table.clone().platform() << "\n\n";

  heading("strides_contiguous_and_transposed");
  std::cout << "table=";
  print_strides(table);
  std::cout << "transposed=";
  print_strides(table.transpose(0, 1));
  std::cout << "broadcast=";
  print_strides(pluss::make_series({1.0, 2.0, 3.0}).unsqueeze(0).expand({4, 3}));
  std::cout << "scalar=";
  print_strides(pluss::make_series(1.0));
  std::cout << "\n";

  auto sequence = pluss::make_series({0LL, 1LL, 2LL, 3LL, 4LL, 5LL});
  heading("split_with_sizes_basic");
  print_parts(sequence.split_with_sizes({2, 1, 3}));
  std::cout << "\n";

  heading("split_with_sizes_zero_piece");
  print_parts(sequence.split_with_sizes({2, 0, 4}));
  std::cout << "\n";

  heading("split_with_sizes_axis");
  print_parts(table.split_with_sizes({1, 3}, -1));
  std::cout << "\n";

  heading("split_with_sizes_reverse");
  auto split_leaf = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0});
  split_leaf.tracks_slope_(true);
  auto pieces = split_leaf.split_with_sizes({2, 1, 3});
  pieces[2].sum().reverse();
  std::cout << split_leaf.sensitivity() << "\n\n";

  heading("split_with_sizes_bad_sum");
  try {
    (void)sequence.split_with_sizes({2, 2});
  } catch (const std::exception& error) {
    error_line(error);
  }
  std::cout << "\n";

  heading("mT_matrix_view");
  auto matrix = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
  std::cout << matrix.mT() << "\n\n";

  heading("mT_batch");
  auto batch = pluss::make_series(
      std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {2, 2, 3});
  std::cout << batch.mT() << "\n\n";

  heading("mT_round_trip");
  auto round_trip = matrix.mT().mT();
  std::cout << "equal=" << std::boolalpha << round_trip.equal(matrix)
            << " strides=";
  print_strides(round_trip);
  std::cout << "\n";

  heading("mT_reverse");
  auto matrix_leaf = matrix.clone();
  matrix_leaf.tracks_slope_(true);
  (matrix_leaf.mT() * pluss::make_series({{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}}))
      .sum()
      .reverse();
  std::cout << matrix_leaf.sensitivity() << "\n\n";

  heading("mT_rank_error");
  try {
    (void)pluss::make_series({1.0, 2.0}).mT();
  } catch (const std::exception& error) {
    error_line(error);
  }
}

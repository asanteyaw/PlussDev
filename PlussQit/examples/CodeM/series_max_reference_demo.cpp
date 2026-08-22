#include <pluss/pluss.h>

#include <iostream>
#include <string>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

void print_error(const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

}  // namespace

int main() {
  auto scores = pluss::make_series({
      {1.0, 7.0, 3.0},
      {4.0, 7.0, 6.0},
  });

  heading("max_all");
  std::cout << scores.max() << "\n\n";

  heading("max_dim1");
  std::cout << scores.max(1) << "\n\n";

  heading("max_dim1_keepdim");
  std::cout << scores.max(1, true) << "\n\n";

  heading("max_negative_dim");
  std::cout << scores.max(-1) << "\n\n";

  auto floor_by_column = pluss::make_series({2.0, 6.0, 8.0});
  heading("max_other_series");
  std::cout << scores.max(floor_by_column) << "\n\n";

  heading("maximum_same_job");
  std::cout << scores.maximum(floor_by_column) << "\n\n";

  auto reduced_ties = pluss::make_series({
      {1.0, 5.0, 5.0},
      {2.0, -1.0, 2.0},
  });
  reduced_ties.tracks_slope_(true);
  auto reduced_winners = reduced_ties.max(1);
  reduced_winners.sum().reverse();
  heading("max_reduce_tie_gradient");
  std::cout << "y=" << reduced_winners << "\n";
  std::cout << "dx=" << reduced_ties.sensitivity() << "\n\n";

  auto global_ties = pluss::make_series({
      {7.0, 0.0, 0.0},
      {7.0, 7.0, 1.0},
  });
  global_ties.tracks_slope_(true);
  auto global_winner = global_ties.max();
  global_winner.reverse();
  heading("max_global_tie_gradient");
  std::cout << "y=" << global_winner << "\n";
  std::cout << "dx=" << global_ties.sensitivity() << "\n\n";

  auto lhs = pluss::make_series({1.0, 5.0, 2.0});
  auto rhs = pluss::make_series({4.0, 2.0, 2.0});
  lhs.tracks_slope_(true);
  rhs.tracks_slope_(true);
  auto binary = lhs.max(rhs);
  binary.sum().reverse();
  heading("max_binary_tie_gradient");
  std::cout << "y=" << binary << "\n";
  std::cout << "dlhs=" << lhs.sensitivity() << "\n";
  std::cout << "drhs=" << rhs.sensitivity() << "\n\n";

  heading("max_bad_dim_error");
  try {
    std::cout << scores.max(2) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  return 0;
}

#include <pluss/pluss.h>

#include <iostream>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

}  // namespace

int main() {
  auto dst = pluss::make_series({0.0, 0.0, 0.0}, {3});
  auto src = pluss::make_series({1.0, 2.0, 3.0}, {3});
  heading("copy_same_object");
  auto& same = dst.copy_(src);
  std::cout << "dst=" << dst << "\n";
  std::cout << "same_object=" << (&same == &dst ? "true" : "false") << "\n\n";

  pluss::Series w = pluss::Series::empty({5, 3}, pluss::ScalarType::Float64, pluss::pCPU);
  w.tracks_slope_(true);
  w.copy_(pluss::make_series({
      {1.0, 1.0, 1.0},
      {2.0, 2.0, 2.0},
      {3.0, 3.0, 3.0},
      {4.0, 4.0, 4.0},
      {5.0, 5.0, 5.0},
  }));
  w.clear_sensitivity();
  auto picked_rows = pluss::make_series({0, 2, 4}, {3});
  auto picked = w.index_select(0, picked_rows);
  picked.sum().reverse();
  heading("copy_leaf_parameter_gradient");
  std::cout << "picked=" << picked << "\n";
  std::cout << "dw=" << w.sensitivity() << "\n\n";

  auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4});
  x.tracks_slope_(true);
  auto replacement = pluss::make_series({10.0, 20.0}, {2});
  replacement.tracks_slope_(true);
  auto middle = x.slice(0, 1, 3, 1);
  middle.copy_(replacement);
  x.sum().reverse();
  heading("copy_view_redirect");
  std::cout << "x=" << x << "\n";
  std::cout << "dx=" << x.sensitivity() << "\n";
  std::cout << "dsrc=" << replacement.sensitivity() << "\n\n";

  auto table = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
  });
  auto gather_cols = pluss::make_series({
      {0, 2},
      {1, 0},
  });
  heading("gather_dim1_per_row");
  std::cout << table.gather(1, gather_cols) << "\n\n";

  auto gather_rows = pluss::make_series({
      {0, 1, 0},
      {1, 0, 1},
  });
  heading("gather_dim0_per_column");
  std::cout << table.gather(0, gather_rows) << "\n\n";

  auto gather_input = pluss::make_series({10.0, 20.0, 30.0}, {3});
  gather_input.tracks_slope_(true);
  auto repeated = pluss::make_series({0, 0, 2}, {3});
  auto gathered = gather_input.gather(0, repeated);
  gathered.sum().reverse();
  heading("gather_repeated_gradient");
  std::cout << "out=" << gathered << "\n";
  std::cout << "dx=" << gather_input.sensitivity() << "\n\n";

  auto weight = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
      {7.0, 8.0, 9.0},
      {10.0, 11.0, 12.0},
  });
  auto row_index = pluss::make_series({0, 2, 3}, {3});
  heading("index_select_rows");
  std::cout << weight.index_select(0, row_index) << "\n\n";

  auto tracked_weight = pluss::make_series({
      {1.0, 1.0, 1.0},
      {2.0, 2.0, 2.0},
      {3.0, 3.0, 3.0},
      {4.0, 4.0, 4.0},
      {5.0, 5.0, 5.0},
  });
  tracked_weight.tracks_slope_(true);
  auto lookup = pluss::make_series({0, 2, 4}, {3});
  auto embedded = tracked_weight.index_select(0, lookup);
  embedded.sum().reverse();
  heading("index_select_sparse_gradient");
  std::cout << "out=" << embedded << "\n";
  std::cout << "dw=" << tracked_weight.sensitivity() << "\n\n";

  auto repeated_lookup = pluss::make_series({0, 0, 3}, {3});
  heading("index_select_repeated_rows");
  std::cout << weight.index_select(0, repeated_lookup) << "\n\n";

  auto repeated_weight = weight.clone();
  repeated_weight.tracks_slope_(true);
  auto repeated_out = repeated_weight.index_select(0, repeated_lookup);
  repeated_out.sum().reverse();
  heading("index_select_repeated_gradient");
  std::cout << "out=" << repeated_out << "\n";
  std::cout << "dw=" << repeated_weight.sensitivity() << "\n\n";

  return 0;
}

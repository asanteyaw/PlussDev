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
  auto base_scatter = pluss::make_series({
      {0.0, 0.0, 0.0},
      {0.0, 0.0, 0.0},
  });
  auto scatter_idx = pluss::make_series({
      {2LL, 0LL, 1LL},
      {1LL, 2LL, 0LL},
  });
  auto scatter_src = pluss::make_series({
      {10.0, 20.0, 30.0},
      {40.0, 50.0, 60.0},
  });
  heading("scatter_dim1_basic");
  std::cout << base_scatter.scatter(1, scatter_idx, scatter_src) << "\n\n";

  auto scatter_rows_idx = pluss::make_series({
      {0LL, 1LL, 0LL},
      {1LL, 0LL, 1LL},
  });
  auto scatter_rows_src = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
  });
  heading("scatter_dim0_basic");
  std::cout << base_scatter.scatter(0, scatter_rows_idx, scatter_rows_src) << "\n\n";

  heading("scatter_negative_dim");
  std::cout << base_scatter.scatter(-1, scatter_idx, scatter_src) << "\n\n";

  auto broadcast_base = pluss::make_series({
      {0.0, 0.0, 0.0, 0.0},
      {0.0, 0.0, 0.0, 0.0},
  });
  auto broadcast_idx = pluss::make_series({
      {0LL, 1LL, 2LL, 3LL},
      {0LL, 1LL, 2LL, 3LL},
  });
  auto broadcast_src = pluss::make_series({{1.0, 2.0, 3.0, 4.0}});
  heading("scatter_broadcast_src_error");
  try {
    std::cout << pluss::series::print(broadcast_base.scatter(1, broadcast_idx, broadcast_src)) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  auto duplicate_base = pluss::make_series({10.0, 20.0, 30.0, 40.0});
  auto duplicate_idx = pluss::make_series({1LL, 1LL});
  auto duplicate_src = pluss::make_series({5.0, 6.0});
  heading("scatter_duplicate_overwrite");
  pluss::series::print(std::cout, duplicate_base.scatter(0, duplicate_idx, duplicate_src)) << "\n\n";

  auto grad_base = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  auto grad_src = pluss::make_series({5.0, 6.0});
  auto grad_idx = pluss::make_series({1LL, 3LL});
  grad_base.tracks_slope_(true);
  grad_src.tracks_slope_(true);
  auto grad_out = grad_base.scatter(0, grad_idx, grad_src);
  grad_out.sum().reverse();
  heading("scatter_gradient");
  std::cout << "out=" << grad_out << "\n";
  std::cout << "dbase=" << grad_base.sensitivity() << "\n";
  std::cout << "dsrc=" << grad_src.sensitivity() << "\n\n";

  auto scatter_inplace = pluss::make_series({10.0, 20.0, 30.0, 40.0});
  heading("scatter_inplace");
  scatter_inplace.scatter_(0, grad_idx, grad_src);
  std::cout << scatter_inplace << "\n\n";

  auto scatter_add_base = pluss::make_series({0.0, 0.0, 0.0, 0.0});
  auto scatter_add_idx = pluss::make_series({0LL, 2LL, 2LL, 3LL});
  auto scatter_add_src = pluss::make_series({0.1, 0.2, 0.3, 0.4});
  heading("scatter_add_basic");
  std::cout << scatter_add_base.scatter_add(0, scatter_add_idx, scatter_add_src) << "\n\n";

  auto scatter_add_keep = pluss::make_series({10.0, 20.0, 30.0, 40.0});
  auto scatter_add_keep_idx = pluss::make_series({1LL, 3LL});
  auto scatter_add_keep_src = pluss::make_series({7.0, 8.0});
  heading("scatter_add_keep_base");
  std::cout << scatter_add_keep.scatter_add(0, scatter_add_keep_idx, scatter_add_keep_src) << "\n\n";

  auto scatter_add_small_base = pluss::make_series({
      {0.0, 0.0, 0.0, 0.0, 0.0},
      {0.0, 0.0, 0.0, 0.0, 0.0},
  });
  auto scatter_add_small_idx = pluss::make_series({
      {0LL, 1LL, 2LL},
      {2LL, 1LL, 0LL},
  });
  auto scatter_add_small_src = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
  });
  heading("scatter_add_smaller_along_dim");
  std::cout << scatter_add_small_base.scatter_add(1, scatter_add_small_idx, scatter_add_small_src) << "\n\n";

  heading("scatter_add_broadcast_src");
  std::cout << broadcast_base.scatter_add(1, broadcast_idx, broadcast_src) << "\n\n";

  auto scatter_add_grad_base = pluss::make_series({0.0, 0.0, 0.0, 0.0});
  auto scatter_add_grad_src = pluss::make_series({2.0});
  auto scatter_add_grad_idx = pluss::make_series({0LL, 1LL, 1LL, 3LL});
  scatter_add_grad_base.tracks_slope_(true);
  scatter_add_grad_src.tracks_slope_(true);
  auto scatter_add_grad_out = scatter_add_grad_base.scatter_add(0, scatter_add_grad_idx, scatter_add_grad_src);
  scatter_add_grad_out.sum().reverse();
  heading("scatter_add_gradient");
  std::cout << "out=" << scatter_add_grad_out << "\n";
  std::cout << "dbase=" << scatter_add_grad_base.sensitivity() << "\n";
  std::cout << "dsrc=" << scatter_add_grad_src.sensitivity() << "\n\n";

  auto reduce_base = pluss::make_series({10.0, 20.0, 30.0, 40.0});
  auto reduce_idx = pluss::make_series({1LL, 1LL});
  auto reduce_src = pluss::make_series({1.0, 3.0});
  heading("scatter_reduce_sum_include_self");
  std::cout << reduce_base.scatter_reduce(0, reduce_idx, reduce_src, "sum", true) << "\n\n";

  heading("scatter_reduce_sum_exclude_self");
  std::cout << reduce_base.scatter_reduce(0, reduce_idx, reduce_src, "sum", false) << "\n\n";

  heading("scatter_reduce_mean");
  std::cout << reduce_base.scatter_reduce(0, reduce_idx, reduce_src, "mean", true) << "\n\n";

  auto reduce_initial_idx = pluss::make_series({0LL, 0LL, 1LL, 1LL});
  auto reduce_initial_src = pluss::make_series({10.0, 10.0, 10.0, 10.0});
  heading("scatter_reduce_initial");
  std::cout << reduce_base.scatter_reduce(0, reduce_initial_idx, reduce_initial_src, "sum", false, true, 5.0) << "\n\n";

  auto reduce_2d_base = pluss::make_series({
      {10.0, 20.0, 30.0},
      {40.0, 50.0, 60.0},
  });
  auto reduce_2d_idx = pluss::make_series({
      {2LL, 0LL},
      {1LL, 1LL},
  });
  auto reduce_2d_src = pluss::make_series({
      {1.0, 2.0},
      {3.0, 4.0},
  });
  heading("scatter_reduce_negative_dim");
  std::cout << reduce_2d_base.scatter_reduce(-1, reduce_2d_idx, reduce_2d_src, "sum", true) << "\n\n";

  auto reduce_broadcast_base = pluss::make_series({
      {0.0, 0.0, 0.0, 0.0},
      {0.0, 0.0, 0.0, 0.0},
  });
  auto reduce_broadcast_idx = pluss::make_series({
      {0LL, 1LL, 2LL, 3LL},
      {0LL, 1LL, 2LL, 3LL},
  });
  auto reduce_broadcast_src = pluss::make_series({{1.0, 2.0, 3.0, 4.0}});
  heading("scatter_reduce_broadcast_src");
  std::cout << reduce_broadcast_base.scatter_reduce(1, reduce_broadcast_idx, reduce_broadcast_src, "sum", false)
            << "\n\n";

  auto amax_base = pluss::make_series({2.0, 5.0, 5.0, 1.0});
  auto amax_src = pluss::make_series({5.0, 4.0});
  auto amax_idx = pluss::make_series({1LL, 3LL});
  amax_base.tracks_slope_(true);
  amax_src.tracks_slope_(true);
  auto amax_out = amax_base.scatter_reduce(0, amax_idx, amax_src, "amax", true);
  amax_out.sum().reverse();
  heading("scatter_reduce_amax_gradient");
  std::cout << "out=" << amax_out << "\n";
  std::cout << "dbase=" << amax_base.sensitivity() << "\n";
  std::cout << "dsrc=" << amax_src.sensitivity() << "\n\n";

  auto amin_base = pluss::make_series({1.0, 2.0});
  auto amin_src = pluss::make_series({-3.0, 0.5});
  auto amin_idx = pluss::make_series({0LL, 1LL});
  amin_base.tracks_slope_(true);
  amin_src.tracks_slope_(true);
  auto amin_out = amin_base.scatter_reduce(0, amin_idx, amin_src, "amin", false);
  amin_out.sum().reverse();
  heading("scatter_reduce_amin_gradient");
  std::cout << "out=" << amin_out << "\n";
  std::cout << "dbase=" << amin_base.sensitivity() << "\n";
  std::cout << "dsrc=" << amin_src.sensitivity() << "\n\n";

  heading("scatter_reduce_bad_reduce");
  try {
    std::cout << reduce_base.scatter_reduce(0, reduce_idx, reduce_src, "prod", true) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("scatter_reduce_mean_initial_error");
  try {
    std::cout << reduce_base.scatter_reduce(0, reduce_idx, reduce_src, "mean", false, true, 5.0) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  auto reduce_inplace = pluss::make_series({10.0, 20.0, 30.0, 40.0});
  heading("scatter_reduce_inplace");
  reduce_inplace.scatter_reduce_(0, reduce_idx, reduce_src, "sum", false, true, 5.0);
  std::cout << reduce_inplace << "\n\n";

  auto reduce_inplace_keep_self = pluss::make_series({10.0, 20.0, 30.0, 40.0});
  heading("scatter_reduce_inplace_include_self");
  reduce_inplace_keep_self.scatter_reduce_(0, reduce_idx, reduce_src, "sum", true);
  std::cout << reduce_inplace_keep_self << "\n\n";

  auto reduce_inplace_mean = pluss::make_series({10.0, 20.0, 30.0, 40.0});
  heading("scatter_reduce_inplace_mean");
  reduce_inplace_mean.scatter_reduce_(0, reduce_idx, reduce_src, "mean", true);
  std::cout << reduce_inplace_mean << "\n\n";

  auto reduce_sum_named = pluss::make_series({10.0, 20.0, 30.0, 40.0});
  heading("scatter_reduce_sum_named");
  std::cout << reduce_sum_named.scatter_reduce_sum(0, reduce_idx, reduce_src, false, 5.0) << "\n\n";

  auto reduce_sum_named_keep_self = pluss::make_series({10.0, 20.0, 30.0, 40.0});
  heading("scatter_reduce_sum_named_include_self");
  std::cout << reduce_sum_named_keep_self.scatter_reduce_sum(0, reduce_idx, reduce_src, true) << "\n\n";

  auto reduce_amax_named = pluss::make_series({10.0, 20.0, 30.0, 40.0});
  heading("scatter_reduce_amax_named");
  std::cout << reduce_amax_named.scatter_reduce_amax(0, reduce_idx, reduce_src, true) << "\n\n";

  auto reduce_amax_named_exclude = pluss::make_series({10.0, 20.0, 30.0, 40.0});
  heading("scatter_reduce_amax_named_exclude_self");
  std::cout << reduce_amax_named_exclude.scatter_reduce_amax(0, reduce_idx, reduce_src, false) << "\n\n";

  auto reduce_amin_named = pluss::make_series({10.0, 20.0, 30.0, 40.0});
  heading("scatter_reduce_amin_named");
  std::cout << reduce_amin_named.scatter_reduce_amin(0, reduce_idx, reduce_src, false) << "\n\n";

  auto reduce_amin_named_keep = pluss::make_series({10.0, 0.0, 30.0, 40.0});
  heading("scatter_reduce_amin_named_include_self");
  std::cout << reduce_amin_named_keep.scatter_reduce_amin(0, reduce_idx, reduce_src, true) << "\n\n";

  auto masked_base = pluss::make_series({10.0, 20.0, 30.0, 40.0});
  auto masked_mask = pluss::make_series({true, false, true, false}, pluss::ScalarType::Bool);
  auto masked_src = pluss::make_series({1.0, 2.0});
  heading("masked_scatter_basic");
  std::cout << masked_base.masked_scatter(masked_mask, masked_src) << "\n\n";

  auto masked_broadcast_base = pluss::make_series({
      {10.0, 20.0, 30.0},
      {40.0, 50.0, 60.0},
  });
  auto masked_broadcast_mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
  auto masked_broadcast_src = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  heading("masked_scatter_broadcast_mask");
  std::cout << masked_broadcast_base.masked_scatter(masked_broadcast_mask, masked_broadcast_src) << "\n\n";

  auto masked_inplace = masked_base.clone();
  heading("masked_scatter_inplace");
  masked_inplace.masked_scatter_(masked_mask, masked_src);
  std::cout << masked_inplace << "\n\n";

  heading("masked_scatter_bad_count");
  try {
    std::cout << masked_base.masked_scatter(masked_mask, pluss::make_series({9.0})) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  heading("masked_scatter_bad_mask");
  try {
    auto bad_mask = pluss::make_series({1LL, 0LL, 1LL, 0LL});
    std::cout << masked_base.masked_scatter(bad_mask, masked_src) << "\n";
  } catch (const std::exception& e) {
    print_error(e);
  }
  std::cout << "\n";

  return 0;
}

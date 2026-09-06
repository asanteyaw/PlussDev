#include <pluss/pluss.h>

#include <iostream>
#include <string>

namespace {

void heading(const char* name) {
  std::cout << "=== " << name << " ===\n";
}

void show(const pluss::Series& value) {
  pluss::series::print(std::cout, value) << "\n\n";
}

void show_error(const auto& operation) {
  try {
    operation();
  } catch (const std::exception& error) {
    const std::string message = error.what();
    std::cout << message.substr(0, message.find('\n')) << "\n\n";
  }
}

} // namespace

int main() {
  heading("index_add_rows_and_alpha");
  const auto base = pluss::make_series({{1.0, 2.0, 3.0},
                                        {4.0, 5.0, 6.0},
                                        {7.0, 8.0, 9.0}});
  const auto rows = pluss::make_series({2, 0, 2});
  const auto additions = pluss::make_series({{10.0, 20.0, 30.0},
                                             {40.0, 50.0, 60.0},
                                             {1.0, 2.0, 3.0}});
  show(base.index_add(0, rows, additions));
  show(base.index_add(0, rows, additions, 0.5));

  heading("index_add_reverse");
  auto differentiable_base = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}});
  auto differentiable_source = pluss::make_series({{10.0, 20.0}, {30.0, 40.0}});
  differentiable_base.tracks_slope_(true);
  differentiable_source.tracks_slope_(true);
  const auto weighted = differentiable_base.index_add(
      0, pluss::make_series({2, 0}), differentiable_source, 2.0);
  weighted.mul(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}})).sum().reverse();
  show(differentiable_base.sensitivity());
  show(differentiable_source.sensitivity());

  heading("index_copy_columns");
  const auto copy_base = pluss::make_series({{1, 2, 3}, {4, 5, 6}});
  const auto columns = pluss::make_series({2, 0});
  const auto replacements = pluss::make_series({{30, 10}, {60, 40}});
  show(copy_base.index_copy(1, columns, replacements));

  heading("index_copy_in_place");
  auto copied_in_place = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
  const void* copy_storage = copied_in_place.raw_data();
  copied_in_place.index_copy_(
      0, pluss::make_series({1}), pluss::make_series({{30.0, 40.0}}));
  show(copied_in_place);
  std::cout << "same storage: " << std::boolalpha
            << (copy_storage == copied_in_place.raw_data()) << "\n\n";

  heading("index_fill_negative_dim");
  show(copy_base.index_fill(-1, pluss::make_series({0, 2}), -1.0));

  heading("index_fill_row");
  show(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}})
           .index_fill(0, pluss::make_series({1}), 0.0));

  heading("index_reduce_include_self");
  const auto reduction_base = pluss::make_series({10.0, 20.0, 30.0, 40.0});
  const auto reduction_index = pluss::make_series({0, 0, 2});
  const auto reduction_source = pluss::make_series({2.0, 3.0, 5.0});
  show(reduction_base.index_reduce(0, reduction_index, reduction_source, "prod", true));
  show(reduction_base.index_reduce(0, reduction_index, reduction_source, "prod", false));
  show(reduction_base.index_reduce(0, reduction_index, reduction_source, "mean", true));
  show(reduction_base.index_reduce(0, reduction_index, reduction_source, "mean", false));
  show(reduction_base.index_reduce(0, reduction_index, reduction_source, "amax", false));
  show(reduction_base.index_reduce(0, reduction_index, reduction_source, "amin", true));

  heading("index_reduce_product_reverse");
  auto product_base = pluss::make_series({2.0, 3.0});
  auto product_source = pluss::make_series({4.0, 5.0});
  product_base.tracks_slope_(true);
  product_source.tracks_slope_(true);
  product_base.index_reduce(0, pluss::make_series({0, 0}), product_source, "prod").sum().reverse();
  show(product_base.sensitivity());
  show(product_source.sensitivity());

  heading("masked_fill_broadcast");
  const auto mask = pluss::make_series({{true, false, true}, {false, true, false}});
  show(pluss::make_series({{1.0, 2.0, 3.0}}).masked_fill(mask, 9.0));

  heading("masked_fill_reverse");
  auto mask_values = pluss::make_series({{1.0, 2.0, 3.0}});
  auto fill = pluss::make_series(9.0);
  mask_values.tracks_slope_(true);
  fill.tracks_slope_(true);
  mask_values.masked_fill(mask, fill).sum().reverse();
  show(mask_values.sensitivity());
  show(fill.sensitivity());

  heading("masked_fill_in_place_failure");
  show_error([&] {
    auto values = pluss::make_series({{1.0, 2.0, 3.0}});
    values.masked_fill_(mask, 9.0);
  });

  heading("slice_scatter_step");
  const auto slice_base = pluss::make_series({{0, 1, 2, 3, 4, 5},
                                              {6, 7, 8, 9, 10, 11}});
  const auto slice_source = pluss::make_series({{100, 200}, {300, 400}});
  show(slice_base.slice_scatter(slice_source, 1, 1, 5, 2));

  heading("slice_scatter_tail");
  show(slice_base.slice_scatter(pluss::make_series({{90, 91}, {92, 93}}), -1, 4, 6));

  heading("select_scatter_negative_index");
  show(copy_base.select_scatter(pluss::make_series({30, 60}), 1, -1));

  heading("select_scatter_reverse");
  auto select_base = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
  auto select_source = pluss::make_series({30.0, 60.0});
  select_base.tracks_slope_(true);
  select_source.tracks_slope_(true);
  select_base.select_scatter(select_source, 1, 1).sum().reverse();
  show(select_base.sensitivity());
  show(select_source.sensitivity());

  heading("scatter_reverse");
  auto scatter_base = pluss::make_series({{0.0, 1.0, 2.0, 3.0}, {4.0, 5.0, 6.0, 7.0}});
  auto scatter_source = pluss::make_series({{10.0, 20.0}, {30.0, 40.0}});
  scatter_base.tracks_slope_(true);
  scatter_source.tracks_slope_(true);
  scatter_base.slice_scatter(scatter_source, 1, 1, 3).sum().reverse();
  show(scatter_base.sensitivity());
  show(scatter_source.sensitivity());

  heading("in_place_storage");
  auto mutable_values = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
  const void* storage = mutable_values.raw_data();
  mutable_values.index_add_(0, pluss::make_series({1}), pluss::make_series({{10.0, 20.0}}));
  show(mutable_values);
  std::cout << "same storage: " << std::boolalpha
            << (storage == mutable_values.raw_data()) << "\n\n";

  heading("contract_failures");
  show_error([&] {
    base.index_reduce(0, rows, additions, "sum");
  });
  show_error([&] {
    copy_base.select_scatter(pluss::make_series({30, 60}).reshape({2, 1}), 1, 0);
  });

  return 0;
}

#include <pluss/pluss.h>

#include <iostream>
#include <string>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

void print_error_prefix(const std::exception& e) {
  std::string msg = e.what();
  const auto nl = msg.find('\n');
  std::cout << msg.substr(0, nl);
}

} // namespace

int main() {
  auto square = pluss::make_series({
      {1, 2, 3},
      {4, 5, 6},
      {7, 8, 9},
  }, pluss::ScalarType::Int64);

  auto rectangular = pluss::make_series({
      {1, 2, 3},
      {4, 5, 6},
  }, pluss::ScalarType::Int64);

  heading("trace_square");
  std::cout << square.trace() << "\n\n";

  heading("trace_rectangular");
  pluss::series::print(std::cout, rectangular.trace()) << "\n\n";

  heading("trace_rank1_error");
  try {
    std::cout << pluss::make_series({1, 2, 3}, pluss::ScalarType::Int64).trace() << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n\n";

  heading("trace_gradient");
  auto trace_grad = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
      {7.0, 8.0, 9.0},
  });
  trace_grad.tracks_slope_(true);
  trace_grad.trace().reverse();
  std::cout << trace_grad.sensitivity() << "\n\n";

  heading("diag_vector_default");
  std::cout << pluss::make_series({9, 8, 7}, pluss::ScalarType::Int64).diag() << "\n\n";

  heading("diag_vector_offset");
  std::cout << pluss::make_series({9, 8, 7}, pluss::ScalarType::Int64).diag(1) << "\n\n";

  heading("diag_matrix_default");
  pluss::series::print(std::cout, square.diag()) << "\n\n";

  heading("diag_matrix_negative_offset");
  std::cout << square.diag(-1) << "\n\n";

  heading("diag_rank3_error");
  try {
    std::cout << pluss::make_series({
        {
            {1, 2},
            {3, 4},
        },
        {
            {5, 6},
            {7, 8},
        },
    }, pluss::ScalarType::Int64).diag() << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n\n";

  heading("diag_extract_gradient");
  auto diag_grad = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
      {7.0, 8.0, 9.0},
  });
  diag_grad.tracks_slope_(true);
  diag_grad.diag().sum().reverse();
  pluss::series::print(std::cout, diag_grad.sensitivity()) << "\n\n";

  heading("diagonal_default");
  std::cout << square.diagonal() << "\n\n";

  heading("diagonal_offset");
  std::cout << rectangular.diagonal(1) << "\n\n";

  heading("diagonal_swapped_dims");
  std::cout << rectangular.diagonal(0, 1, 0) << "\n\n";

  heading("diagonal_same_dim_error");
  try {
    std::cout << square.diagonal(0, 0, 0) << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n\n";

  heading("diagflat_from_matrix");
  std::cout << pluss::make_series({
      {1, 2},
      {3, 4},
  }, pluss::ScalarType::Int64).diagflat() << "\n\n";

  heading("diagflat_offset");
  pluss::series::print(std::cout, pluss::make_series({5, 6}, pluss::ScalarType::Int64).diagflat(-1)) << "\n\n";

  heading("diag_embed_vector");
  std::cout << pluss::make_series({4, 5, 6}, pluss::ScalarType::Int64).diag_embed() << "\n\n";

  heading("diag_embed_from_matrix_flattens");
  std::cout << pluss::make_series({
      {1, 2},
      {3, 4},
  }, pluss::ScalarType::Int64).diag_embed(1) << "\n\n";

  heading("diag_embed_gradient");
  auto diag_embed_grad = pluss::make_series({1.0, 2.0, 3.0});
  diag_embed_grad.tracks_slope_(true);
  diag_embed_grad.diag_embed().sum().reverse();
  std::cout << diag_embed_grad.sensitivity() << "\n\n";

  heading("diagonal_scatter_basic");
  std::cout << square.diagonal_scatter(
      pluss::make_series({90, 80, 70}, pluss::ScalarType::Int64)) << "\n\n";

  heading("diagonal_scatter_offset_truncates");
  std::cout << rectangular.diagonal_scatter(
      pluss::make_series({50, 60, 70, 80}, pluss::ScalarType::Int64), 1) << "\n\n";

  heading("diagonal_scatter_rank_error");
  try {
    std::cout << pluss::make_series({1, 2, 3}, pluss::ScalarType::Int64).diagonal_scatter(
        pluss::make_series({9}, pluss::ScalarType::Int64)) << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n\n";

  heading("tril_basic");
  std::cout << square.tril() << "\n\n";

  heading("tril_positive_diagonal");
  std::cout << square.tril(1) << "\n\n";

  heading("tril_batched");
  std::cout << pluss::make_series({
      {
          {1, 2, 3},
          {4, 5, 6},
      },
      {
          {7, 8, 9},
          {10, 11, 12},
      },
  }, pluss::ScalarType::Int64).tril() << "\n\n";

  heading("triu_basic");
  std::cout << square.triu() << "\n\n";

  heading("triu_negative_diagonal");
  std::cout << square.triu(-1) << "\n\n";

  heading("triu_rank1_error");
  try {
    std::cout << pluss::make_series({1, 2, 3}, pluss::ScalarType::Int64).triu() << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n\n";

  heading("tril_gradient_mask");
  auto tril_grad = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
      {7.0, 8.0, 9.0},
  });
  tril_grad.tracks_slope_(true);
  tril_grad.tril().sum().reverse();
  std::cout << tril_grad.sensitivity() << "\n\n";

  heading("tril_inplace");
  auto tril_inplace = square.clone();
  std::cout << "before=" << tril_inplace.storage_version() << "\n";
  tril_inplace.tril_(1);
  std::cout << "after=" << tril_inplace.storage_version() << "\n";
  std::cout << tril_inplace << "\n\n";

  heading("triu_inplace");
  auto triu_inplace = square.clone();
  pluss::series::print(std::cout, triu_inplace.triu_(-1)) << "\n\n";

  return 0;
}

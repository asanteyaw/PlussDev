#include <pluss/pluss.h>

#include <complex>
#include <iostream>
#include <string>
#include <vector>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

void print_error_prefix(const std::exception& e) {
  const std::string message = e.what();
  const auto newline = message.find('\n');
  std::cout << message.substr(0, newline);
}

} // namespace

int main() {
  heading("matmul_matrix_matrix");
  auto a = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
  });
  auto b = pluss::make_series({
      {7.0, 8.0},
      {9.0, 10.0},
      {11.0, 12.0},
  });
  std::cout << a.linearalg.matmul(b) << "\n\n";

  heading("matmul_rank_rules");
  auto vector = pluss::make_series({1.0, 2.0, 3.0});
  auto matrix = pluss::make_series({
      {1.0, 2.0},
      {3.0, 4.0},
      {5.0, 6.0},
  });
  std::cout << "vector_vector=" << vector.linearalg(pluss::make_series({4.0, 5.0, 6.0})).matmul() << "\n";
  std::cout << "vector_matrix=" << vector.linearalg.matmul(matrix) << "\n";
  std::cout << "matrix_vector=" << matrix.transpose(0, 1).linearalg.matmul(vector) << "\n\n";

  heading("matmul_batched_broadcast");
  auto batch_left = pluss::make_series({
      {
          {1.0, 2.0},
          {3.0, 4.0},
      },
  });
  auto batch_right = pluss::make_series({
      {
          {1.0, 0.0},
          {0.0, 1.0},
      },
      {
          {2.0, 0.0},
          {0.0, 3.0},
      },
  });
  std::cout << batch_left.linearalg.matmul(batch_right) << "\n\n";

  heading("matmul_gradient");
  auto grad_left = pluss::make_series({
      {1.0, 2.0},
      {3.0, 4.0},
  });
  auto grad_right = pluss::make_series({
      {5.0, 6.0},
      {7.0, 8.0},
  });
  grad_left.tracks_slope_(true);
  grad_right.tracks_slope_(true);
  grad_left.linearalg.matmul(grad_right).sum().reverse();
  std::cout << "dL_dleft=" << grad_left.sensitivity() << "\n";
  std::cout << "dL_dright=" << grad_right.sensitivity() << "\n\n";

  heading("matmul_shape_error");
  try {
    std::cout << pluss::make_series({{1.0, 2.0}}).linearalg.matmul(
        pluss::make_series({{1.0, 2.0}})) << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n\n";

  heading("matmul_dtype_error");
  try {
    auto floats = pluss::make_series({{1.0, 2.0}});
    auto integers = pluss::make_series({{1, 2}, {3, 4}}, pluss::ScalarType::Int64);
    std::cout << floats.linearalg.matmul(integers) << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n\n";

  heading("mm_call_styles");
  auto mm_left = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
  auto mm_right = pluss::make_series({{2.0, 0.0}, {1.0, 2.0}});
  std::cout << "peer_argument=" << mm_left.linearalg.mm(mm_right) << "\n";
  std::cout << "peer_bound=" << mm_left.linearalg(mm_right).mm() << "\n\n";

  heading("mm_rectangular");
  auto mm_rect_left = pluss::make_series({
      {1.0, 0.0, 2.0},
      {-1.0, 3.0, 1.0},
  });
  auto mm_rect_right = pluss::make_series({
      {3.0, 1.0},
      {1.0, 2.0},
      {2.0, 0.0},
  });
  std::cout << mm_rect_left.linearalg.mm(mm_rect_right) << "\n\n";

  heading("mm_gradient");
  auto mm_grad_left = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
  auto mm_grad_right = pluss::make_series({{5.0, 6.0}, {7.0, 8.0}});
  mm_grad_left.tracks_slope_(true);
  mm_grad_right.tracks_slope_(true);
  mm_grad_left.linearalg.mm(mm_grad_right).sum().reverse();
  std::cout << "dL_dleft=" << mm_grad_left.sensitivity() << "\n";
  std::cout << "dL_dright=" << mm_grad_right.sensitivity() << "\n\n";

  heading("mm_rank_error");
  try {
    std::cout << mm_left.linearalg.mm(pluss::make_series({1.0, 2.0})) << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n\n";

  heading("mv_matrix_vector");
  auto mv_matrix = pluss::make_series({
      {2.0, 0.0, -1.0},
      {1.0, 3.0, 2.0},
  });
  auto mv_vector = pluss::make_series({4.0, 5.0, 6.0});
  std::cout << mv_matrix.linearalg.mv(mv_vector) << "\n\n";

  heading("mv_basis_vectors");
  auto basis_matrix = pluss::make_series({
      {2.0, -1.0, 4.0},
      {0.0, 3.0, 5.0},
  });
  std::cout << "first_column=" << basis_matrix.linearalg.mv(
      pluss::make_series({1.0, 0.0, 0.0})) << "\n";
  std::cout << "third_column=" << basis_matrix.linearalg.mv(
      pluss::make_series({0.0, 0.0, 1.0})) << "\n\n";

  heading("mv_gradient");
  auto mv_grad_matrix = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
  auto mv_grad_vector = pluss::make_series({5.0, 6.0});
  mv_grad_matrix.tracks_slope_(true);
  mv_grad_vector.tracks_slope_(true);
  mv_grad_matrix.linearalg.mv(mv_grad_vector).sum().reverse();
  std::cout << "dL_dmatrix=" << mv_grad_matrix.sensitivity() << "\n";
  std::cout << "dL_dvector=" << mv_grad_vector.sensitivity() << "\n\n";

  heading("mv_rank_error");
  try {
    std::cout << pluss::make_series({1.0, 2.0}).linearalg.mv(
        pluss::make_series({3.0, 4.0})) << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n\n";

  heading("dot_vectors");
  auto dot_left = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  auto dot_right = pluss::make_series({10.0, 20.0, 30.0, 40.0});
  std::cout << dot_left.linearalg.dot(dot_right) << "\n\n";

  heading("dot_gradient");
  auto dot_x = pluss::make_series({1.0, 2.0, 3.0});
  auto dot_y = pluss::make_series({4.0, 5.0, 6.0});
  dot_x.tracks_slope_(true);
  dot_y.tracks_slope_(true);
  dot_x.linearalg.dot(dot_y).reverse();
  std::cout << "dL_dx=" << dot_x.sensitivity() << "\n";
  std::cout << "dL_dy=" << dot_y.sensitivity() << "\n\n";

  heading("dot_scaled_gradient");
  auto scaled_x = pluss::make_series({1.0, -2.0, 3.0});
  auto scaled_y = pluss::make_series({4.0, 5.0, -1.0});
  scaled_x.tracks_slope_(true);
  scaled_y.tracks_slope_(true);
  scaled_x.linearalg.dot(scaled_y).mul(2.5).reverse();
  std::cout << "value=" << scaled_x.linearalg.dot(scaled_y) << "\n";
  std::cout << "dL_dx=" << scaled_x.sensitivity() << "\n";
  std::cout << "dL_dy=" << scaled_y.sensitivity() << "\n\n";

  heading("dot_orthogonal");
  std::cout << pluss::make_series({2.0, 1.0}).linearalg.dot(
      pluss::make_series({-1.0, 2.0})) << "\n\n";

  heading("dot_rank_error");
  try {
    std::cout << pluss::make_series({{1.0, 2.0}}).linearalg.dot(
        pluss::make_series({{3.0, 4.0}})) << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n\n";

  heading("dot_length_error");
  try {
    std::cout << pluss::make_series({1.0, 2.0}).linearalg.dot(
        pluss::make_series({3.0, 4.0, 5.0})) << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n\n";

  heading("vdot_real");
  std::cout << pluss::make_series({1.0, 2.0, 3.0}).linearalg.vdot(
      pluss::make_series({4.0, 5.0, 6.0})) << "\n\n";

  heading("vdot_complex_conjugates_left");
  using Complex = std::complex<double>;
  auto complex_left = pluss::make_series(
      std::vector<Complex>{{1.0, 2.0}, {3.0, -1.0}}, {2});
  auto complex_right = pluss::make_series(
      std::vector<Complex>{{2.0, -1.0}, {4.0, 2.0}}, {2});
  std::cout << "dot=" << complex_left.linearalg.dot(complex_right) << "\n";
  std::cout << "vdot=" << complex_left.linearalg.vdot(complex_right) << "\n";
  std::cout << "reverse_vdot=" << complex_right.linearalg.vdot(complex_left) << "\n\n";

  heading("vdot_self");
  std::cout << complex_left.linearalg.vdot(complex_left) << "\n\n";

  heading("vdot_rank_error");
  try {
    std::cout << pluss::make_series({{1.0, 2.0}}).linearalg.vdot(
        pluss::make_series({{3.0, 4.0}})) << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n\n";

  heading("vdot_length_error");
  try {
    std::cout << pluss::make_series({1.0}).linearalg.vdot(
        pluss::make_series({2.0, 3.0})) << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n";

  return 0;
}

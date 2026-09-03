#include <pluss/pluss.h>

#include <exception>
#include <iostream>
#include <string>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

void print_error_prefix(const std::exception& e) {
  const std::string message = e.what();
  const auto newline = message.find('\n');
  std::cout << message.substr(0, newline) << "\n";
}

} // namespace

int main() {
  heading("norm_values_and_gradient");
  auto vector = pluss::make_series({3.0, 4.0});
  std::cout << "vector=" << vector.linearalg.norm() << "\n";
  std::cout << "matrix=" << pluss::make_series({{1.0, 2.0}, {2.0, 1.0}}).linearalg.norm() << "\n";
  vector.tracks_slope_(true);
  vector.linearalg.norm().reverse();
  std::cout << "gradient=" << vector.sensitivity() << "\n\n";

  heading("cholesky_lower_upper_and_batch");
  auto positive = pluss::make_series({{4.0, 2.0}, {2.0, 3.0}});
  auto lower = positive.linearalg.cholesky();
  auto upper = positive.linearalg.cholesky(true);
  std::cout << "lower=" << lower << "\n";
  std::cout << "lower_reconstruction=" << lower.linearalg.mm(lower.transpose(0, 1)) << "\n";
  std::cout << "upper=" << upper << "\n";
  std::cout << "upper_reconstruction=" << upper.transpose(0, 1).linearalg.mm(upper) << "\n";
  auto positive_batch = pluss::make_series({
      {{4.0, 2.0}, {2.0, 3.0}},
      {{9.0, 0.0}, {0.0, 4.0}},
  });
  std::cout << "batch=" << positive_batch.linearalg.cholesky() << "\n\n";

  heading("cholesky_gradient_and_failure");
  auto chol_input = pluss::make_series({{4.0, 2.0}, {2.0, 3.0}});
  chol_input.tracks_slope_(true);
  chol_input.linearalg.cholesky().sum().reverse();
  std::cout << "gradient=" << chol_input.sensitivity() << "\n";
  try {
    std::cout << pluss::make_series({{1.0, 2.0}, {2.0, 1.0}}).linearalg.cholesky() << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n";

  heading("qr_reduced_full_and_reconstruction");
  auto tall = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}});
  auto q_reduced = tall.linearalg.qr_q();
  auto r_reduced = tall.linearalg.qr_r();
  std::cout << "q_reduced=" << q_reduced << "\n";
  std::cout << "r_reduced=" << r_reduced << "\n";
  std::cout << "reconstruction=" << q_reduced.linearalg.mm(r_reduced) << "\n";
  std::cout << "orthogonality=" << q_reduced.transpose(0, 1).linearalg.mm(q_reduced) << "\n";
  std::cout << "q_full=" << tall.linearalg.qr_q(false) << "\n";
  std::cout << "r_full=" << tall.linearalg.qr_r(false) << "\n\n";

  heading("qr_batch_and_gradient");
  auto qr_batch = pluss::make_series({
      {{1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}},
      {{2.0, 0.0}, {0.0, 1.0}, {0.0, 1.0}},
  });
  std::cout << "q_batch=" << qr_batch.linearalg.qr_q() << "\n";
  auto qr_input = pluss::make_series({{2.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}});
  qr_input.tracks_slope_(true);
  qr_input.linearalg.qr_r().sum().reverse();
  std::cout << "r_gradient=" << qr_input.sensitivity() << "\n\n";

  heading("svd_joint_and_component_shapes");
  auto rectangular = pluss::make_series({{3.0, 0.0}, {0.0, 2.0}, {0.0, 0.0}});
  auto reduced_svd = rectangular.linearalg.svd(false);
  std::cout << "u_reduced=" << reduced_svd.u << "\n";
  std::cout << "s=" << reduced_svd.s << "\n";
  std::cout << "vh_reduced=" << reduced_svd.vh << "\n";
  std::cout << "u_full=" << rectangular.linearalg.svd_u() << "\n";
  std::cout << "vh_full=" << rectangular.linearalg.svd_vh() << "\n\n";

  heading("svd_call_styles_and_reconstruction");
  auto coupled = pluss::make_series({{3.0, 1.0}, {1.0, 3.0}});
  auto coupled_parts = coupled.linearalg.svd(false);
  auto coupled_scale = coupled_parts.s.diag_embed();
  std::cout << "accessor_s=" << coupled.linearalg.svd_s(false) << "\n";
  std::cout << "proxy_s=" << coupled.linearalg().svd_s(false) << "\n";
  std::cout << "u=" << coupled_parts.u << "\n";
  std::cout << "vh=" << coupled_parts.vh << "\n";
  std::cout << "reconstruction="
            << coupled_parts.u.linearalg.mm(coupled_scale).linearalg.mm(coupled_parts.vh)
            << "\n";
  std::cout << "u_orthogonality="
            << coupled_parts.u.transpose(0, 1).linearalg.mm(coupled_parts.u) << "\n";
  std::cout << "vh_orthogonality="
            << coupled_parts.vh.linearalg.mm(coupled_parts.vh.transpose(0, 1)) << "\n\n";

  heading("svd_wide_full_reduced_and_rank_deficient");
  auto wide_svd = pluss::make_series({{3.0, 0.0, 0.0}, {0.0, 2.0, 0.0}});
  std::cout << "wide_u_reduced=" << wide_svd.linearalg.svd_u(false) << "\n";
  std::cout << "wide_vh_reduced=" << wide_svd.linearalg.svd_vh(false) << "\n";
  std::cout << "wide_vh_full=" << wide_svd.linearalg.svd_vh(true) << "\n";
  auto rank_deficient = pluss::make_series({{1.0, 2.0}, {2.0, 4.0}});
  std::cout << "rank_deficient_s=" << rank_deficient.linearalg.svd_s(false) << "\n\n";

  heading("svd_batch_and_gradient");
  auto svd_batch = pluss::make_series({
      {{3.0, 0.0}, {0.0, 2.0}, {0.0, 0.0}},
      {{4.0, 0.0}, {0.0, 1.0}, {0.0, 0.0}},
  });
  std::cout << "singular_values=" << svd_batch.linearalg.svd_s(false) << "\n";
  auto svd_input = pluss::make_series({{3.0, 0.0}, {0.0, 2.0}, {0.0, 0.0}});
  svd_input.tracks_slope_(true);
  svd_input.linearalg.svd_s(false).sum().reverse();
  std::cout << "s_gradient=" << svd_input.sensitivity() << "\n\n";

  heading("svd_vector_gradients");
  auto u_input = pluss::make_series({{3.0, 1.0}, {0.0, 2.0}});
  u_input.tracks_slope_(true);
  u_input.linearalg.svd_u(false).sum().reverse();
  std::cout << "u_gradient=" << u_input.sensitivity() << "\n";
  auto vh_input = pluss::make_series({{3.0, 1.0}, {0.0, 2.0}});
  vh_input.tracks_slope_(true);
  vh_input.linearalg.svd_vh(false).sum().reverse();
  std::cout << "vh_gradient=" << vh_input.sensitivity() << "\n\n";

  heading("triangular_solve_modes");
  auto lower_tri = pluss::make_series({{2.0, 0.0}, {1.0, 3.0}});
  auto upper_tri = pluss::make_series({{2.0, 1.0}, {0.0, 3.0}});
  auto rhs_vector = pluss::make_series({2.0, 7.0});
  auto rhs_matrix = pluss::make_series({{2.0, 4.0}, {7.0, 10.0}});
  std::cout << "lower_vector=" << lower_tri.linearalg.triangular_solve(rhs_vector, false) << "\n";
  std::cout << "upper_vector=" << upper_tri.linearalg.triangular_solve(rhs_vector) << "\n";
  std::cout << "lower_matrix=" << lower_tri.linearalg.triangular_solve(rhs_matrix, false) << "\n";
  auto unit_lower = pluss::make_series({{99.0, 0.0}, {2.0, -7.0}});
  std::cout << "unit_lower=" << unit_lower.linearalg.triangular_solve(pluss::make_series({3.0, 8.0}), false, true) << "\n\n";

  heading("triangular_solve_broadcast_and_gradients");
  auto tri_batch = pluss::make_series({
      {{2.0, 0.0}, {1.0, 3.0}},
      {{1.0, 0.0}, {2.0, 2.0}},
  });
  std::cout << "broadcast=" << tri_batch.linearalg.triangular_solve(rhs_vector, false) << "\n";
  auto tri_grad = pluss::make_series({{2.0, 0.0}, {1.0, 3.0}});
  auto tri_rhs_grad = pluss::make_series({2.0, 7.0});
  tri_grad.tracks_slope_(true);
  tri_rhs_grad.tracks_slope_(true);
  tri_grad.linearalg.triangular_solve(tri_rhs_grad, false).sum().reverse();
  std::cout << "factor_gradient=" << tri_grad.sensitivity() << "\n";
  std::cout << "rhs_gradient=" << tri_rhs_grad.sensitivity() << "\n\n";

  heading("cholesky_solve_and_inverse");
  auto chol = positive.linearalg.cholesky();
  auto system_rhs = pluss::make_series({2.0, 1.0});
  std::cout << "solve=" << chol.linearalg.cholesky_solve(system_rhs) << "\n";
  std::cout << "inverse=" << chol.linearalg.cholesky_inverse() << "\n";
  std::cout << "identity=" << positive.linearalg.mm(chol.linearalg.cholesky_inverse()) << "\n";
  std::cout << "upper_solve=" << upper.linearalg.cholesky_solve(system_rhs, true) << "\n\n";

  heading("cholesky_solver_gradients");
  auto chol_grad = pluss::make_series({{2.0, 0.0}, {1.0, 1.4142135623730951}});
  auto chol_rhs_grad = pluss::make_series({2.0, 1.0});
  chol_grad.tracks_slope_(true);
  chol_rhs_grad.tracks_slope_(true);
  chol_grad.linearalg.cholesky_solve(chol_rhs_grad).sum().reverse();
  std::cout << "solve_factor_gradient=" << chol_grad.sensitivity() << "\n";
  std::cout << "solve_rhs_gradient=" << chol_rhs_grad.sensitivity() << "\n";
  auto inverse_factor = pluss::make_series({{2.0, 0.0}, {1.0, 1.4142135623730951}});
  inverse_factor.tracks_slope_(true);
  inverse_factor.linearalg.cholesky_inverse().sum().reverse();
  std::cout << "inverse_factor_gradient=" << inverse_factor.sensitivity() << "\n\n";

  heading("lu_solve_rhs_and_broadcast");
  auto coefficients = pluss::make_series({{3.0, 1.0}, {1.0, 2.0}});
  std::cout << "vector=" << coefficients.linearalg.lu_solve(pluss::make_series({9.0, 8.0})) << "\n";
  std::cout << "matrix=" << coefficients.linearalg.lu_solve(
      pluss::make_series({{9.0, 1.0}, {8.0, 0.0}})) << "\n";
  auto coefficient_batch = pluss::make_series({
      {{3.0, 1.0}, {1.0, 2.0}},
      {{2.0, 0.0}, {0.0, 4.0}},
  });
  std::cout << "broadcast=" << coefficient_batch.linearalg.lu_solve(pluss::make_series({9.0, 8.0})) << "\n\n";

  heading("lu_solve_gradients_and_failure");
  auto a_grad = pluss::make_series({{3.0, 1.0}, {1.0, 2.0}});
  auto b_grad = pluss::make_series({9.0, 8.0});
  a_grad.tracks_slope_(true);
  b_grad.tracks_slope_(true);
  a_grad.linearalg.lu_solve(b_grad).sum().reverse();
  std::cout << "coefficient_gradient=" << a_grad.sensitivity() << "\n";
  std::cout << "rhs_gradient=" << b_grad.sensitivity() << "\n";
  try {
    std::cout << pluss::make_series({{1.0, 2.0}, {2.0, 4.0}})
                     .linearalg.lu_solve(pluss::make_series({1.0, 2.0})) << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }

  return 0;
}

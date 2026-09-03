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
  auto jordan = pluss::make_series({{1.0, 2.0}, {0.0, 1.0}});

  heading("matrix_power_positive_zero_negative");
  std::cout << "cube=" << jordan.linearalg.matrix_power(3) << "\n";
  std::cout << "zero=" << jordan.linearalg.matrix_power(0) << "\n";
  std::cout << "negative_one=" << jordan.linearalg.matrix_power(-1) << "\n\n";

  heading("matrix_power_batch");
  auto power_batch = pluss::make_series({
      {{1.0, 1.0}, {0.0, 1.0}},
      {{2.0, 0.0}, {0.0, 3.0}},
  });
  std::cout << power_batch.linearalg.matrix_power(2) << "\n\n";

  heading("matrix_power_gradient");
  auto power_input = pluss::make_series({{2.0, 0.0}, {0.0, 3.0}});
  power_input.tracks_slope_(true);
  power_input.linearalg.matrix_power(3).sum().reverse();
  std::cout << power_input.sensitivity() << "\n\n";

  heading("matrix_power_negative_gradient");
  auto negative_power_input = pluss::make_series({{2.0, 0.0}, {0.0, 4.0}});
  negative_power_input.tracks_slope_(true);
  negative_power_input.linearalg.matrix_power(-1).sum().reverse();
  std::cout << negative_power_input.sensitivity() << "\n\n";

  heading("matrix_power_shape_error");
  try {
    std::cout << pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}})
                     .linearalg.matrix_power(2) << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n";

  heading("matrix_exp_zero_and_nilpotent");
  auto zero = pluss::make_series({{0.0, 0.0}, {0.0, 0.0}});
  auto nilpotent = pluss::make_series({{0.0, 1.0}, {0.0, 0.0}});
  std::cout << "zero=" << zero.linearalg.matrix_exp() << "\n";
  std::cout << "nilpotent=" << nilpotent.linearalg.matrix_exp() << "\n\n";

  heading("matrix_exp_diagonal_and_large");
  auto diagonal = pluss::make_series({{0.0, 0.0}, {0.0, 0.6931471805599453}});
  auto large = pluss::make_series({{20.0, 0.0}, {0.0, -20.0}});
  std::cout << "diagonal=" << diagonal.linearalg.matrix_exp() << "\n";
  std::cout << "large=" << large.linearalg.matrix_exp() << "\n\n";

  heading("matrix_exp_batch");
  auto exp_batch = pluss::make_series({
      {{0.0, 0.0}, {0.0, 0.0}},
      {{0.0, 1.0}, {0.0, 0.0}},
  });
  std::cout << exp_batch.linearalg.matrix_exp() << "\n\n";

  heading("matrix_exp_gradient");
  auto exp_input = pluss::make_series({{0.0, 0.0}, {0.0, 0.6931471805599453}});
  exp_input.tracks_slope_(true);
  exp_input.linearalg.matrix_exp().sum().reverse();
  std::cout << exp_input.sensitivity() << "\n\n";

  heading("matrix_exp_batch_gradient");
  auto exp_batch_input = pluss::make_series({
      {{0.0, 0.0}, {0.0, 0.0}},
      {{0.0, 0.0}, {0.0, 0.6931471805599453}},
  });
  exp_batch_input.tracks_slope_(true);
  exp_batch_input.linearalg.matrix_exp().sum().reverse();
  std::cout << exp_batch_input.sensitivity() << "\n\n";

  heading("pinverse_tall_and_penrose");
  auto tall = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}});
  auto tall_pinv = tall.linearalg.pinverse();
  std::cout << "pinverse=" << tall_pinv << "\n";
  std::cout << "A_P_A=" << tall.linearalg.matmul(tall_pinv).linearalg.matmul(tall) << "\n\n";

  heading("pinverse_wide");
  auto wide = pluss::make_series({{1.0, 0.0, 1.0}, {0.0, 1.0, 1.0}});
  std::cout << wide.linearalg.pinverse() << "\n\n";

  heading("pinverse_rank_deficient");
  auto rank_one = pluss::make_series({{1.0, 2.0}, {2.0, 4.0}});
  std::cout << rank_one.linearalg.pinverse() << "\n\n";

  heading("pinverse_rcond");
  auto separated = pluss::make_series({{1.0, 0.0}, {0.0, 0.001}});
  std::cout << "default=" << separated.linearalg.pinverse() << "\n";
  std::cout << "rcond_0_01=" << separated.linearalg.pinverse(0.01) << "\n\n";

  heading("pinverse_batch");
  auto pinv_batch = pluss::make_series({
      {{1.0, 0.0}, {0.0, 2.0}, {1.0, 1.0}},
      {{2.0, 0.0}, {0.0, 1.0}, {0.0, 1.0}},
  });
  std::cout << pinv_batch.linearalg.pinverse() << "\n\n";

  heading("pinverse_gradient");
  auto pinv_input = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}});
  pinv_input.tracks_slope_(true);
  pinv_input.linearalg.pinverse().sum().reverse();
  std::cout << pinv_input.sensitivity() << "\n\n";

  heading("pinverse_batch_gradient");
  auto pinv_batch_input = pluss::make_series({
      {{1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}},
      {{2.0, 0.0}, {0.0, 1.0}, {0.0, 1.0}},
  });
  pinv_batch_input.tracks_slope_(true);
  pinv_batch_input.linearalg.pinverse().sum().reverse();
  std::cout << pinv_batch_input.sensitivity() << "\n\n";

  heading("pinverse_parameter_and_shape_errors");
  try {
    std::cout << tall.linearalg.pinverse(-0.1) << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  try {
    std::cout << pluss::make_series({1.0, 2.0}).linearalg.pinverse() << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }

  return 0;
}

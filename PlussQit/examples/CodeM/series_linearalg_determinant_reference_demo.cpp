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
  heading("det_orientation_and_singularity");
  auto positive = pluss::make_series({{4.0, 7.0}, {2.0, 6.0}});
  auto negative = pluss::make_series({{2.0, 6.0}, {4.0, 7.0}});
  auto singular = pluss::make_series({{1.0, 2.0}, {2.0, 4.0}});
  std::cout << "positive=" << positive.linearalg.det() << "\n";
  std::cout << "row_swapped=" << negative.linearalg.det() << "\n";
  std::cout << "singular=" << singular.linearalg.det() << "\n\n";

  heading("det_batched");
  auto batch = pluss::make_series({
      {{1.0, 0.0}, {0.0, 3.0}},
      {{2.0, 1.0}, {1.0, 2.0}},
      {{1.0, 2.0}, {2.0, 4.0}},
  });
  std::cout << batch.linearalg.det() << "\n\n";

  heading("det_gradient");
  auto det_input = pluss::make_series({{2.0, 0.5}, {0.25, 1.5}});
  det_input.tracks_slope_(true);
  det_input.linearalg.det().reverse();
  std::cout << "value=" << det_input.linearalg.det() << "\n";
  std::cout << "d_det_dA=" << det_input.sensitivity() << "\n\n";

  heading("det_shape_error");
  try {
    std::cout << pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}}).linearalg.det() << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n";

  heading("inverse_basic_and_identity");
  auto inverse = positive.linearalg.inverse();
  std::cout << "inverse=" << inverse << "\n";
  std::cout << "A_times_inverse=" << positive.linearalg.mm(inverse) << "\n\n";

  heading("inverse_batched");
  auto invertible_batch = pluss::make_series({
      {{2.0, 0.0}, {0.0, 4.0}},
      {{1.0, 1.0}, {0.0, 2.0}},
  });
  std::cout << invertible_batch.linearalg.inverse() << "\n\n";

  heading("inverse_gradient");
  auto inverse_input = pluss::make_series({{2.0, 0.0}, {0.0, 4.0}});
  inverse_input.tracks_slope_(true);
  inverse_input.linearalg.inverse().sum().reverse();
  std::cout << inverse_input.sensitivity() << "\n\n";

  heading("inverse_singular_error");
  try {
    std::cout << singular.linearalg.inverse() << "\n";
  } catch (const std::exception& e) {
    print_error_prefix(e);
  }
  std::cout << "\n";

  heading("logdet_sign_cases");
  std::cout << "positive=" << positive.linearalg.logdet() << "\n";
  std::cout << "negative=" << negative.linearalg.logdet() << "\n";
  std::cout << "singular=" << singular.linearalg.logdet() << "\n\n";

  heading("logdet_batched");
  std::cout << batch.linearalg.logdet() << "\n\n";

  heading("logdet_gradient");
  auto logdet_input = pluss::make_series({{2.0, 0.0}, {0.0, 4.0}});
  logdet_input.tracks_slope_(true);
  logdet_input.linearalg.logdet().reverse();
  std::cout << logdet_input.sensitivity() << "\n\n";

  heading("logdet_product_identity");
  auto logdet_a = pluss::make_series({{2.0, 0.0}, {0.0, 3.0}});
  auto logdet_b = pluss::make_series({{4.0, 0.0}, {0.0, 5.0}});
  std::cout << "product=" << logdet_a.linearalg.mm(logdet_b).linearalg.logdet() << "\n";
  std::cout << "sum=" << logdet_a.linearalg.logdet().add(logdet_b.linearalg.logdet()) << "\n\n";

  heading("slogdet_sign_cases");
  std::cout << "positive=" << positive.linearalg.slogdet() << "\n";
  std::cout << "negative=" << negative.linearalg.slogdet() << "\n";
  std::cout << "singular=" << singular.linearalg.slogdet() << "\n\n";

  heading("slogdet_batched");
  std::cout << batch.linearalg.slogdet() << "\n\n";

  heading("slogdet_logabs_gradient");
  auto slogdet_input = pluss::make_series({{2.0, 0.0}, {0.0, 4.0}});
  slogdet_input.tracks_slope_(true);
  slogdet_input.linearalg.slogdet().select(0, 1).reverse();
  std::cout << slogdet_input.sensitivity() << "\n\n";

  heading("slogdet_reconstruction");
  auto reconstruction_input = pluss::make_series({{2.0, 6.0}, {4.0, 7.0}});
  auto signed_log = reconstruction_input.linearalg.slogdet();
  auto reconstructed = signed_log.select(0, 0).mul(signed_log.select(0, 1).exp());
  std::cout << reconstructed << "\n";

  return 0;
}

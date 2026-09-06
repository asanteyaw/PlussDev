#include <pluss/pluss.h>

#include <iostream>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

void show(const pluss::Series& value) {
  pluss::series::print(std::cout, value) << "\n\n";
}

pluss::Series sample() {
  return pluss::series::complex(
      pluss::make_series({{3.0f, -1.0f}, {0.0f, 0.0f}}),
      pluss::make_series({{4.0f, 0.0f}, {2.0f, 0.0f}}));
}

} // namespace

int main() {
  const auto z = sample();

  heading("cartesian_components");
  show(z);
  show(z.real());
  show(z.imag());

  heading("cartesian_round_trip");
  show(pluss::series::complex(z.real(), z.imag()));

  heading("components_on_real_input");
  const auto integers = pluss::make_series({-3, 0, 2});
  show(integers.real());
  show(integers.imag());

  heading("real_reverse");
  auto real_leaf = sample();
  real_leaf.tracks_slope_(true);
  real_leaf.real().sum().reverse();
  show(real_leaf.sensitivity());

  heading("imag_reverse");
  auto imag_leaf = sample();
  imag_leaf.tracks_slope_(true);
  imag_leaf.imag().sum().reverse();
  show(imag_leaf.sensitivity());

  heading("angle_quadrants");
  const auto quadrants = pluss::series::complex(
      pluss::make_series({1.0f, 0.0f, -1.0f, 0.0f, 3.0f, 0.0f}),
      pluss::make_series({0.0f, 1.0f, 0.0f, -1.0f, 4.0f, 0.0f}));
  show(quadrants.angle());

  heading("angle_real_input");
  show(pluss::make_series({2.0, -3.0, 0.0}).angle());

  heading("angle_reverse");
  auto phase_leaf = pluss::series::complex(pluss::make_series({3.0f, 0.0f}),
                                           pluss::make_series({4.0f, 0.0f}));
  phase_leaf.tracks_slope_(true);
  phase_leaf.angle().sum().reverse();
  show(phase_leaf.sensitivity());

  heading("conjugation");
  show(z.conj());
  show(z.conj().conj());

  heading("conjugate_product");
  show(z.mul(z.conj()).real());
  show(z.abs().square());

  heading("conjugate_reverse");
  auto conjugate_leaf = sample();
  conjugate_leaf.tracks_slope_(true);
  conjugate_leaf.conj().imag().sum().reverse();
  show(conjugate_leaf.sensitivity());

  heading("sgn_real");
  show(pluss::make_series({-3.0, 0.0, 2.5}).sgn());

  heading("sgn_complex");
  show(z.sgn());
  show(z.sgn().abs());

  heading("sgn_reverse");
  auto direction_leaf = pluss::series::complex(pluss::make_series({3.0f}),
                                               pluss::make_series({4.0f}));
  direction_leaf.tracks_slope_(true);
  direction_leaf.sgn().real().sum().reverse();
  show(direction_leaf.sensitivity());

  return 0;
}

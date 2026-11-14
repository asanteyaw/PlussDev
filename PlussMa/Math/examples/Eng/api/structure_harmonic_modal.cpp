#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>
#include <complex>

#include <cmath>
#include <iostream>

namespace structure = pluss::subsp::eng::structure;
using namespace pluss::tils;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a straight steel beam of length 2 m along x, with density, divided into equal members
static structure::Frame beam(int members, bool with_density = true) {
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int i = 0; i <= members; ++i) {
    position.push_back(2.0 * i / members);
    position.push_back(0.0);
  }
  for (int i = 0; i < members; ++i) {
    from.push_back(i);
    to.push_back(i + 1);
  }
  structure::Sections sections{vec(std::vector<double>(members, 1e-3)), vec(std::vector<double>(members, 8.333e-8)), vec(std::vector<double>(members, 200e9))};
  if (with_density) sections.density = vec(std::vector<double>(members, 7850.0));
  return structure::frame({grid(position, 2), from, to}, sections);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const int members = 8;
  const auto frame = beam(members);
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const std::size_t tip = 3 * members + 1;
  const double force = 10.0, b = 1e-4;
  const std::vector<double> frequencies = {10, 30, 100, 150};
  std::vector<double> loads;
  for (std::size_t f = 0; f < frequencies.size(); ++f) {
    std::vector<double> load(3 * (members + 1), 0.0);
    load[tip] = force;
    loads.insert(loads.end(), load.begin(), load.end());
  }
  structure::HarmonicOptions options;
  options.stiffness_damping = b;
  const auto h = structure::harmonic(frame, pluss::series::make(loads).reshape({(std::int64_t)frequencies.size(), members + 1, 3}), vec(frequencies), wall, options);
  const auto direct = h.displacement.select(-2, members).select(-1, 1);
  const auto re = flat(direct.real()), im = flat(direct.imag());

  // the same response as a sum over the first six mass-normalised modes
  structure::EigOptions eig;
  eig.count = 6;
  const auto modes = structure::modes(frame, wall, eig);
  const auto omega = flat(modes.omega), shape = flat(modes.shape);
  out.writeln("omega    direct (mm, phase deg)       mode sum (mm, phase deg)");
  for (std::size_t f = 0; f < frequencies.size(); ++f) {
    std::complex<double> sum = 0.0;
    for (int n = 0; n < 6; ++n) {
      const double phi = shape[n * 3 * (members + 1) + tip];
      sum += phi * phi * force / std::complex<double>(omega[n] * omega[n] - frequencies[f] * frequencies[f], b * omega[n] * omega[n] * frequencies[f]);
    }
    out.writeln(frequencies[f], "   ", 1000.0 * std::hypot(re[f], im[f]), " ", std::atan2(im[f], re[f]) * 180.0 / M_PI, "      ", 1000.0 * std::abs(sum), " ", std::arg(sum) * 180.0 / M_PI);
  }
}

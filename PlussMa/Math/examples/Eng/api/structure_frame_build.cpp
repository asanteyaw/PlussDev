#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

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
  out.print_defaults(PrintOptions{}.with_sep(""));
  // a portal frame: nodes 0 left base, 1 left top, 2 right top, 3 right base; members: left column, beam, right column
  const structure::Mesh portal{grid({0, 0, 0, 3, 4, 3, 4, 0}, 2), {0, 1, 3}, {1, 2, 2}};
  const structure::Sections sections{vec({2e-2, 2e-2, 2e-2}), vec({6.667e-5, 6.667e-5, 6.667e-5}), vec({200e9, 200e9, 200e9})};
  const auto frame = structure::frame(portal, sections);
  out.writeln("without density: nodes ", frame.nodes(), ", members ", frame.members(), ", degrees of freedom per node ", frame.dofs());

  // the density is optional; with it the frame has mass, and mass() returns the inertial force of an acceleration
  structure::Sections with_mass = sections;
  with_mass.density = vec({7850.0, 7850.0, 7850.0});
  const auto heavy = structure::frame(portal, with_mass);
  std::vector<double> gravity(12, 0.0);
  for (int node = 0; node < 4; ++node) gravity[3 * node + 1] = 1.0;
  const auto force = structure::mass(heavy, grid(gravity, 3));
  double total = 0.0;
  for (int node = 0; node < 4; ++node) total += force.select(-2, node).select(-1, 1).item<double>();
  out.writeln("total mass from mass() (kg): ", total, "   rho A (2 L_column + L_beam) = ", 7850.0 * 2e-2 * (3 + 4 + 3));
}

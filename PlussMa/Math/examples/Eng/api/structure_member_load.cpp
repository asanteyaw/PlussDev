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
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a 4 m beam in two members, pinned at the left end and on a roller at the right end
  std::vector<double> position = {0, 0, 2, 0, 4, 0};
  const structure::Frame frame = structure::frame({grid(position, 2), {0, 1}, {1, 2}}, {vec({1e-3, 1e-3}), vec({1e-7, 1e-7}), vec({200e9, 200e9})});
  const structure::Fix supports{{0, 0, 2}, {0, 1, 1}, {}};
  const auto no_nodal_load = pluss::series::zeros({3, 3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));

  // transverse: 1000 N/m downward on both members (local y points up for a member running left to right)
  const auto sagging = structure::solve(frame, no_nodal_load, structure::MemberLoad{vec({-1000.0, -1000.0}), {}}, supports);
  out.writeln("uniform load: mid-span deflection (mm) ", 1000.0 * flat(sagging.displacement)[4], "   formula ", -1000.0 * 5 * 1000.0 * 256.0 / (384 * 200e9 * 1e-7));
  out.writeln("  reactions (N) ", flat(sagging.reaction)[1], " ", flat(sagging.reaction)[7], ";  moment at mid-span from the end action (N m) ", flat(sagging.end_force)[5], "   formula ", 1000.0 * 16.0 / 8.0);

  // load on one member only: the other carries none
  const auto half = structure::solve(frame, no_nodal_load, structure::MemberLoad{vec({-1000.0, 0.0}), {}}, supports);
  out.writeln("load on the first member only: reactions (N) ", flat(half.reaction)[1], " ", flat(half.reaction)[7], "   formula ", 1000.0 * 2.0 * 3.0 / 4.0, " ", 1000.0 * 2.0 * 1.0 / 4.0);

  // axial: a vertical bar hanging from the ceiling under its own weight, 2000 N/m along the member axis (which points down)
  const auto bar = structure::frame({grid({0, 0, 0, -3}, 2), {0}, {1}}, {vec({1e-3}), vec({8e-8}), vec({200e9})});
  const auto hanging = structure::solve(bar, pluss::series::zeros({2, 3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64)), structure::MemberLoad{{}, vec({2000.0})}, {{0, 0, 0}, {0, 1, 2}, {}});
  out.writeln("hanging bar: tip drop (mm) ", -1000.0 * flat(hanging.displacement)[4], "   formula ", 1000.0 * 2000.0 * 9.0 / (2 * 200e9 * 1e-3), ";  ceiling reaction (N) ", flat(hanging.reaction)[1]);

  // both members together, and a batch of three intensities
  const auto intensities = pluss::series::make(std::vector<double>{-500, -500, -1000, -1000, -2000, -2000}).reshape({3, 2});
  const auto batch = structure::solve(frame, no_nodal_load, structure::MemberLoad{intensities, {}}, supports);
  out.writeln("batch of three intensities: mid-span deflection (mm) ", flat(batch.displacement.select(-2, 1).select(-1, 1).mul(1000.0)));
}

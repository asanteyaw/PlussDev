#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <iostream>

namespace structure = pluss::ma::eng::structure;
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
  // a plane truss: two bars from the feet (0, 0) and (6, 0) to the apex (3, 4)
  const structure::Mesh plane{grid({0, 0, 6, 0, 3, 4}, 2), {0, 1}, {2, 2}};
  const auto triangle = structure::truss(plane, {vec({1e-3, 1e-3}), vec({200e9, 200e9})});
  out.writeln("plane truss: nodes ", triangle.nodes(), ", bars ", triangle.bars(), ", axes ", triangle.dim());

  // a space truss: three legs from a ring on the ground to an apex 2 m above its centre
  const structure::Mesh space{grid({1.5, 0, 0, -0.75, 1.299038105676658, 0, -0.75, -1.299038105676658, 0, 0, 0, 2}, 3), {0, 1, 2}, {3, 3, 3}};
  const auto tripod = structure::truss(space, {vec({1e-3, 1e-3, 1e-3}), vec({200e9, 200e9, 200e9})});
  out.writeln("space truss: nodes ", tripod.nodes(), ", bars ", tripod.bars(), ", axes ", tripod.dim());

  // a batch of three apex heights: a leading axis on the positions describes three trusses at once
  const auto positions = pluss::series::make(std::vector<double>{0, 0, 6, 0, 3, 2, 0, 0, 6, 0, 3, 4, 0, 0, 6, 0, 3, 8}).reshape({3, 3, 2});
  const auto batch = structure::truss({positions, {0, 1}, {2, 2}}, {vec({1e-3, 1e-3}), vec({200e9, 200e9})});
  const auto result = structure::solve(batch, grid({0, 0, 0, 0, 0, -10000}, 2), {{0, 0, 1, 1}, {0, 1, 0, 1}, {}});
  out.writeln("batch of three heights: bar forces have shape ", result.force.sizes());
}

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
  out.print_defaults(PrintOptions{}.fixed(1).with_sep(""));
  const double euler = M_PI * M_PI * 200e9 * 8.333e-8 / 4.0;
  out.writeln("pi^2 E I / L^2 = ", euler, " N");
  const int members = 16;
  const auto column = beam(members, false);
  std::vector<double> load(3 * (members + 1), 0.0);
  load[3 * members] = -1.0;   // a 1 N reference load: the factor is the critical load in newtons
  struct Case { const char *name; structure::Fix fix; double units; };
  const Case cases[] = {
      {"cantilever            ", {{0, 0, 0}, {0, 1, 2}, {}}, 0.25},
      {"pinned-pinned         ", {{0, 0, members}, {0, 1, 1}, {}}, 1.0},
      {"fixed-pinned          ", {{0, 0, 0, members}, {0, 1, 2, 1}, {}}, 2.0457},
      {"fixed-fixed (sliding) ", {{0, 0, 0, members, members}, {0, 1, 2, 1, 2}, {}}, 4.0},
  };
  for (const Case &c : cases) {
    const auto r = structure::buckling(column, grid(load, 3), c.fix);
    out.writeln(c.name, "critical load (N) ", flat(r.factor)[0], "   Euler ", c.units * euler);
  }
}

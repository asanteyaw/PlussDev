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
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  const int members = 8;
  const auto column = beam(members, false);
  const double euler = M_PI * M_PI * 200e9 * 8.333e-8 / 4.0;
  out.writeln("Euler load of the pinned column (N): ", euler);

  // a 1000 N compressive reference load along the column: the factors times 1000 are the critical loads
  std::vector<double> load(3 * (members + 1), 0.0);
  load[3 * members] = -1000.0;
  structure::EigOptions options;
  options.count = 3;
  const auto r = structure::buckling(column, grid(load, 3), {{0, 0, members}, {0, 1, 1}, {}}, options);
  out.writeln("factors: ", flat(r.factor), ", critical loads (N): ", flat(r.factor.mul(1000.0)));
  out.writeln("converged ", flat(r.converged), ", residuals ", flat(r.residual), ", shape array ", r.shape.sizes(), ", iterations ", r.iterations);

  // the same column fixed at the base and free at the top (a cantilever): a quarter of the load
  const auto cantilever = structure::buckling(column, grid(load, 3), {{0, 0, 0}, {0, 1, 2}, {}});
  out.writeln("cantilever critical load (N): ", flat(cantilever.factor)[0] * 1000.0, "   Euler / 4 = ", euler / 4.0);

  // a reference load that doesn't compress anything
  std::vector<double> pull(3 * (members + 1), 0.0);
  pull[3 * members] = 1000.0;
  try {
    (void)structure::buckling(column, grid(pull, 3), {{0, 0, members}, {0, 1, 1}, {}});
  } catch (const std::exception &e) {
    std::cout << "tensile reference load: " << e.what() << "\n";
  }
}

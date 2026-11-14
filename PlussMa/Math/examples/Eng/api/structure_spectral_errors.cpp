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

#include <functional>

static void report(const char *what, const std::function<void()> &call) {
  try {
    call();
    std::cout << what << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << what << ": " << e.what() << "\n";
  }
}

int main() {
  const int members = 4;
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  std::vector<double> push(3 * (members + 1), 0.0);
  push[3 * members] = -1000.0;
  report("modes without density      ", [&] { (void)structure::modes(beam(members, false), wall); });
  report("mass without density       ", [&] { (void)structure::mass(beam(members, false), grid(std::vector<double>(3 * (members + 1), 0.0), 3)); });
  report("more modes than free dofs   ", [&] { structure::EigOptions o; o.count = 50; (void)structure::modes(beam(members), wall, o); });
  report("zero modes requested        ", [&] { structure::EigOptions o; o.count = 0; (void)structure::modes(beam(members), wall, o); });
  report("buckling, tensile reference ", [&] { for (auto &v : push) v = -v; (void)structure::buckling(beam(members, false), grid(push, 3), {{0, 0, members}, {0, 1, 1}, {}}); });
}

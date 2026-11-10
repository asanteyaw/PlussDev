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

#include <functional>

static void report(const char *what, const std::function<void()> &build) {
  try {
    build();
    std::cout << what << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << what << ": " << e.what() << "\n";
  }
}

int main() {
  const auto line = grid({0, 0, 2, 0}, 2);
  const structure::Bars good{vec({1e-3}), vec({200e9})};
  report("negative area         ", [&] { (void)structure::truss({line, {0}, {1}}, {vec({-1e-3}), vec({200e9})}); });
  report("bar from a node to itself", [&] { (void)structure::truss({line, {0}, {0}}, good); });
  report("coincident ends       ", [&] { (void)structure::truss({grid({0, 0, 0, 0}, 2), {0}, {1}}, good); });
  report("node index out of range", [&] { (void)structure::truss({line, {0}, {5}}, good); });
  report("wrong area count      ", [&] { (void)structure::truss({line, {0}, {1}}, {vec({1e-3, 1e-3}), vec({200e9})}); });
  report("coordinates not a grid", [&] { (void)structure::truss({vec({0, 2}), {0}, {1}}, good); });
}

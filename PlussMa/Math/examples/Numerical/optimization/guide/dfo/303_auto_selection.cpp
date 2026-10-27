// What `Auto` picks. The choice depends on whether the problem is a black box, whether it has constraints, whether it
// has bounds, and how many variables it has. `selection_reason` says which rule fired. The rules are the same ones
// listed in the chapter; this program prints them for a spread of problems.
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <vector>

namespace dfo = pluss::subsp::math::optimization;
using namespace pluss::tils;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series sphere(const pluss::Series& x) { return x.square().sum(); }

int main() {
  struct Case {
    const char* label;
    std::int64_t n;
    bool bounded, constrained;
  };
  const std::vector<Case> cases = {{"3 variables, no bounds", 3, false, false},  {"12 variables, no bounds", 12, false, false}, {"50 variables, no bounds", 50, false, false},
                                   {"3 variables, bounds", 3, true, false},      {"50 variables, bounds", 50, true, false},      {"3 variables, one constraint", 3, false, true}};

  OutputEngine out;
  for (const auto& c : cases) {
    dfo::DfoProblem p;
    p.initial = filled(c.n, 1.5);
    p.objective = sphere;
    if (c.bounded) {
      p.lower_bounds = filled(c.n, -5.0);
      p.upper_bounds = filled(c.n, 5.0);
    }
    if (c.constrained) p.constraints = [](const pluss::Series& x) { return (x.sum() * -1.0 + 1.0).reshape({1}); };  // 1 - sum(x) <= 0
    dfo::DfoOptions o;
    o.max_evaluations = 3000;
    const auto r = dfo::DFO::solve(p, o);
    out.writeln(c.label);
    out.writeln("    ->", dfo::to_string(r.selected_solver) + ":", r.selection_reason);
  }
}

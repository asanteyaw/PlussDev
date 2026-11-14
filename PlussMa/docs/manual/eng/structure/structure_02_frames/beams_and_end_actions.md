<!-- generated from beams_and_end_actions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Beams and end actions

[Back to Structures 2: Frames](../structure_02_frames.md)

A beam is the simplest frame: a straight line of members carrying loads across its length. This page solves a cantilever and a simply supported beam against the textbook formulas, shows how to read the bending-moment diagram from the member end actions, and applies uniform loads along the members.

## The mental model

`solve(frame, load, fix)` takes a `Frame`, a `load` of shape `[..., node, 3]` holding `(Fx, Fy, Mz)` at each node, and a `Fix` naming the supported degrees of freedom (axes `0`, `1`, `2` are x, y and rotation). It returns a `FrameResult`:

| Member | Shape (single structure) | Meaning |
|---|---|---|
| `displacement` | `[node, 3]` | `(ux, uy, rz)` of each node |
| `reaction` | `[node, 3]` | force and moment each support applies; zero at free degrees of freedom |
| `end_force` | `[member, 6]` | local end actions `(Ni, Vi, Mi, Nj, Vj, Mj)` of each member |
| `energy` | `[]` | total strain energy |
| `residual_norm`, `converged`, `breakdown` | `[]` | solve diagnostics, as for a truss |

The member model is the Euler-Bernoulli beam with axial stiffness: cubic displacement along the member between its two nodes. That model gives the **exact** displacements and rotations at the nodes for loads applied at the nodes, however long the member, so a single member between two supports or a free end is already exact for point loads there. More members are needed only where a load is applied, the section changes or the geometry bends.

Loads at points are applied at nodes; a uniform load along members is given as a `MemberLoad`, as the last section of this page shows.

## A cantilever

A cantilever of length `L = 2 m`, with `E = 200 GPa` and `I = 1e-7 m^4` (`E I = 20 kN m^2`), is built into a wall at its left end and carries a `100 N` load pointing down at the free end. The classical results are the tip deflection `-P L^3 / (3 E I) = -13.333 mm`, the tip rotation `-P L^2 / (2 E I) = -0.01 rad` (the tip turns clockwise), and at the wall a reaction of `P = 100 N` upward and a moment of `P L = 200 N m` counterclockwise.

Because the formula is exact for an end load, the answer must not change when the cantilever is split into more members. The program solves it with one member, with two, and with eight, and prints the tip results of each beside the formulas.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a straight beam along x with the given number of equal members
static st::Frame beam(int members, double length, double inertia) {
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int i = 0; i <= members; ++i) {
    position.push_back(length * i / members);
    position.push_back(0.0);
  }
  for (int i = 0; i < members; ++i) {
    from.push_back(i);
    to.push_back(i + 1);
  }
  const std::vector<double> area(members, 1e-3), inertias(members, inertia), modulus(members, 200e9);
  return st::frame({grid(position, 2), from, to}, {vec(area), vec(inertias), vec(modulus)});
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const double length = 2.0, inertia = 1e-7, load = 100.0, stiffness = 200e9 * inertia;
  out.writeln("formulas: tip deflection (mm) ", -1000.0 * load * length * length * length / (3 * stiffness), ", tip rotation (rad) ", -load * length * length / (2 * stiffness),
              ", wall reaction (N, N m) ", load, " ", load * length);

  for (int members : {1, 2, 8}) {
    const auto frame = beam(members, length, inertia);
    // the wall fixes the three degrees of freedom of node 0
    const st::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
    std::vector<double> nodal(3 * (members + 1), 0.0);
    nodal[3 * members + 1] = -load;   // 100 N down at the free end
    const auto r = st::solve(frame, grid(nodal, 3), wall);
    const auto u = flat(r.displacement);
    const auto reaction = flat(r.reaction);
    out.writeln(members, " member(s): tip deflection (mm) ", 1000.0 * u[3 * members + 1], ", tip rotation (rad) ", u[3 * members + 2], ", wall reaction (N, N m) ", reaction[1], " ", reaction[2]);
  }
}
```

Output

```text
formulas: tip deflection (mm) -13.33333, tip rotation (rad) -0.01000, wall reaction (N, N m) 100.00000 200.00000
1 member(s): tip deflection (mm) -13.33333, tip rotation (rad) -0.01000, wall reaction (N, N m) 100.00000 200.00000
2 member(s): tip deflection (mm) -13.33333, tip rotation (rad) -0.01000, wall reaction (N, N m) 100.00000 200.00000
8 member(s): tip deflection (mm) -13.33333, tip rotation (rad) -0.01000, wall reaction (N, N m) 100.00000 200.00000
```

All three meshes agree with the formulas to the printed digits: the deflection `-13.33333 mm`, the rotation `-0.01 rad`, the reaction `100 N` and the wall moment `200 N m`. A finer mesh adds nothing here, which is the property of the cubic member the mental model describes.

## A simply supported beam and its moment diagram

A beam of length `4 m` is pinned at its left end (fixed in x and y, free to rotate) and rests on a roller at its right end (fixed in y only), with a `1000 N` load pointing down at its middle. The classical results are a mid-span deflection `-P L^3 / (48 E I)`, end rotations `-/+ P L^2 / (16 E I)`, reactions of `P / 2 = 500 N` each, and a bending moment that rises linearly to `P L / 4 = 1000 N m` at mid-span.

The end actions give the bending moment directly. With four members there are nodes at `0, 1, 2, 3, 4 m`. At node `k`, between member `k - 1` (ending there) and member `k` (starting there), the bending moment is `+Mj` of the member ending at the node, and `-Mi` of the member starting at it; the two agree because the moment is continuous. The program reads the diagram from the `Mj` of each member and compares it with the formula for a load at mid-span, `M(x) = P x / 2` for `x` up to the middle.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static st::Frame beam(int members, double length, double inertia) {
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int i = 0; i <= members; ++i) {
    position.push_back(length * i / members);
    position.push_back(0.0);
  }
  for (int i = 0; i < members; ++i) {
    from.push_back(i);
    to.push_back(i + 1);
  }
  const std::vector<double> area(members, 1e-3), inertias(members, inertia), modulus(members, 200e9);
  return st::frame({grid(position, 2), from, to}, {vec(area), vec(inertias), vec(modulus)});
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const int members = 4;
  const double length = 4.0, inertia = 1e-7, load = 1000.0, stiffness = 200e9 * inertia;
  const auto frame = beam(members, length, inertia);

  std::vector<double> nodal(3 * (members + 1), 0.0);
  nodal[3 * (members / 2) + 1] = -load;   // 1000 N down at the middle node
  const st::Fix supports{{0, 0, members}, {0, 1, 1}, {}};   // pin at node 0, roller at the last node
  const auto r = st::solve(frame, grid(nodal, 3), supports);

  const auto u = flat(r.displacement);
  out.writeln("mid-span deflection (mm)  solver ", 1000.0 * u[3 * (members / 2) + 1], "   formula ", -1000.0 * load * length * length * length / (48 * stiffness));
  out.writeln("end rotations (rad)       solver ", u[2], " ", u[3 * members + 2], "   formula ", -load * length * length / (16 * stiffness), " ", load * length * length / (16 * stiffness));
  out.writeln("reactions (N)             ", flat(r.reaction)[1], " ", flat(r.reaction)[3 * members + 1]);

  // bending moment (sagging positive) at each node: Mj of the member that ends there, and 0 at the left end (-Mi of member 0)
  const auto action = flat(r.end_force);   // six values per member
  out.writeln("bending moment at the nodes, from the end actions (N m) and from M(x) = P x / 2:");
  out.writeln("  x = 0: ", -action[2], "   formula 0");
  for (int member = 0; member < members; ++member) {
    const double x = length * (member + 1) / members;
    const double formula = load * std::min(x, length - x) / 2.0;
    out.writeln("  x = ", x, ": ", action[6 * member + 5], "   formula ", formula);
  }
}
```

Output

```text
mid-span deflection (mm)  solver -66.6667   formula -66.6667
end rotations (rad)       solver -0.0500 0.0500   formula -0.0500 0.0500
reactions (N)             500.0000 500.0000
bending moment at the nodes, from the end actions (N m) and from M(x) = P x / 2:
  x = 0: 0.0000   formula 0
  x = 1.0000: 500.0000   formula 500.0000
  x = 2.0000: 1000.0000   formula 1000.0000
  x = 3.0000: 500.0000   formula 500.0000
  x = 4.0000: 0.0000   formula 0.0000
```

The solver's deflection and rotations equal the formulas, the reactions are `500 N` each, and the moment diagram is exact at every node: `0, 500, 1000, 500, 0 N m`, a triangle peaking at mid-span. The simply supported ends carry no moment, as a pin should.

## Distributed loads

A uniform load along a member, a floor's weight on a beam or a member's own weight, is described by a `MemberLoad` and passed between the nodal load and the supports: `solve(frame, load, member_load, fix)`. Its two members are `transverse`, the intensity along the member's local y axis, and `axial`, the intensity along its local x axis, each in force per length with shape `[..., member]`; either may be left undefined. For a member running from left to right, local y points up, so a downward load is negative. The solver carries the member load exactly and its `end_force` includes the member's own load, so the end actions are the true actions at the ends of each member, without any correction on your side. Loads that vary along a member are written as a uniform load on each of several shorter members.

The program applies `w = 1000 N/m` downward to the whole of a `4 m` simply supported beam, with two members and with four, and no nodal loads. The classical results are a mid-span deflection of `-5 w L^4 / (384 E I)`, reactions of `w L / 2 = 2000 N`, and a bending moment `M(x) = w x (L - x) / 2`, peaking at `w L^2 / 8 = 2000 N m`. The moment at each node is read from the `Mj` of the member that ends there, as in the previous section.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static st::Frame beam(int members, double length, double inertia) {
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int i = 0; i <= members; ++i) {
    position.push_back(length * i / members);
    position.push_back(0.0);
  }
  for (int i = 0; i < members; ++i) {
    from.push_back(i);
    to.push_back(i + 1);
  }
  const std::vector<double> area(members, 1e-3), inertias(members, inertia), modulus(members, 200e9);
  return st::frame({grid(position, 2), from, to}, {vec(area), vec(inertias), vec(modulus)});
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const double length = 4.0, inertia = 1e-7, w = 1000.0, stiffness = 200e9 * inertia;
  out.writeln("exact mid-span deflection (mm): ", -1000.0 * 5 * w * std::pow(length, 4) / (384 * stiffness));

  for (int members : {2, 4}) {
    const auto frame = beam(members, length, inertia);
    const st::Fix supports{{0, 0, members}, {0, 1, 1}, {}};
    const st::MemberLoad uniform{vec(std::vector<double>(members, -w)), {}};   // downward, on every member
    const auto no_nodal_load = pluss::series::zeros({members + 1, 3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
    const auto r = st::solve(frame, no_nodal_load, uniform, supports);

    const auto u = flat(r.displacement);
    const auto action = flat(r.end_force);
    out.writeln(members, " members: mid-span deflection (mm) ", 1000.0 * u[3 * (members / 2) + 1], ", reactions (N) ", flat(r.reaction)[1], " ", flat(r.reaction)[3 * members + 1]);
    std::vector<double> moments, exact;
    for (int member = 0; member < members; ++member) {
      const double x = length * (member + 1) / members;
      moments.push_back(action[6 * member + 5] + 0.0);   // Mj of the member that ends at x
      exact.push_back(w * x * (length - x) / 2.0);
    }
    out.writeln("  bending moment at the nodes after the first (N m): ", moments, "   exact ", exact);
  }
}
```

Output

```text
exact mid-span deflection (mm): -166.6667
2 members: mid-span deflection (mm) -166.6667, reactions (N) 2000.0000 2000.0000
  bending moment at the nodes after the first (N m): [2000.0000, -0.0000]   exact [2000.0000, 0.0000]
4 members: mid-span deflection (mm) -166.6667, reactions (N) 2000.0000 2000.0000
  bending moment at the nodes after the first (N m): [1500.0000, 2000.0000, 1500.0000, 0.0000]   exact [1500.0000, 2000.0000, 1500.0000, 0.0000]
```

The deflection is exact for both meshes, the reactions are `2000 N`, and the moments at the nodes equal `w x (L - x) / 2` to the printed digits, from the end actions directly. Because the load is carried exactly, a coarse mesh is as accurate at the nodes as a fine one; more members are needed only to see the shape between nodes.

A uniform load along the member's axis is the `axial` member. The program hangs a vertical bar from a ceiling and gives it a uniform downward load `p = 2000 N/m`, as its own weight would be, on a `3 m` bar with `E A = 200 MN`. The tip drops by `p L^2 / (2 E A) = 0.045 mm`, the ceiling carries `p L = 6000 N`, and the axial force in the bar falls from `p L` at the top to zero at the free end. The bar runs from the ceiling at node 0 down to node 1, so its local x axis points down, and a load along the weight's direction is positive.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const double length = 3.0, p = 2000.0, ea = 200e9 * 1e-3;
  // the ceiling at (0, 0), the free end at (0, -3): the member direction, and so local x, points down
  const auto bar = st::frame({grid({0, 0, 0, -3}, 2), {0}, {1}}, {vec({1e-3}), vec({8e-8}), vec({200e9})});
  const st::Fix ceiling{{0, 0, 0}, {0, 1, 2}, {}};
  const st::MemberLoad own_weight{{}, vec({p})};
  const auto r = st::solve(bar, pluss::series::zeros({2, 3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64)), own_weight, ceiling);

  out.writeln("tip drop (mm): ", -1000.0 * flat(r.displacement)[4], "   formula p L^2 / (2 E A) = ", 1000.0 * p * length * length / (2 * ea));
  out.writeln("ceiling reaction (N, upward): ", flat(r.reaction)[1], "   formula p L = ", p * length);
  const auto action = flat(r.end_force);
  out.writeln("axial end actions Ni, Nj (N): ", action[0], " ", action[3], "   (the node at the ceiling pulls the bar up with p L; the free end carries nothing)");
}
```

Output

```text
tip drop (mm): 0.0450   formula p L^2 / (2 E A) = 0.0450
ceiling reaction (N, upward): 6000.0000   formula p L = 6000.0000
axial end actions Ni, Nj (N): -6000.0000 0.0000   (the node at the ceiling pulls the bar up with p L; the free end carries nothing)
```

The drop and the reaction equal their formulas, and the end actions show the axial force `p L` at the ceiling end and nothing at the free end.

## Decisions

- **A concentrated load:** put a node there; one member per segment between loads and supports is exact.
- **A distributed load:** a `MemberLoad` for a uniform load; for one that varies along the beam, split it into members and give each its own intensity, as a staircase that follows the load.
- **Reading a diagram:** the sagging bending moment is `-Mi` at the start and `+Mj` at the end of a member; the shear and axial force come from `V` and `N` in the same local axes.

## Pitfalls

Each of these mistakes gives a result that looks plausible. The program makes each one and prints what it does: a transverse member load of the wrong sign, a moment entered where a force was meant, and a member load with the wrong number of entries.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // a cantilever of length 2 m, E I = 20 kN m^2, one member
  const auto frame = st::frame({grid({0, 0, 2, 0}, 2), {0}, {1}}, {vec({1e-3}), vec({1e-7}), vec({200e9})});
  const st::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const auto no_nodal_load = pluss::series::zeros({2, 3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));

  // 1. the sign of a transverse member load: local y points up for a member that runs left to right
  for (double q : {-100.0, 100.0}) {
    const auto r = st::solve(frame, no_nodal_load, st::MemberLoad{vec({q}), {}}, wall);
    out.writeln("transverse load ", q, " N/m: tip deflection (mm) ", 1000.0 * flat(r.displacement)[4]);
  }

  // 2. the third entry of a nodal load is a moment, not a force: 100 in the third place bends the tip by a moment of 100 N m
  const auto moment = st::solve(frame, grid({0, 0, 0, 0, 0, 100}, 3), wall);
  out.writeln("a 100 N m moment at the tip: deflection (mm) ", 1000.0 * flat(moment.displacement)[4], ", rotation (rad) ", flat(moment.displacement)[5],
              "   formulas M L^2 / (2 E I) and M L / (E I): ", 1000.0 * 100 * 4 / (2 * 20000.0), " ", 100 * 2 / 20000.0);

  // 3. a member load must have one entry per member
  try {
    (void)st::solve(frame, no_nodal_load, st::MemberLoad{vec({-100.0, -100.0}), {}}, wall);
  } catch (const std::exception &e) {
    std::cout << "two entries for one member: " << e.what() << "\n";
  }
}
```

Output

```text
transverse load -100.000 N/m: tip deflection (mm) -10.000
transverse load 100.000 N/m: tip deflection (mm) 10.000
a 100 N m moment at the tip: deflection (mm) 10.000, rotation (rad) 0.010   formulas M L^2 / (2 E I) and M L / (E I): 10.000 0.010
two entries for one member: eng::structure::solve: the transverse member load must end in the member count
```

A downward load needs a negative `transverse` intensity (and a positive one deflects the beam upward); a quantity written in the third place of a nodal load is a moment, with the rotation and the deflection of a moment-loaded cantilever; and a member load of the wrong length is rejected with an error naming the member count. Rotations are in radians and moments in newton metres.

Reference: [`solve`](../../../../api/eng/structure/statics/solve.md), [`FrameResult`](../../../../api/eng/structure/statics/FrameResult.md), [`MemberLoad`](../../../../api/eng/structure/statics/MemberLoad.md), [`Fix`](../../../../api/eng/structure/model/Fix.md), [`frame`](../../../../api/eng/structure/model/frame.md).

Where next: [Portal frames](portal_frames.md) solves a frame with columns and a beam, where the stiffness of the beam decides how far the structure sways.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="describing_a_frame.html">
    <span class="label">Previous</span>
    <span class="title">Describing a frame</span>
  </a>
  <a class="next-link" href="portal_frames.html">
    <span class="label">Next</span>
    <span class="title">Portal frames</span>
  </a>
</div>
<!-- apiref:nav:end -->

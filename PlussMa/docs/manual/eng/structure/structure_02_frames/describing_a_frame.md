<!-- generated from describing_a_frame.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Describing a frame

[Back to Structures 2: Frames](../structure_02_frames.md)

A frame is a set of straight members joined rigidly at nodes. Unlike a truss bar, a frame member carries an axial force, a shear force and a bending moment, and a joint transmits a moment from one member to the next. `structure::frame` takes the geometry and the cross-section of every member and compiles them into a `Frame` that the solvers analyse. This page explains how a frame is written down: the nodes and the three degrees of freedom at each, the section properties and how to compute them, the local axes and sign conventions of member end actions, and the checks that reject a description that cannot be a frame.

## The mental model

A frame is described by a `Mesh` for the geometry and `Sections` for the members:

| Structure | Member | Shape | Meaning |
|---|---|---|---|
| `Mesh` | `position` | `[..., node, 2]` | coordinates of every node, in the plane of the frame |
| `Mesh` | `from`, `to` | one index per member | the nodes at the start and the end of each member |
| `Sections` | `area` | `[..., member]` | cross-sectional area `A`, resisting axial force |
| `Sections` | `inertia` | `[..., member]` | second moment of area `I` about the axis perpendicular to the plane, resisting bending |
| `Sections` | `modulus` | `[..., member]` | Young's modulus `E` |
| `Sections` | `density` | `[..., member]` | mass per volume; optional, needed only for mass, vibration and time response |

A frame lives in a plane, so `position` has exactly two axes (a frame in space is not supported; a truss is). A member of length `L` has an axial stiffness `E A / L` and a bending stiffness that scales as `E I / L^3`. Use one consistent system of units: the programs here use newtons, metres and pascals.

Each node has **three degrees of freedom**, in this order: the displacement along x, the displacement along y and the rotation about the out-of-plane axis, positive counterclockwise. `Frame::dofs()` returns 3. Loads, supports and results all use that order: a load at a node is a triple `(Fx, Fy, Mz)`, a result array has shape `[..., node, 3]`, and a constraint `Fix{node, axis}` has an axis of `0` (x), `1` (y) or `2` (rotation).

## Section properties and a first frame

The section properties come from the shape of the cross-section. For a solid rectangle of width `b` and depth `d`, bending in the plane of the depth, `A = b d` and `I = b d^3 / 12`; for a solid round bar of diameter `D`, `A = pi D^2 / 4` and `I = pi D^4 / 64`. The depth enters the inertia as a cube, which is why a deep beam is so much stiffer than a wide one of the same area.

The program computes the properties of a `100 mm` by `200 mm` timber-sized rectangle, bent about its strong axis, builds a portal frame of two columns and a beam from it, and prints what the compiled `Frame` reports. The columns are `3 m` high and the beam spans `4 m`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(4).with_sep(""));

  const double width = 0.100, depth = 0.200;
  const double area = width * depth;
  const double inertia = width * depth * depth * depth / 12.0;
  out.writeln("rectangle: A = ", area, " m^2, I = ", inertia, " m^4");

  // nodes: 0 left base, 1 left top, 2 right top, 3 right base; members: left column, beam, right column
  const st::Mesh portal{grid({0, 0, 0, 3, 4, 3, 4, 0}, 2), {0, 1, 3}, {1, 2, 2}};
  const auto frame = st::frame(portal, {vec({area, area, area}), vec({inertia, inertia, inertia}), vec({200e9, 200e9, 200e9})});
  out.writeln("nodes ", frame.nodes(), ", members ", frame.members(), ", degrees of freedom per node ", frame.dofs());
}
```

Output

```text
rectangle: A = 2.0000e-02 m^2, I = 6.6667e-05 m^4
nodes 4, members 3, degrees of freedom per node 3
```

The inertia is `6.667e-5 m^4`. Turned on its side, so that it bends about its weak axis, the same section has `I = 0.200 * 0.100^3 / 12 = 1.667e-5 m^4`, four times smaller with the same area: always compute `I` from the actual cross-section and about the axis that resists the bending.

## Local axes and member end actions

Each member has its own axes. The **local x axis** runs along the member from its `from` node to its `to` node, and the **local y axis** is the local x axis turned a quarter turn counterclockwise. A horizontal member from left to right has local x pointing right and local y pointing up; a vertical column from its base to its top has local x pointing up and local y pointing left.

A solved frame reports, for each member, six **end actions** in these local axes, `(Ni, Vi, Mi, Nj, Vj, Mj)`: the axial force, the shear force and the moment that the nodes apply to the member at its start `i` and its end `j`. A positive `N` points along local x, a positive `V` along local y, and a positive `M` is counterclockwise. Because these are actions on the member, a member in tension has a negative `Ni` and a positive `Nj`, and a member whose ends are pulled apart by equal and opposite forces is in tension.

| Quantity | Meaning at member end `i` | Meaning at member end `j` |
|---|---|---|
| `N` | force along local x, `Ni = -(tension)` | `Nj = +(tension)` |
| `V` | force along local y applied by the node | force along local y applied by the node |
| `M` | moment applied by the node, counterclockwise positive | moment applied by the node, counterclockwise positive |

The bending moment of the usual beam diagram, positive when the member sags (tension on the underside of a horizontal member left to right), is `-Mi` at the start and `+Mj` at the end. The [next page](beams_and_end_actions.md) uses this with beams whose diagrams are known.

## What the checks reject

`frame` refuses a description that cannot be a frame and says why. The program prints the message for four mistakes: coordinates in three dimensions, a negative inertia, an inertia list of the wrong length and a non-positive density.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <functional>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

static void report(const char *what, const std::function<void()> &build) {
  try {
    build();
    std::cout << what << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << what << ": " << e.what() << "\n";
  }
}

int main() {
  const auto line = grid({0, 0, 1, 0}, 2);
  const auto area = vec({1e-3}), inertia = vec({8e-8}), modulus = vec({200e9});
  report("coordinates in 3D  ", [&] { (void)st::frame({grid({0, 0, 0, 1, 0, 0}, 3), {0}, {1}}, {area, inertia, modulus}); });
  report("negative inertia   ", [&] { (void)st::frame({line, {0}, {1}}, {area, vec({-8e-8}), modulus}); });
  report("wrong inertia count", [&] { (void)st::frame({line, {0}, {1}}, {area, vec({8e-8, 8e-8}), modulus}); });
  report("zero density       ", [&] { (void)st::frame({line, {0}, {1}}, {area, inertia, modulus, vec({0.0})}); });
}
```

Output

```text
coordinates in 3D  : eng::structure::frame: a frame needs at least two nodes and exactly two spatial axes
negative inertia   : eng::structure::frame: area, inertia, and modulus must be positive
wrong inertia count: eng::structure::frame: area, inertia, and modulus must end in the member count
zero density       : eng::structure::frame: density must be positive
```

Every message starts with the function that raised it and names the property that is wrong, so a description error is found at `frame`, before any analysis runs.

## Decisions

- **Beam or truss:** members that must resist bending and joints that transmit moment are a frame; bars that only carry axial force between pins are a truss, with fewer unknowns and no inertia to supply.
- **Where to put nodes:** at every support, joint, load and change of section; a load can only be applied at a node, so a member that must be loaded at its middle is two members.
- **Density:** give it whenever the structure will be used for vibration, buckling-with-mass or time response; a static frame does not need it.

## Pitfalls

The program makes two mistakes whose effect is easy to miss: taking the inertia about the wrong axis of a section, and supporting a beam by a single pin.

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
  const double width = 0.100, depth = 0.200, area = width * depth;
  const double strong = width * depth * depth * depth / 12.0, weak = depth * width * width * width / 12.0;

  // a 2 m cantilever carrying 1 kN at the tip, with the section's two possible inertias
  const auto tip_deflection = [&](double inertia) {
    const auto beam = st::frame({grid({0, 0, 2, 0}, 2), {0}, {1}}, {vec({area}), vec({inertia}), vec({10e9})});
    return 1000.0 * flat(st::solve(beam, grid({0, 0, 0, 0, -1000, 0}, 3), {{0, 0, 0}, {0, 1, 2}, {}}).displacement)[4];
  };
  out.writeln("bending about the strong axis (I = ", strong * 1e6, "e-6 m^4): tip deflection (mm) ", tip_deflection(strong));
  out.writeln("bending about the weak axis   (I = ", weak * 1e6, "e-6 m^4): tip deflection (mm) ", tip_deflection(weak));

  // a beam with a pin at one end and nothing at the other is free to rotate about the pin
  const auto beam = st::frame({grid({0, 0, 2, 0}, 2), {0}, {1}}, {vec({area}), vec({strong}), vec({10e9})});
  try {
    (void)st::solve(beam, grid({0, 0, 0, 0, -1000, 0}, 3), {{0, 0}, {0, 1}, {}});
  } catch (const std::exception &e) {
    std::cout << "a beam on one pin: " << e.what() << "\n";
  }
}
```

Output

```text
bending about the strong axis (I = 66.667e-6 m^4): tip deflection (mm) -4.000
bending about the weak axis   (I = 16.667e-6 m^4): tip deflection (mm) -16.000
a beam on one pin: eng::structure::solve: the structure has a direction with no stiffness under this load, so it is a mechanism; check the supports and the bar connectivity
```

The weak axis is four times less stiff, so the same load bends the beam four times as far: choose the inertia for the axis the load bends about. A beam held by one pin is free to swing about it, and the solver reports it as a mechanism rather than returning a displacement. A member is straight and prismatic; a tapered or curved member is a chain of short straight members of changing section.

Reference: [`Mesh`](../../../../api/eng/structure/model/Mesh.md), [`Sections`](../../../../api/eng/structure/model/Sections.md), [`Frame`](../../../../api/eng/structure/model/FrameModel.md), [`frame`](../../../../api/eng/structure/model/frame.md), [`solve`](../../../../api/eng/structure/statics/solve.md), [`FrameResult`](../../../../api/eng/structure/statics/FrameResult.md).

Where next: [Beams and end actions](beams_and_end_actions.md) solves beams against the textbook formulas and shows how to apply a distributed load.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="beams_and_end_actions.html">
    <span class="label">Next</span>
    <span class="title">Beams and end actions</span>
  </a>
</div>
<!-- apiref:nav:end -->

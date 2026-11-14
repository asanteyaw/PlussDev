#pragma once

#include "pluss/codem/codem.h"
#include "pluss/subsp/eng/circuit.h"

#include <cstdint>
#include <string>
#include <vector>

namespace pluss::subsp::eng::thermal {

// ---------------------------------------------------------------------------------------------------------------------
// Lumped thermal networks. A network is the circuit system with temperature in place of voltage and heat flow in place
// of current, so it is solved by the circuit engine: `steady` is the operating point and `tran` the transient. Nodes are
// numbered from zero; the reference the temperatures are measured from is internal. Temperatures are in any scale that
// is linear in kelvin (kelvin or degrees Celsius) because conduction depends only on differences. Values use the final
// axis for elements; leading axes are parameter-sweep batches.
// ---------------------------------------------------------------------------------------------------------------------

// Thermal conductances [W/K] between node pairs. Heat flows from `a` to `b` when `a` is warmer.
struct Links {
  std::vector<std::int64_t> a;
  std::vector<std::int64_t> b;
  Series conductance{};
};

// Thermal capacities [J/K] of nodes.
struct Stores {
  std::vector<std::int64_t> node;
  Series capacity{};
};

// Nodes held at a prescribed temperature.
struct Fixed {
  std::vector<std::int64_t> node;
  Series temperature{};
};

// Heat inflows [W] into nodes (negative withdraws heat).
struct Heats {
  std::vector<std::int64_t> node;
  Series power{};
};

struct Net {
  std::int64_t nodes{};
  Links links;
  Stores stores;
  Fixed fixed;
  Heats heats;
};

// `temperature` is [..., nodes]; `link_flow` [..., links] is the heat flow from `a` to `b`; `fixed_flow` [..., fixed] is
// the heat the boundary delivers into the network (negative when the boundary absorbs heat). `residual` is the defect of
// the nodal heat balance.
struct Result {
  Series temperature;
  Series link_flow;
  Series fixed_flow;
  Series residual;
};

// Time-varying boundary temperatures and heat inflows, [..., time, elements] along the `tran` time vector. An undefined
// member holds the value stored in the network constant over the run.
struct Drive {
  Series temperature{};
  Series power{};
};

// Temperature of every store at the first time sample, [..., stores]; undefined means zero.
struct Start {
  Series temperature{};
};

// Steady state: capacities are ignored. Every node needs a conduction path to a fixed temperature.
Result steady(const Net &net, const circuit::SolveOptions &options = {});

// Transient on a uniform time grid [s]. Every Result member gains a time axis immediately before its final element axis.
Result tran(const Net &net, const Series &time, const Drive &drive = {}, const Start &start = {},
            circuit::Scheme scheme = circuit::Scheme::Trapezoid);

// ---------------------------------------------------------------------------------------------------------------------
// Conduction and convection resistances [K/W]; inputs broadcast.
// ---------------------------------------------------------------------------------------------------------------------

// Plane wall: thickness / (k area).
Series wall(const Series &conductivity, const Series &thickness, const Series &area);
// Cylindrical shell of length L: ln(r_out / r_in) / (2 pi k L).
Series tube(const Series &conductivity, const Series &r_in, const Series &r_out, const Series &length);
// Spherical shell: (1 / r_in - 1 / r_out) / (4 pi k).
Series shell(const Series &conductivity, const Series &r_in, const Series &r_out);
// Convection film: 1 / (h area).
Series film(const Series &coefficient, const Series &area);
// Biot number h L / k.
Series biot(const Series &coefficient, const Series &conductivity, const Series &length);

// Fin tip condition.
enum class Tip { Adiabatic, Convective, Infinite };

struct FinResult {
  Series heat;
  Series efficiency;
  Series tip;
};

// Straight fin of constant cross-section: `perimeter` [m], `area` [m^2] of the section, `length` [m]. `base` and
// `ambient` are temperatures; `heat` [W] leaves the base, `tip` is the tip temperature, `efficiency` the heat over the
// heat of an isothermal fin at the base temperature.
FinResult fin(const Series &coefficient, const Series &conductivity, const Series &perimeter, const Series &area,
              const Series &length, const Series &base, const Series &ambient, Tip tip = Tip::Adiabatic);

// Layered plane wall of `area` [m^2]. Layer values are [..., layers]; each layer is divided into `cells` equal cells.
struct Layers {
  std::vector<std::int64_t> cells;
  Series thickness{};
  Series conductivity{};
  Series density{};
  Series specific_heat{};
};

// A slab compiled into a network: node 0 is the inner surface, nodes 1..cells are the cell centres and node cells + 1 is
// the outer surface. Surface nodes carry no capacity. Add films, fixed temperatures and heats to `net`.
struct Slab {
  Net net;
  std::int64_t inner{};
  std::int64_t outer{};
  std::int64_t cells{};
};

Slab mesh(const Layers &layers, const Series &area);

// ---------------------------------------------------------------------------------------------------------------------
// Heat exchangers. Capacity rates are C = m cp [W/K]; `cr` is Cmin / Cmax.
// ---------------------------------------------------------------------------------------------------------------------

enum class Flow { Parallel, Counter, Cross, Shell, Boil };

// Effectiveness of an exchanger with `ntu` = UA / Cmin.
Series eff(const Series &ntu, const Series &cr, Flow flow);
// Number of transfer units that gives effectiveness `effectiveness`.
Series ntu(const Series &effectiveness, const Series &cr, Flow flow);

struct Exchange {
  Series duty;
  Series hot_out;
  Series cold_out;
  Series effectiveness;
  Series ntu;
};

// Rates an exchanger of conductance `ua` [W/K].
Exchange rate(const Series &hot_c, const Series &cold_c, const Series &hot_in, const Series &cold_in, const Series &ua,
              Flow flow);

// Log-mean temperature difference of a parallel or counter-flow exchanger from its four terminal temperatures.
Series lmtd(const Series &hot_in, const Series &hot_out, const Series &cold_in, const Series &cold_out, Flow flow);

// ---------------------------------------------------------------------------------------------------------------------
// Moist air. Temperatures [deg C], pressure [Pa], humidity ratio w [kg water / kg dry air], enthalpy [J / kg dry air].
// Reference state: dry air at 0 deg C and saturated liquid water at 0 deg C have zero enthalpy.
// ---------------------------------------------------------------------------------------------------------------------

// Saturation pressure of water vapour over liquid water (0 deg C and above) or ice (below), -60 to 150 deg C.
Series psat(const Series &temperature);
// Humidity ratio from relative humidity (0..1).
Series ratio(const Series &temperature, const Series &relative, const Series &pressure);
// Relative humidity (0..1) from the humidity ratio.
Series rh(const Series &temperature, const Series &w, const Series &pressure);
// Specific enthalpy.
Series enth(const Series &temperature, const Series &w);
// Specific volume [m^3 / kg dry air].
Series vol(const Series &temperature, const Series &w, const Series &pressure);
// Dew-point temperature.
Series dew(const Series &w, const Series &pressure);
// Thermodynamic wet-bulb temperature.
Series wet(const Series &temperature, const Series &w, const Series &pressure);

struct MixState {
  Series temperature;
  Series w;
};

// Adiabatic mixing of two streams of dry-air mass flow m1 and m2.
MixState mix(const Series &t1, const Series &w1, const Series &m1, const Series &t2, const Series &w2, const Series &m2);

// ---------------------------------------------------------------------------------------------------------------------
// Property provider. One place for fluid properties, each correlation with its reference state and source.
// ---------------------------------------------------------------------------------------------------------------------

enum class Fluid { Water, Air };

struct Properties {
  Series density;       // kg/m^3
  Series specific_heat; // J/(kg K)
  Series conductivity;  // W/(m K)
  Series viscosity;     // Pa s
  Series prandtl;
};

// Properties at `temperature` [K] and `pressure` [Pa]. Water is the saturated liquid (pressure does not enter); air is
// an ideal gas at `pressure`. A temperature outside the validity range is an error: nothing is extrapolated.
Properties props(Fluid fluid, const Series &temperature, const Series &pressure = {});

struct Origin {
  std::string name;
  std::string reference;
  std::string state;
  double t_min{};
  double t_max{};
};

// Provenance of the properties of a fluid.
Origin source(Fluid fluid);

} // namespace pluss::subsp::eng::thermal

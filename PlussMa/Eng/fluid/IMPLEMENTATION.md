# Engineering Fluid Networks Implementation

This file records implemented behavior, ownership, verification, and deferred scope for
`pluss::subsp::eng::fluid`. It is an implementation boundary, not an API tutorial. The
generated inventory in `PlussMa/public_calls.md` determines which declarations are public.

## E7a: Steady Pressurized Pipe Networks

### Public contract

E7a exposes two functional entry points:

- `Network fluid::network(const Net&, const Liquid&)` validates and compiles immutable
  node/link topology while retaining differentiable physical Series; and
- `SteadyResult fluid::steady(const Network&, const Options&)` solves junction heads and
  pipe flows as one coupled nonlinear system.

Nodes are either variable-head junctions or fixed-head reservoirs. Junction demand is
positive when volume leaves the network. A pipe's positive flow direction is its declared
`from -> to` direction; negative results are valid and represent reverse flow. Every
connected component must contain a reservoir, so its absolute head is anchored.

Physical values end in a node or pipe axis and may carry arbitrary equal leading scenario
axes. Unbatched fields broadcast across that common scenario grid. The compiled network
preserves `Float32` or `Float64` and the source platform.

### Compiled topology

`network()` constructs three constant sparse-in-meaning maps once:

```text
C[node, pipe] = -1 at from, +1 at to
E[pipe, node] = +1 at from, -1 at to
Pj[junction, node] selects variable-head nodes
```

The present Series backend applies these as dense matrices. This keeps all scenario lanes
batched and differentiable and avoids scalar physical loops, while leaving the compiled
topology boundary ready for a canonical sparse operator when that backend exists. Host
work is restricted to integer connectivity validation and construction of constant maps.

An initial flow satisfying junction continuity is obtained from the minimum-norm solve

```text
q0 = Cj^T (Cj Cj^T)^-1 d,   Cj = Pj C.
```

The connected-component reservoir check makes `Cj` full row rank, including looped and
parallel-pipe networks.

### Coupled hydraulic solve

For unknown junction head `hj` and pipe flow `q`, each Newton residual contains pipe energy
and junction continuity:

```text
r_energy  = E h - loss(q)
r_balance = Cj q - demand

J = [ E Pj^T   -diag(d loss / d q) ]
    [    0                 Cj       ]
```

The whole trailing block system is solved with the canonical batched Series LU path. The
Newton updates, constitutive equations, linear solves, and result projections remain on the
Series graph. Consequently head, pressure, flow, velocity, friction factor, continuity
balance, and energy residual are returned for every scenario without extracting physical
lanes to host containers.

On CPU the canonical LU owner factors through its Float64 work path. Each solution is cast
back to the network dtype before the next hydraulic operator application, matching the
existing Engineering Newton convention: Float32 model state remains Float32 while the
factorization receives the backend's numerically safer work precision. The cast remains on
the reverse graph.

`Options::head_tolerance` applies to pipe energy residuals in metres;
`flow_tolerance` applies to junction continuity in cubic metres per second. The gradient
floor regularizes zero-flow links for a solvable Newton Jacobian; it does not replace the
reported constitutive head loss.

### Pipe laws

`LossModel::DarcyWeisbach` uses

```text
h = (f L/D + K) v |v| / (2g),   v = 4q/(pi D^2).
```

The Darcy factor follows EPANET 2.2's hydraulic coefficient kernel:

- `f = 64/Re` at `Re <= 2000`;
- the Dunlop cubic interpolation over `2000 < Re < 4000`; and
- the Swamee--Jain explicit turbulent relation at `Re >= 4000`.

The implementation carries EPANET's analytical `df/d|q|` into the Newton gradient. This is
important near the transition: treating the friction factor as constant gives a different
Jacobian and materially poorer convergence.

`LossModel::HazenWilliams` uses SI coefficient `10.67`, exponent `1.852`, and diameter
exponent `4.8704`. Its roughness lane is the positive Hazen--Williams `C` coefficient rather
than an absolute roughness length. The result's Darcy friction-factor field is therefore
zero under this model instead of implying an equivalent Darcy factor.

### Validation and failures

Construction rejects malformed endpoint arrays, out-of-range or self-referential pipes,
reservoir-free connected components, absent junction/reservoir roles, non-floating or
non-finite physical fields, non-positive lengths or diameters, negative roughness or minor
loss, nonzero reservoir demand, mixed dtype/platform values, and incompatible scenario
axes. `steady()` additionally requires positive Hazen--Williams coefficients and positive
solver controls.

Nonconvergence raises by default. Setting `fail_on_nonconvergence=false` returns the latest
iterate and a scenario-shaped Boolean `converged` field, allowing callers to inspect exact
residuals without accepting a silent success.

### Verification

`Eng/fluid/test/fluid_test.cpp` covers:

- an independently reconstructed one-pipe Darcy--Weisbach solution;
- Hazen--Williams head loss;
- laminar and transition Reynolds regimes;
- a looped three-pipe network with energy and continuity invariants;
- simultaneous demand and geometry scenarios;
- `Float32` retention with scale-appropriate tolerances;
- reverse sensitivity of solved head to demand; and
- invalid topology, missing reference head, zero diameter, and invalid Hazen roughness.

### References inspected

- EPA/OWA EPANET 2.2 commit `598ee6cd214de07a0d62e7dafc1bdbf09173645a`,
  `SRC_engines/src/solver/hydcoeffs.c` (file header last updated 2019-10-04),
  inspected 2026-10-25,
  especially `linkcoeffs`, `nodecoeffs`, `pipecoeff`, `DWpipecoeff`, and
  `frictionFactor`, for Darcy/Hazen coefficients, Reynolds partition, Dunlop transition,
  Swamee--Jain factor, and analytical hydraulic gradients;
- EPANET 2.2 User Manual, chapters 3 and 12, inspected 2026-10-25, for node/link
  conventions, fixed-grade nodes, continuity, energy equations, and the gradient method;
- E. Todini and S. Pilati, “A Gradient Algorithm for the Analysis of Pipe Networks,” in
  *Computer Applications in Water Supply*, 1988, inspected 2026-10-25, for the coupled
  network formulation and pressure-head solve architecture; and
- the existing PlussMa power-network topology compiler and batched Series linear-system
  paths, inspected 2026-10-25, for reusable validation, scenario-axis, and platform
  conventions. No source was copied verbatim.

### Deliberate next scope

E7a is the steady, incompressible, pressurized pipe foundation. Emitters,
pressure-dependent demand, tanks, demand patterns, event controls, extended-period
simulation, water age/constituent transport, sparse factorization, and thermal-fluid
coupling remain explicit later E7 contracts. They are not approximated as pipes here.

## E7b: Pumps And Control Valves

E7b extends the same compiled graph rather than introducing a second solver. Link ordering
is deterministic—pipes, pumps, then valves—and every result link axis follows that order.
`Network::pipes()`, `pumps()`, `valves()`, and `links()` expose the family counts.

Variable-speed pumps use the positive head-gain curve

```text
gain(q, s) = s^2 H0 - R s^(2-n) q |q|^(n-1)
```

stored in the common energy convention as negative head loss. This is EPANET's power pump
curve and affinity-law speed transformation, with an analytical flow gradient. Batched
speed and curve sweeps remain Series operations and retain reverse sensitivity. A final
reverse-flow requirement is rejected because E7b models an active one-way pump rather than
silently extending its curve into turbine operation.

| Valve kind | Coupled equation |
| --- | --- |
| `Throttle` | `h_from - h_to = K v|v|/(2g)` |
| `Flow` | `q = setting` |
| `Reduce` | `h_to = elevation_to + setting` |
| `Sustain` | `h_from = elevation_from + setting` |

The latter three replace that link's ordinary energy row while continuity remains in the
same square Newton system. Pressure valves must regulate a junction. Infeasible active
controls fail through nonconvergence or reverse-flow validation. E7b exposes the active
hydraulic equation, not EPANET's history-dependent OPEN/CLOSED/ACTIVE state machine;
hysteretic switching belongs with operational controls and extended-period state.

An active flow or pressure valve cannot raise the head across it: a solution in which its head loss is
below `-100 * head_tolerance` is refused with "an active control valve cannot add head", in the same way
as a reversed flow. Without this check, a sustaining setting below the natural upstream pressure, or a flow
setting above the flow the head difference can pass, returned a solution in which the valve acted as a pump.

Permanent verification adds a pump-only network, affinity-law speed sweep, exact pump-speed
reverse derivative, independent minor-loss valve result, active reducing- and
sustaining-valve pressures, and a fixed-flow valve embedded in a looped network. It also
checks Float32 pump state/output preservation and rejects reverse flow through an active
one-way pump instead of evaluating the pump curve outside its declared operating contract.

### E7b references inspected

- EPA/OWA EPANET 2.2 commit `598ee6cd214de07a0d62e7dafc1bdbf09173645a`,
  `SRC_engines/src/solver/hydcoeffs.c`: `pumpcoeff`, `curvecoeff`, `valvecoeff`,
  `tcvcoeff`, `fcvcoeff`, `prvcoeff`, and `psvcoeff`;
- the same revision's `SRC_engines/src/solver/hydstatus.c`: `pumpstatus`, `cvstatus`,
  `fcvstatus`, `prvstatus`, and `psvstatus`, used to separate continuous active equations
  implemented here from discrete switching intentionally deferred;
- the same revision's `User_Manual/docs/3_network_model.rst`, sections “Pumps” and
  “Valves,” for affinity laws, device meanings, settings, and output conventions; and
- the same revision's `User_Manual/docs/12_analysis_algorithms.rst`, for hydraulic-analysis
  equations and pressure/flow control treatment. All were inspected 2026-10-25. No source
  code was copied verbatim.

## E7c: Storage, Demand Policy, And Extended-Period Operation

### One hydraulic owner

E7c does not introduce a separate transient network solver. `fluid::run()` advances the
event clock and tank state, then invokes the same coupled `steady()` owner at each
hydraulic event through a cheap Network snapshot. Compiled incidence, selection, device,
and control maps are reused. The snapshot changes only time-varying Series: demand,
fixed-grade tank head, pump speed, valve setting, and the scenario-wise closed-link mask.
Each accepted solve supplies detached junction-head and link-flow warm starts to the next
event. Physical state dependence still enters through the tracked tank level and scheduled
parameters; the numerical guess itself is not added as a second reverse path.

The event clock is necessarily sequential because storage at time `t + dt` depends on the
flow solved at `t`. Physical lanes are not scalarized: all scenario axes, Newton systems,
demand laws, tank balances, control masks, result fields, and reverse graphs remain batched
Series operations. Host work is restricted to immutable topology, control records, and the
single next-event time shared by the batch.

### Pressure-driven use and emitters

`DemandMode::Fixed` retains the E7a demand-driven equation. Under
`DemandMode::Pressure`, positive junction demand follows the Wagner law

```text
d(p) = 0                                             p <= p_min
d(p) = D ((p - p_min)/(p_full - p_min))^exponent    p_min < p < p_full
d(p) = D                                             p >= p_full
```

where `p = head - elevation` is pressure head in metres. A non-positive base demand is an
injection and remains fixed; pressure throttling is not applied to a source. The analytical
interior derivative `dd/dp` is assembled into the lower-left Newton block instead of
freezing demand during an iteration.

The Newton step is relaxed lane by lane: a lane whose full step does not reduce its weighted residual
norm (energy residual in metres plus continuity residual scaled by `head_tolerance / flow_tolerance`) takes a
halved step, up to eight times. The factor is a detached decision, so an accepted full step keeps its reverse
graph unchanged. Without it, a pressure-driven demand far above what the pipes can deliver made the full
step cycle between two distant heads and the solve never converged, where the gradient-method reference
converges with its step-size control.

A junction emitter contributes

```text
q_emit = coefficient * max(p, 0)^emitter_exponent
```

with its analytical pressure derivative in the same continuity row. Ordinary delivered
demand and emitter discharge are returned separately. Reservoir and tank lanes are
projected out structurally, so their required zero inputs do not acquire spurious reverse
sensitivity.

### Tanks and conservation

A `NodeKind::Tank` is a dynamic fixed-grade node. Its initial water-surface head comes from
`Nodes::head`; bottom elevation comes from `Nodes::elevation`; area and level limits follow
the compact tank axis in `Tanks`. During one hydraulic interval,

```text
q_tank = C_tank q
level(t + dt) = level(t) + dt * (q_tank - spill) / area.
```

The clock shortens exactly to the earliest minimum level, maximum level, pattern boundary,
timed action, or tank-level control crossing. A non-overflowing tank temporarily closes
only incident links whose solved direction would push it farther beyond a bound; this mask
is recomputed at the next event, so a changed hydraulic gradient can reopen the path. An
overflowing tank remains at its maximum level and reports the excess conserved discharge
through `RunResult::spill` rather than silently discarding volume.

E7c uses constant-area cylindrical storage. Arbitrary level-volume curves remain a later
contract and are not approximated by an average area.

### Patterns and controls

`Schedule` accepts cyclic two-dimensional patterns:

| Field | Shape | Meaning |
| --- | --- | --- |
| `demand` | `[period, node]` | multiplier on compiled base demand |
| `pump` | `[period, pump]` | multiplier on compiled relative speed |
| `valve` | `[period, valve]` | absolute operating setting: throttle `K`, flow target, or pressure-head target |

Pattern rows change only at exact `pattern_step` boundaries. `TimeControl` applies an
action at an exact elapsed second. `StateControl` observes tank level for tank nodes and
pressure head for all other nodes. `Open`, `Close`, and `Set` actions are persistent;
`Open` clears a latched setting and returns the device to its current pattern baseline,
while `Set` opens the device and latches the supplied pump speed, throttle loss
coefficient, flow target, or pressure-head target according to device kind.

Two state controls with separated `Below` and `Above` thresholds form an explicit
hysteresis band. Tank-threshold crossings are localized continuously from the current
storage rate. Junction-pressure predicates are algebraic and are resolved at hydraulic
events; control actions and tank-limit guards are re-solved until their status is stable,
with a hard failure instead of silent oscillation after sixteen status passes.

### Result and failure contract

`RunResult` stacks every accepted event state on a leading sample axis. It includes time,
all steady hydraulic fields, delivered demand, emitter discharge, tank level, spill,
open/closed link state, convergence, and Newton iteration count. Samples include `t=0`,
every shortened event, and the requested final duration.

Construction or execution rejects malformed tank metadata, initial levels outside bounds,
nonzero demand or emitters on fixed-grade nodes, incompatible pattern widths, negative
pattern values, controls outside the time/device/node domain, invalid settings, and an
event clock that cannot advance. Existing nonconvergence and reverse-flow failures remain
in force at every hydraulic state.

### Permanent verification

The focused suite covers the Wagner equation reconstructed from solved pressure, emitter
flow reconstructed from solved pressure, cyclic demand patterns, exact cylindrical tank
mass balance, reverse sensitivity of final level to demand, timed pump-speed changes,
scheduled throttle-loss changes, localized tank-level pump shutdown, batch-lane storage,
Float32 retention, active overflow and spill, non-overflowing bound closure, malformed
patterns, and invalid tank bounds. Existing E7a/E7b cases run through the same target to
guard the shared steady owner.

### E7c references inspected

- EPA/OWA EPANET 2.2 commit `598ee6cd214de07a0d62e7dafc1bdbf09173645a`,
  `SRC_engines/src/solver/hydcoeffs.c`: `emittercoeffs`, `emitterheadloss`,
  `demandcoeffs`, and `demandheadloss`, for pressure-dependent continuity terms and
  analytical hydraulic gradients;
- the same revision's `SRC_engines/src/solver/hydraul.c`: `demands`, `controls`,
  `timestep`, `tanktimestep`, `controltimestep`, and `tanklevels`, for pattern cycling,
  persistent device state, event shortening, and storage advancement;
- the same revision's `User_Manual/docs/3_network_model.rst`, for tank, pattern, emitter,
  pressure-demand, and simple-control meanings;
- the same revision's `User_Manual/docs/8_analyzing_network.rst`, for SI hydraulic option
  contracts and pressure-demand parameters; and
- the same revision's `User_Manual/docs/12_analysis_algorithms.rst`, especially “Pressure
  Dependent Demand Model” and “Extended Period Simulation,” for the Wagner equation and
  solve/advance sequence. All were inspected 2026-10-25. The implementation adapts the
  published equations to batched differentiable Series operations; no source was copied.

## E7d: Constituent Transport And Water Age

### One hydraulic clock, one transport owner

`fluid::transport()` composes with E7c instead of solving hydraulics again at each quality
substep. It first obtains the accepted hydraulic event trajectory from `fluid::run()`, then
holds each event's flow, delivered demand, emitter discharge, and spill constant while it
advances quality to the next hydraulic event. The quality clock may take several smaller
steps inside one hydraulic interval, but it never crosses an event at which a pattern,
control, tank bound, or solved flow changes.

The public `Quality` record describes the transported quantity. `Constituent` transports a
nonnegative concentration-like scalar. `Age` is the same conservation equation with zero
quality at reservoir boundaries and a unit source of one second of age per elapsed second.
Age therefore does not accept user source or reaction fields: combining those meanings
would produce a quantity that is no longer water age.

`RouteOptions` separates transport resolution from hydraulic resolution and is
shared by constituent, age, and thermal routing:

| Field | Meaning | Contract |
| --- | --- | --- |
| `step` | maximum quality step, seconds | finite and positive |
| `courant` | maximum transported fraction of a pipe cell or mixed tank per step | `(0, 1]` |
| `flow_tolerance` | flow magnitude treated as stagnant | finite and nonnegative |
| `cells` | equal-volume finite-volume cells per physical pipe | positive integer |

The implementation owns time stepping and immutable topology on the host. Concentrations,
reaction rates, volumes, fluxes, junction systems, scenario axes, and reverse history stay
in `Series`. A physical batch lane is never extracted into a host scalar loop.

### Pipe transport and flow reversal

Every physical pipe is represented by `cells` equal-volume control volumes. For pipe
volume `V`, flow magnitude `|q|`, and quality step `dt`, the local Courant number is

```text
lambda = |q| dt / (V / cells).
```

The step is shortened globally until every active pipe and draining tank satisfies
`lambda <= courant`. A first-order conservative upwind update then moves quality in the
solved flow direction:

```text
c_i(t + dt) = c_i(t) + lambda (c_upwind - c_i(t)).
```

A negative solved flow reverses both the selected boundary node and cell traversal; it is
not clipped or reinterpreted as a forward magnitude. Pumps and valves have zero quality
storage. They transmit their upstream quality algebraically and can participate in chains
and recirculating junction systems without acquiring fictitious device volume.

This fixed-cell formulation is deliberately different from EPANET's variable-length
Lagrangian segment lists. It preserves fixed tensor shapes, batching, platform placement,
and reverse differentiation. It also has the known first-order upwind trade-off:
concentration fronts diffuse numerically when the Courant number is below one. Because the
quality clock is shared by all batch lanes, the fastest lane can shorten the step and thus
increase numerical diffusion in a slower lane. Batch lanes remain physically independent,
but changing the batch can change that discretization error. Callers needing sharper fronts
should raise `cells`, use a suitable Courant limit, or run scenarios separately.

### Junctions, boundaries, and zero-volume devices

Junction quality is the complete-mixing solution. Pipe inflow, external negative demand
(injection), and zero-volume device transfer contribute mass; outgoing flow and positive
demand remove the mixed quality. Pumps and valves form algebraic transfer coefficients in
the same batched linear system, so a zero-volume cycle is solved simultaneously instead of
being routed according to declaration order.

Reservoir nodes retain `Quality::source` at every quality step. If a constituent source is
omitted, its value defaults to the corresponding lane of `initial`; this makes a fixed
reservoir concentration concise without weakening the explicit source override. Positive
ordinary demand, emitters, and reservoir backflow export the current mixed quality.

For an inactive dry junction with no inflow or injection, the previous node value is
retained. This avoids a singular zero-flow mixing row without inventing concentration.

### Reactions and fully mixed tanks

`pipe_rate` and `tank_rate` are first-order rates in inverse seconds. Each reaction stage
uses the exact constant-rate update

```text
c(t + dt) = c(t) exp(k dt).
```

Negative rates decay, positive rates grow, and zero preserves the transported amount.
Pipe rates cover physical pipes only; zero-volume pumps and valves cannot react because
they have no residence time in this contract.

A tank is one completely mixed volume. Outflow during a step removes water at the tank's
pre-mixing quality; incoming mass is then mixed with the remaining inventory, and the
hydraulic tank volume is advanced from the same accepted flow and spill. The Courant guard
also prevents a step from draining more mixed tank volume than is available. Reservoirs
remain prescribed boundaries and carry no stored quality inventory.

### Results and conservation diagnostic

`TransportResult` contains the hydraulic trajectory used by transport and quality sampled
at the same hydraulic event times:

| Field | Shape | Meaning |
| --- | --- | --- |
| `hydraulic` | `RunResult` | exact E7c states that drove transport |
| `node` | `[sample, ..., node]` | mixed junction, reservoir, and tank quality |
| `link` | `[sample, ..., link]` | volume mean for pipes; upstream value for zero-volume devices |
| `tank` | `[sample, ..., tank]` | completely mixed tank quality |
| `mass` | `[sample, ...]` | stored pipe and tank quality-volume inventory |
| `mass_error` | `[sample, ...]` | inventory change minus cumulative reaction and boundary fluxes |

`mass_error` is a signed diagnostic in concentration-volume units. For a nonreacting
closed accounting domain it should remain near zero to the precision implied by dtype and
solver tolerances. For reacting cases the exact exponential inventory change is included
in the reaction ledger; sources, withdrawals, reservoir exchange, emitters, and tank spill
are included in the boundary ledger.

### Validation and permanent verification

The boundary rejects invalid clock controls, missing constituent initial quality,
non-floating or non-finite fields, negative initial/source values, mismatched dtype,
platform, width, or non-scalar batch axes, and reaction/source fields supplied to Age.
Hydraulic topology and schedules retain all E7a--E7c validation.

The permanent E7d suite covers a Courant-one four-cell front, complete mixing of two
reservoir tributaries, exact first-order decay, age growth and reservoir reset, fully mixed
tank decay, zero-volume pump transfer, declared reverse flow, batched speed scenarios,
Float32 retention, mass conservation, and reverse sensitivity to source quality. Failure
tests cover invalid cell counts, missing constituent initial state, and illegal Age
reaction input. All earlier E7 hydraulic cases execute in the same target.

### Deliberate limits

E7d transports one constituent per call. It does not yet expose EPANET-MSX multi-species
reaction systems, wall/mass-transfer reactions, concentration/flow-paced/mass source
classes, non-mixing tank models (`2COMP`, FIFO, or LIFO), arbitrary reaction callbacks,
high-resolution TVD/WENO advection, or dynamic Lagrangian segments. Those capabilities
require explicit public contracts and validation; none is silently approximated here.

### E7d references inspected

- EPA/OWA EPANET 2.2 commit `598ee6cd214de07a0d62e7dafc1bdbf09173645a`,
  `SRC_engines/src/solver/qualroute.c`, especially `transport`, `evalnodeinflow`,
  `findnodequal`, `evalnodeoutflow`, `initsegs`, and `reversesegs`, for conservative
  routing, complete junction mixing, zero-volume links, tanks, and flow reversal;
- the same revision's `User_Manual/docs/12_analysis_algorithms.rst`, “Water Quality
  Analysis,” for the advection/reaction balance, complete mixing, boundary mass accounting,
  tank models, and Lagrangian reference architecture;
- the same revision's `User_Manual/docs/3_network_model.rst`, “Water Quality Model,” for
  constituent, source, reaction, water-age, reservoir, and tank semantics; and
- the same revision's `User_Manual/docs/8_analyzing_network.rst`, for quality mode,
  timestep, and tolerance options. All were inspected 2026-10-25. The equations and
  invariants were adapted to a fixed-shape batched Series finite-volume owner; no reference
  source was copied verbatim.

## E7e: Thermal-Fluid Routing

### One route engine, two physical laws

`fluid::heat()` is a thermal specialization of the E7d route engine, not a second network
simulator. It consumes the accepted `fluid::run()` trajectory and therefore inherits the
same event boundaries, solved flow direction, finite-volume cells, algebraic pump and valve
transfer, complete junction mixing, constant-area tank volume, batching, dtype, platform,
and reverse graph. `RouteOptions` is intentionally shared by `transport()` and `heat()` so
Courant limits and cell resolution cannot acquire divergent meanings.

The `Heat` record supplies absolute temperatures and constant-property sensible-heat data:

| Field | Shape or type | Meaning | Default |
| --- | --- | --- | --- |
| `initial` | `[..., node]` | initial node and implied pipe/tank temperature, K | required |
| `source` | `[..., node]` | fixed reservoir and negative-demand injection temperature, K | `initial` |
| `ambient` | `[..., pipe]` | surrounding temperature for each physical pipe, K | required where `ua > 0` |
| `ua` | `[..., pipe]` | total pipe-to-ambient conductance, W/K | zero |
| `tank_ambient` | `[..., tank]` | surrounding temperature for each mixed tank, K | required where `tank_ua > 0` |
| `tank_ua` | `[..., tank]` | total tank-to-ambient conductance, W/K | zero |
| `cp` | scalar | constant fluid specific heat, J/(kg K) | `4180` |

Density comes from the compiled `Network::liquid()`. `cp` is a call-level constant while
temperatures and conductances may carry every network batch axis. This contract is narrow
on purpose: temperature-dependent properties belong in a future property-law interface,
not in an undocumented host callback that would break batching and differentiation.

### Exact local heat exchange

For pipe volume `V`, total conductance `UA`, ambient temperature `Ta`, density `rho`, and
specific heat `cp`, the thermal reaction stage solves

```text
dT/dt = -UA (T - Ta) / (rho cp V)
```

with the exact constant-coefficient update

```text
T(t + dt) = Ta + (T(t) - Ta) exp[-UA dt / (rho cp V)].
```

Although a pipe is split into equal finite volumes for advection, dividing both its volume
and conductance among those cells leaves the same rate coefficient. The implementation can
therefore apply one batched pipe rate without constructing per-cell host records. A tank
uses the identical equation with its current hydraulic volume. Its coefficient is rebuilt
from that volume at every route substep before the exact exponential is applied.

Advection and heat exchange are operator-split at the route step. At Courant one, a plug's
computed outlet temperature matches the analytical residence-time expression. At smaller
Courant numbers the existing first-order upwind diffusion remains; reducing `step` improves
splitting error, while increasing `cells` improves spatial resolution. Thermal routing does
not alter hydraulic density or viscosity during a call, so it is one-way coupled in E7e.

### Mixing, boundaries, and devices

Junction temperature is the mass-flow-weighted sensible-energy mixture. E7e assumes one
fluid with common `rho` and `cp`, so those factors cancel and the canonical volume-flow
mixing system from E7d is exact. Reservoirs hold `source`; if omitted, they hold `initial`.
Negative node demand injects fluid at `source`, while positive demand, emitters, reservoir
backflow, and tank spill export the current mixed temperature.

Pumps and valves remain zero-volume algebraic devices. Their outlet temperature equals the
upstream mixed temperature: E7e does not silently add pump inefficiency heat, motor heat, or
valve flashing. Those effects require explicit device-energy records in a later tranche.
Solved negative flow reverses both the thermal boundary and finite-volume stencil.

### Results and energy ledger

`HeatResult` returns temperatures at hydraulic event times and converts the common route
inventory to joules:

| Field | Shape | Meaning |
| --- | --- | --- |
| `hydraulic` | `RunResult` | hydraulic states that drove the thermal route |
| `node` | `[sample, ..., node]` | mixed node temperature, K |
| `link` | `[sample, ..., link]` | pipe volume mean or zero-volume upstream temperature, K |
| `tank` | `[sample, ..., tank]` | mixed tank temperature, K |
| `energy` | `[sample, ...]` | stored `rho cp sum(V T)`, J, relative to the zero-K datum |
| `energy_error` | `[sample, ...]` | storage change minus ambient exchange and boundary flux, J |

The absolute datum does not affect heat-transfer rates, but it makes boundary and storage
accounting unambiguous. Ambient exchange is measured from the exact exponential inventory
change, not approximated as `UA (T-Ta) dt`. A closed nonreacting domain therefore has nearly
zero `energy_error`; a cooled domain also closes after its ambient term is reconciled.

### Validation and permanent verification

The boundary rejects absent initial temperature, negative or non-finite absolute
temperatures, nonpositive/non-finite `cp`, negative/non-finite conductance, a positive
conductance without its ambient temperature, shape-width errors, and dtype/platform or
batch mismatch. The common route clock retains its positive-step, `(0, 1]` Courant,
nonnegative flow-tolerance, and positive-cell contracts.

The permanent E7e suite covers an analytical stagnant-pipe cooldown, a Courant-one moving
plug with analytical residence-time heat loss, equal-flow thermal mixing, analytical tank
cooldown, scenario batching, Float32 retention, reverse sensitivities to source temperature
and `UA`, energy closure, and stable invalid-property failures. It runs in the same target
as every E7a--E7d hydraulic and constituent regression.

### Deliberate limits

E7e is a differentiable, constant-property, one-way-coupled network thermal model. It does
not yet include fluid/wall dual capacitance, radial multilayer insulation records,
temperature-dependent EOS/transport properties, hydraulic feedback, pump heat, phase
change, compressibility, radiation, ground-temperature fields, or heat-exchanger
effectiveness models. In particular, `ua` represents the complete steady resistance from
bulk fluid to the prescribed ambient; it must not be interpreted as only the internal film
coefficient when wall capacitance matters.

### E7e references inspected

- EnergyPlus 26.2 Engineering Reference, “Pipes,” including the Hanby fluid/wall energy
  balances, capacitance, insulation resistance, and finite-volume time delay;
- EnergyPlus Engineering Reference, “Plant Temperature Source Component,” for prescribed
  thermal boundaries;
- EnergyPlus Engineering Reference, “Plant Loop Heat Pump,” for the sensible-energy
  relation between heat rate, mass flow, `cp`, and outlet temperature;
- CoolProp Water documentation and the IAPWS-95 references it identifies, used to bound the
  future variable-property contract rather than to introduce an unverified property fit;
  and
- EPA/OWA EPANET 2.2 `qualroute.c`, retained as the routing, mixing, reversal, tank, and
  boundary-accounting reference inherited from E7d.

These references were inspected 2026-10-29. Their equations and invariants were adapted to
the existing batched Series route owner; no implementation source was copied verbatim. A
search of the locally installed MATLAB R2025b toolbox did not reveal a visible source
implementation with a stronger reusable thermal-pipe contract, and no protected code was
decompiled or inferred.

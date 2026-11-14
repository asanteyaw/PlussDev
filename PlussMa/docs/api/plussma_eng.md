<!-- generated from plussma_eng.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Engineering API Reference

The engineering framework is available beneath `pluss::subsp::eng`. Its public families model engineered systems and signals while delegating array execution, differentiation, linear algebra, optimization and transforms to their lower numerical owners.

```cpp
#include <pluss/ma.h>

namespace sig = pluss::subsp::eng::signal;
namespace lti = pluss::subsp::eng::lti;
namespace control = pluss::subsp::eng::control;
namespace structure = pluss::subsp::eng::structure;
namespace fluid = pluss::subsp::eng::fluid;
namespace robot = pluss::subsp::eng::robot;
namespace comms = pluss::subsp::eng::comms;
namespace thermal = pluss::subsp::eng::thermal;
```

## Families

| Family | Concern |
|---|---|
| `signal` | Windows, convolution, stateful filtering, frequency response and IIR design |
| `lti` | State-space and pole-zero models, system response, Gramians and simulation |
| `control` | Riccati equations, LQR/LQE, loop analysis, pole placement, PID and MPC |
| `circuit` | Compiled electrical networks with diodes and transistors: operating point, AC and transient analysis |
| `power` | AC/DC power flow, sensitivities, contingencies, estimation, continuation and dispatch |
| `fluid` | Hydraulic networks, controls, transport and heat propagation |
| `robot` | Rigid-body robots: kinematics, Jacobians, inverse kinematics and dynamics |
| `comms` | Digital modulation, pulse shaping, noise, detection and error metrics |
| `thermal` | Thermal networks, conduction and fins, heat exchangers, moist air and fluid properties |
| `structure` | Truss/frame assembly, static, modal, buckling, motion and nonlinear response |

The reference currently expands `signal`, `lti`, `control`, `circuit`, `structure`, `power`, `fluid`, `robot`, `comms` and `thermal` completely. Signal pages cover coefficient generation, filtering and IIR design; LTI pages cover state-space analysis, pole-zero systems, frequency response and simulation; Control pages cover feedback synthesis, estimation, stability analysis, controller construction, and predictive control; Structure pages cover model description, static and large-displacement solves, modes and buckling, and time and frequency response. The remaining families retain their public contracts and will be expanded in the same hierarchy rather than compressed into a superficial omnibus page.

## Design Boundary

Engineering functions accept and return `Series` so batch axes, dtype, platform and reverse sensitivity remain available. Small host loops are reserved for structural recurrences such as filter order or state propagation; numerical lanes remain vectorized through the shared Series and linear-algebra kernels.

```{toctree}
:maxdepth: 3
:hidden:

eng/signal
eng/lti
eng/control
eng/circuit
eng/structure
eng/power
eng/fluid
eng/robot
eng/comms
eng/thermal
```

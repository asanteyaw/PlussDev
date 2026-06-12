# PlussMa Componentization

This file defines the include and build layering for PlussMa so users can opt into only what they need.

## Include Layers

1. Full umbrella
- `#include <pluss/ma.h>`
- Pulls Math + Stats + Eng + NatSci umbrellas.

2. Subspace umbrellas
- `#include <pluss/ma/math.h>`
- `#include <pluss/ma/stats.h>`
- `#include <pluss/ma/eng.h>`
- `#include <pluss/ma/natsci.h>`

3. Module-level headers
- Math:
  - `#include <pluss/ma/math/optimization.h>`
  - `#include <pluss/ma/math/linearalg.h>`
  - `#include <pluss/ma/math/finance.h>`
- Stats:
  - `#include <pluss/ma/stats/core.h>`
  - `#include <pluss/ma/stats/distributions.h>`
  - `#include <pluss/ma/stats/time_series.h>`
- Engineering:
  - `#include <pluss/ma/eng/control.h>`
  - `#include <pluss/ma/eng/signal_proc.h>`
- Natural Sciences:
  - `#include <pluss/ma/natsci/physics.h>`
  - `#include <pluss/ma/natsci/chemistry.h>`
  - `#include <pluss/ma/natsci/biology.h>`
  - `#include <pluss/ma/natsci/common.h>`

Prefer module-level headers when compile-time and dependency scope matter.

Notes:
- `Eng` and `NatSci` headers currently provide stable namespace entry points while those subspaces are still expanding.
- As concrete public APIs are added, module headers in those subspaces should be updated to include them directly.

## CMake Build Switches

Subspace switches:
- `PLUSSMA_ENABLE_MATH`
- `PLUSSMA_ENABLE_STATS`
- `PLUSSMA_ENABLE_ENG`
- `PLUSSMA_ENABLE_NATSCI`
- `PLUSSMA_ENABLE_EXAMPLES`

Math module switches:
- `PLUSSMA_MATH_ENABLE_ALGEBRA`
- `PLUSSMA_MATH_ENABLE_ANALYSIS`
- `PLUSSMA_MATH_ENABLE_ECONOMICS`
- `PLUSSMA_MATH_ENABLE_FINANCE`
- `PLUSSMA_MATH_ENABLE_NUMERICAL`
- `PLUSSMA_MATH_ENABLE_PROBABILITY`

Math::Numerical switches:
- `PLUSSMA_MATH_NUMERICAL_ENABLE_INTERPOLATION`
- `PLUSSMA_MATH_NUMERICAL_ENABLE_LINEAR_ALGEBRA`
- `PLUSSMA_MATH_NUMERICAL_ENABLE_OPTIMIZATION`
- `PLUSSMA_MATH_NUMERICAL_ENABLE_SPECTRAL_METHODS`

Math::Finance switches:
- `PLUSSMA_MATH_FINANCE_ENABLE_FILTERING`
- `PLUSSMA_MATH_FINANCE_ENABLE_STOCHASTIC_PROCESSES`

Stats module switches:
- `PLUSSMA_STATS_ENABLE_BAYESIAN`
- `PLUSSMA_STATS_ENABLE_DISTRIBUTIONS`
- `PLUSSMA_STATS_ENABLE_ESTIMATION`
- `PLUSSMA_STATS_ENABLE_INFERENCE`
- `PLUSSMA_STATS_ENABLE_NONPARAMETRIC`
- `PLUSSMA_STATS_ENABLE_REGRESSION`
- `PLUSSMA_STATS_ENABLE_TIME_SERIES`

Engineering module switches:
- `PLUSSMA_ENG_ENABLE_CONTROL`
- `PLUSSMA_ENG_ENABLE_SIGNAL_PROC`

Natural Sciences module switches:
- `PLUSSMA_NATSCI_ENABLE_PHYSICS`
- `PLUSSMA_NATSCI_ENABLE_CHEMISTRY`
- `PLUSSMA_NATSCI_ENABLE_BIOLOGY`
- `PLUSSMA_NATSCI_ENABLE_COMMON`

## Dependency Guardrails

- `Stats::time_series` requires `Stats::distributions`.
- `Math::Finance::calibration` requires `Stats::time_series`.

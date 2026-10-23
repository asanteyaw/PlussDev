# PlussMa Reference Implementation Map

## Purpose

This map identifies the primary implementation, documentation, and publication sources
to inspect before changing a PlussMa family. References are correctness oracles, not code
donors. PlussMa implementations must remain original, Series-native, compatible with the
Pluss type and error systems, and independently verified against published formulas or
executable reference outputs.

Local MATLAB R2025b sources may be inspected anywhere below
`/Applications/MATLAB_R2025b.app/toolbox`. Visible `.m` and coder sources can inform
contracts and algorithms. Protected `.p` files and compiled MEX modules are black-box
behavioral references only: they are not decompiled and must not be described as inspected
source.

| PlussMa area | Primary reference | Secondary reference | What to study |
| --- | --- | --- | --- |
| Numerical fundamentals | [SciPy](https://scipy.org/), [Boost.Math](https://www.boost.org/library/master/math/) | [GSL](https://www.gnu.org/software/gsl/doc/html/intro.html), MATLAB | Signatures, tolerances, special cases, error handling |
| ODE/DAE and sensitivities | [SUNDIALS](https://github.com/LLNL/sundials) | SciPy, MATLAB ODE Suite | Solver abstraction, events, stiff systems, forward/adjoint sensitivities |
| Units and quantities | [mp-units](https://mpusz.github.io/mp-units/2.1/) | [Astropy Units](https://docs.astropy.org/en/stable/units/) | Compile-time dimensions, runtime units, conversions, quantity safety |
| Physical constants | [Astropy Constants](https://docs.astropy.org/en/stable/constants/index.html) | CODATA and IAU publications | Versioned constants, uncertainty, provenance |
| PDE and continuum methods | [FEniCSx](https://docs.fenicsproject.org/) | [PETSc SNES](https://petsc.org/release/manualpages/SNES/SNES/), MATLAB PDE Toolbox | Meshes, weak forms, boundary conditions, scalable solver composition |
| Mechanics and orbital systems | [REBOUND](https://rebound.hanno-rein.de/integrators/) | MATLAB Aerospace Toolbox, Astropy | N-body states, integrators, orbital elements, conservation diagnostics |
| Astronomy | [Astropy](https://docs.astropy.org/) | REBOUND, NASA SPICE | Coordinates, cosmology, units, constants, time standards |
| Quantum physics | [QuTiP](https://qutip.org/index.html) | MATLAB Quantum Computing support | States, operators, density matrices, open-system solvers |
| Quantum chemistry | [Psi4](https://psi4.github.io/psi4docs/master/) | PySCF | Molecular geometry, basis sets, SCF, wavefunctions, electronic structure |
| Chemical kinetics | [Cantera](https://www.cantera.org/stable/reference/index.html) | MATLAB/SimBiology | Species, reactions, rate models, reactors, thermodynamics and transport |
| Systems biology | [libRoadRunner](https://www.libroadrunner.org/) | [COPASI](https://copasi.org/) | Reaction networks, SBML, events, steady states, sensitivity and stochastic simulation |
| Molecular biophysics | [OpenMM](https://docs.openmm.org/latest/userguide/) | GROMACS | Force systems, integrators, constraints, trajectories, accelerated simulation |
| Control systems | [python-control](https://python-control.readthedocs.io/en/latest/statesp.html) | [MATLAB Control System Toolbox](https://www.mathworks.com/help/control/index.html) | State-space systems, LQR, Riccati equations, stability and responses |
| MPC and optimal control | [CasADi](https://web.casadi.org/docs/) | acados, MATLAB MPC Toolbox | Dynamic optimization, shooting, collocation, constraints, sensitivities |
| Electric power systems | [MATPOWER](https://matpower.org/documentation/) | PowerModels.jl, PSERC publications | AC/DC network models, power flow, continuation, state estimation, OPF variables/constraints, derivatives, multipliers and benchmark cases |
| Structural mechanics and FEM | [OpenSees](https://github.com/OpenSees/OpenSees), [DOLFINx](https://docs.fenicsproject.org/dolfinx/) | PETSc KSP, CALFEM, MATLAB PDE Toolbox | Elements, degree maps, constraints, assembly, matrix-free solves, reactions, derived stress, null modes, dynamics and nonlinear continuation |
| Signal processing | `scipy.signal` | [MATLAB Signal Processing](https://www.mathworks.com/help/overview/signal-processing.html) | Filtering, spectral estimation, transforms, windows, multirate processing |
| Bayesian statistics | [Stan Math](https://github.com/stan-dev/math) and Stan | PyMC, Turing.jl | Log-density conventions, autodiff, HMC/NUTS, diagnostics, variational inference |
| Abstract algebra | [GAP](https://docs.gap-system.org/doc/ref/manual.pdf) | SageMath | Groups, permutations, actions, representations, canonical forms |
| Computational economics | [QuantEcon](https://dynamics.quantecon.org/genindex.html) | Dynare, MATLAB economics toolboxes | Dynamic programming, Markov models, growth and equilibrium algorithms |
| Actuarial loss models | [`actuar`](https://mirror.linux.ncsu.edu/cran/web/packages/actuar/index.html) | [CRAN Actuarial Science](https://ftp.ussg.iu.edu/CRAN/web/views/ActuarialScience.html) | Heavy-tailed distributions, ruin, credibility and compound losses |
| Life contingencies | [`lifecontingencies`](https://cran.nics.utk.edu/cran/web/packages/lifecontingencies/index.html) | Published actuarial tables and standards | Life tables, annuities, assurances, reserves and pensions |

## Required Audit Record

Before implementation, each family records:

1. the exact upstream version, revision, or local MATLAB release inspected;
2. the public contracts and source files relevant to the tranche;
3. conventions where references disagree;
4. independent numerical vectors and failure cases;
5. reusable Pluss machinery selected as the canonical implementation owner; and
6. any behavior intentionally deferred rather than approximated.

Reference parity means matching a stated contract and test vector. It never means copying
licensed source or reproducing internal structure that does not fit Pluss.
